/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "PvZ/Android/Native/AudioOutput.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <array>
#include <condition_variable>
#include <mutex>

namespace {

/*
 * 使用两个完整输入块做双缓冲。
 *
 * 当前每次传入大约 8192 字节：
 * 8192 / 2声道 / 2字节 = 2048帧。
 */
constexpr std::size_t QUEUE_DEPTH = 2;

/*
 * 两个完整缓冲入队后才启动播放器，
 * 避免只播放一个缓冲后队列立即耗尽。
 */
constexpr std::size_t START_THRESHOLD = 2;

/*
 * 当前输入块为8192字节。
 * 留出余量，避免输入块稍微增大时越界。
 */
constexpr std::size_t MAX_BUFFER_BYTES = 16 * 1024;

/* OpenSL ES Engine。 */
SLObjectItf engineObject = nullptr;
SLEngineItf engineEngine = nullptr;

/* Output Mix。 */
SLObjectItf outputMixObject = nullptr;

/* Audio Player。 */
SLObjectItf playerObject = nullptr;
SLPlayItf playerPlay = nullptr;
SLAndroidSimpleBufferQueueItf playerBufferQueue = nullptr;

/*
 * OpenSL ES 的 Enqueue 不会复制传入数据，
 * 所以必须将数据复制到生命周期足够长的内部缓冲。
 */
std::array<std::array<std::uint8_t, MAX_BUFFER_BYTES>, QUEUE_DEPTH> queueBuffers{};

/*
 * writeMutex：
 * 防止多个线程同时写入；
 * 同时保证 shutdown() 不会在 Enqueue 时销毁播放器。
 */
std::mutex writeMutex;

/*
 * queueMutex：
 * 保护队列计数和播放器状态。
 */
std::mutex queueMutex;
std::condition_variable queueCv;

/* 下一个用于写入的内部缓冲下标。 */
std::size_t writeIndex = 0;

/* 已入队但尚未播放完成的缓冲数量。 */
std::size_t queuedCount = 0;

/* 当前 OpenSL 是否已经完整初始化。 */
bool openSlReady = false;

/* 是否正在关闭。 */
bool shuttingDown = true;

/* 上层是否已经请求开始播放。 */
bool playRequested = false;

/*
 * 是否已经向 OpenSL 设置过 PLAYING。
 *
 * 当队列完全耗尽时会重新设置为 false，
 * 下一批数据将重新积累两个缓冲后再启动。
 */
bool playerStarted = false;

bool checkResult(SLresult result) {
    return result == SL_RESULT_SUCCESS;
}

/*
 * OpenSL 每播放完成一个缓冲后调用。
 *
 * 回调中只修改简单状态并唤醒写入线程，
 * 不进行内存分配或复杂操作。
 */
void playerCallback(SLAndroidSimpleBufferQueueItf /* bufferQueue */, void * /* context */) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (queuedCount > 0) {
            --queuedCount;
        }

        /*
         * 队列已经完全耗尽。
         *
         * 下一批数据重新等待两个缓冲后再启动，
         * 避免一个缓冲一个缓冲地反复启动。
         */
        if (queuedCount == 0) {
            playerStarted = false;
        }
    }

    queueCv.notify_one();
}

/*
 * 调用此函数时，调用者必须持有 writeMutex。
 *
 * 两个完整缓冲准备完成后，才真正启动播放器。
 */
bool startPlayerIfReady() {
    SLPlayItf playInterface = nullptr;
    bool shouldStart = false;

    {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (openSlReady && !shuttingDown && playRequested && !playerStarted && queuedCount >= START_THRESHOLD && playerPlay != nullptr) {
            /*
             * 先设置状态，防止重复执行 SetPlayState。
             */
            playerStarted = true;
            playInterface = playerPlay;
            shouldStart = true;
        }
    }

    if (!shouldStart) {
        return true;
    }

    const SLresult result = (*playInterface)->SetPlayState(playInterface, SL_PLAYSTATE_PLAYING);

    if (!checkResult(result)) {
        std::lock_guard<std::mutex> lock(queueMutex);
        playerStarted = false;
        return false;
    }

    return true;
}

namespace opensl {

    /*
     * 提前声明，因为 setup() 初始化前会清理旧实例。
     */
    void shutdown();

    bool setup(int sampleRate, int channels, int bits) {
        /*
         * 防止重复 setup 导致旧对象泄漏。
         */
        shutdown();

        /*
         * 当前实现只支持常用的：
         * 单声道/双声道；
         * 8-bit/16-bit PCM。
         */
        if (sampleRate <= 0 || (channels != 1 && channels != 2) || (bits != 8 && bits != 16)) {
            return false;
        }

        /*
         * 1. 创建 OpenSL Engine。
         */
        SLresult result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);

        if (!checkResult(result) || engineObject == nullptr) {
            shutdown();
            return false;
        }

        result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);

        if (!checkResult(result)) {
            shutdown();
            return false;
        }

        result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);

        if (!checkResult(result) || engineEngine == nullptr) {
            shutdown();
            return false;
        }

        /*
         * 2. 创建 Output Mix。
         */
        result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, nullptr, nullptr);

        if (!checkResult(result) || outputMixObject == nullptr) {
            shutdown();
            return false;
        }

        result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);

        if (!checkResult(result)) {
            shutdown();
            return false;
        }

        /*
         * 3. 配置 Android Simple Buffer Queue。
         */
        SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, static_cast<SLuint32>(QUEUE_DEPTH)};

        SLuint32 channelMask;

        if (channels == 1) {
            channelMask = SL_SPEAKER_FRONT_CENTER;
        } else {
            channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        }

        /*
         * OpenSL ES 的 samplesPerSec 单位是 milliHz，
         * 因此需要 sampleRate * 1000。
         */
        SLDataFormat_PCM pcmFormat = {SL_DATAFORMAT_PCM,
                                      static_cast<SLuint32>(channels),
                                      static_cast<SLuint32>(sampleRate) * 1000U,
                                      static_cast<SLuint32>(bits),
                                      static_cast<SLuint32>(bits),
                                      channelMask,
                                      SL_BYTEORDER_LITTLEENDIAN};

        SLDataSource audioSource = {&bufferQueueLocator, &pcmFormat};

        SLDataLocator_OutputMix outputMixLocator = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};

        SLDataSink audioSink = {&outputMixLocator, nullptr};

        /*
         * 使用 Android Simple Buffer Queue 接口。
         *
         * 不附加音效处理接口，避免影响低延迟路径。
         */
        const SLInterfaceID interfaceIds[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};

        const SLboolean interfaceRequired[] = {SL_BOOLEAN_TRUE};

        /*
         * 4. 创建 Audio Player。
         */
        result = (*engineEngine)->CreateAudioPlayer(engineEngine, &playerObject, &audioSource, &audioSink, 1, interfaceIds, interfaceRequired);

        if (!checkResult(result) || playerObject == nullptr) {
            shutdown();
            return false;
        }

        result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);

        if (!checkResult(result)) {
            shutdown();
            return false;
        }

        /*
         * 5. 获取播放接口。
         */
        result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay);

        if (!checkResult(result) || playerPlay == nullptr) {
            shutdown();
            return false;
        }

        /*
         * 6. 获取 Android Simple Buffer Queue 接口。
         *
         * 这里必须和创建时使用的接口一致。
         */
        result = (*playerObject)->GetInterface(playerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &playerBufferQueue);

        if (!checkResult(result) || playerBufferQueue == nullptr) {
            shutdown();
            return false;
        }

        /*
         * 7. 注册缓冲播放完成回调。
         */
        result = (*playerBufferQueue)->RegisterCallback(playerBufferQueue, playerCallback, nullptr);

        if (!checkResult(result)) {
            shutdown();
            return false;
        }

        /*
         * 所有对象创建成功后，最后再开放 write()。
         */
        {
            std::lock_guard<std::mutex> lock(queueMutex);

            writeIndex = 0;
            queuedCount = 0;

            shuttingDown = false;
            playRequested = false;
            playerStarted = false;
            openSlReady = true;
        }

        return true;
    }

    /*
     * play() 不立即启动空播放器。
     *
     * 真正的 PLAYING 状态在两个完整缓冲入队后设置。
     */
    void play() {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (!openSlReady || shuttingDown) {
            return;
        }

        playRequested = true;
    }

    int write(const void *data, int dataSize) {
        if (data == nullptr || dataSize <= 0) {
            return 0;
        }

        if (static_cast<std::size_t>(dataSize) > MAX_BUFFER_BYTES) {
            /*
             * 防止 memcpy 越界。
             */
            return 0;
        }

        /*
         * 保证同一时间只有一个生产者写入，
         * 并防止 shutdown() 同时销毁接口。
         */
        std::lock_guard<std::mutex> producerLock(writeMutex);

        std::size_t slotIndex;
        SLAndroidSimpleBufferQueueItf queueInterface;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            /*
             * 两个缓冲都被占用时才等待。
             *
             * stop()/shutdown() 会修改状态并唤醒这里，
             * 因此不会永久死锁。
             */
            queueCv.wait(lock, [] { return shuttingDown || !openSlReady || !playRequested || queuedCount < QUEUE_DEPTH; });

            if (shuttingDown || !openSlReady || !playRequested || playerBufferQueue == nullptr) {
                return 0;
            }

            slotIndex = writeIndex;
            queueInterface = playerBufferQueue;

            /*
             * Enqueue 不复制数据，
             * 因此先复制到内部持久缓冲。
             */
            std::memcpy(queueBuffers[slotIndex].data(), data, static_cast<std::size_t>(dataSize));

            /*
             * 提交一个完整输入块。
             */
            const SLresult result = (*queueInterface)->Enqueue(queueInterface, queueBuffers[slotIndex].data(), static_cast<SLuint32>(dataSize));

            if (!checkResult(result)) {
                return 0;
            }

            writeIndex = (writeIndex + 1) % QUEUE_DEPTH;

            ++queuedCount;
        }

        /*
         * 第二个完整缓冲入队后才启动。
         */
        if (!startPlayerIfReady()) {
            /*
             * 如果启动失败，清空队列，
             * 防止后续 write() 永远等待 callback。
             */
            if (playerBufferQueue != nullptr) {
                (*playerBufferQueue)->Clear(playerBufferQueue);
            }

            {
                std::lock_guard<std::mutex> lock(queueMutex);

                queuedCount = 0;
                writeIndex = 0;

                playerStarted = false;
                playRequested = false;
            }

            queueCv.notify_all();
            return 0;
        }

        return dataSize;
    }

    [[maybe_unused]] void stop() {
        /*
         * 先禁止新的 write() 继续入队。
         */
        {
            std::lock_guard<std::mutex> lock(queueMutex);

            playRequested = false;
            playerStarted = false;
        }

        queueCv.notify_all();

        /*
         * 等待正在进行的 write() 完成。
         */
        std::lock_guard<std::mutex> producerLock(writeMutex);

        if (playerPlay != nullptr) {
            (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_STOPPED);
        }

        /*
         * Android 文档建议 STOPPED 后主动 Clear。
         */
        if (playerBufferQueue != nullptr) {
            (*playerBufferQueue)->Clear(playerBufferQueue);
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);

            queuedCount = 0;
            writeIndex = 0;
        }

        queueCv.notify_all();
    }

    void shutdown() {
        /*
         * 先阻止新写入，并唤醒所有等待线程。
         */
        {
            std::lock_guard<std::mutex> lock(queueMutex);

            openSlReady = false;
            shuttingDown = true;
            playRequested = false;
            playerStarted = false;
        }

        queueCv.notify_all();

        /*
         * 等待正在执行的 write() 退出，
         * 再销毁 OpenSL 对象。
         */
        std::lock_guard<std::mutex> producerLock(writeMutex);

        if (playerPlay != nullptr) {
            (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_STOPPED);
        }

        if (playerBufferQueue != nullptr) {
            (*playerBufferQueue)->Clear(playerBufferQueue);
        }

        if (playerObject != nullptr) {
            (*playerObject)->Destroy(playerObject);

            playerObject = nullptr;
            playerPlay = nullptr;
            playerBufferQueue = nullptr;
        }

        if (outputMixObject != nullptr) {
            (*outputMixObject)->Destroy(outputMixObject);

            outputMixObject = nullptr;
        }

        if (engineObject != nullptr) {
            (*engineObject)->Destroy(engineObject);

            engineObject = nullptr;
            engineEngine = nullptr;
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);

            queuedCount = 0;
            writeIndex = 0;
        }
    }

} // namespace opensl

} // namespace

/*
 * 保持 HookInit.cpp 使用的全局函数。
 *
 * 返回值保持 int。
 */
int AudioWrite(const void *data, int dataSize) {
    return opensl::write(data, dataSize);
}

void Native::AudioOutput::initialize() {
    old_Native_AudioOutput_initialize(this);
}

bool Native::AudioOutput::setup(int sampleRate, int channels, int bits) {
    /*
     * 保留原游戏初始化，
     * 避免其内部对象状态没有建立。
     */
    const bool oldResult = old_Native_AudioOutput_setup(this, sampleRate, channels, bits);

    const bool openSlResult = opensl::setup(sampleRate, channels, bits);

    if (openSlResult) {
        opensl::play();
    }

    return oldResult && openSlResult;
}

int Native::AudioOutput::write(const void *data, int dataSize) {
    return AudioWrite(data, dataSize);
}

void Native::AudioOutput::shutdown() {
    opensl::shutdown();
    old_Native_AudioOutput_shutdown(this);
}
