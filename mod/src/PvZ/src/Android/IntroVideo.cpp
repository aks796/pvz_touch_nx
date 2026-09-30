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

#include "PvZ/Android/IntroVideo.h"
#include "Homura/Logger.h"
#include "PvZ/Android/Native/AudioOutput.h"
#include "PvZ/Android/Native/BridgeApp.h"
#include "PvZ/Android/Native/NativeApp.h"

#include <atomic>

namespace {

std::atomic_bool gVideoCompleted{false};

bool CheckAndClearJniException(JNIEnv *env, const char *operation) {
    if (env == nullptr || !env->ExceptionCheck()) {
        return false;
    }

    LOG_ERROR("JNI exception while calling {}", operation);
    env->ExceptionDescribe();
    env->ExceptionClear();
    return true;
}

class ActivityJniContext {
public:
    JNIEnv *mEnv = nullptr;
    jobject mActivity = nullptr;
    jclass mActivityClass = nullptr;

    ActivityJniContext() {
        Native::BridgeApp *bridgeApp = Native::BridgeApp::getSingleton();
        if (bridgeApp == nullptr || bridgeApp->mNativeApp == nullptr) {
            LOG_ERROR("Video JNI: BridgeApp or NativeApp is null");
            return;
        }

        mEnv = bridgeApp->getJNIEnv();
        if (mEnv == nullptr) {
            LOG_ERROR("Video JNI: JNIEnv is null");
            return;
        }

        // getActivity() is assumed to return a reference owned by NativeApp.
        // Therefore this file does not delete mActivity as a local reference.
        mActivity = bridgeApp->mNativeApp->getActivity();
        if (mActivity == nullptr) {
            LOG_ERROR("Video JNI: activity is null");
            return;
        }

        mActivityClass = mEnv->GetObjectClass(mActivity);
        if (CheckAndClearJniException(mEnv, "GetObjectClass")) {
            mActivityClass = nullptr;
        }
    }

    ~ActivityJniContext() {
        if (mEnv != nullptr && mActivityClass != nullptr) {
            mEnv->DeleteLocalRef(mActivityClass);
        }
    }

    [[nodiscard]] bool IsValid() const {
        return mEnv != nullptr && mActivity != nullptr && mActivityClass != nullptr;
    }

    jmethodID GetMethod(const char *name, const char *signature) const {
        if (!IsValid()) {
            return nullptr;
        }

        jmethodID method = mEnv->GetMethodID(mActivityClass, name, signature);

        if (method == nullptr || CheckAndClearJniException(mEnv, name)) {
            LOG_ERROR("Video JNI: method {}{} was not found", name, signature);
            return nullptr;
        }

        return method;
    }
};

bool CallBooleanMethod(const char *name) {
    ActivityJniContext context;
    jmethodID method = context.GetMethod(name, "()Z");
    if (method == nullptr) {
        return false;
    }

    const jboolean result = context.mEnv->CallBooleanMethod(context.mActivity, method);

    if (CheckAndClearJniException(context.mEnv, name)) {
        return false;
    }

    return result == JNI_TRUE;
}

} // namespace

int AGVideoOpen(const char *videoPath) {
    if (videoPath == nullptr || videoPath[0] == '\0') {
        LOG_ERROR("AGVideoOpen: videoPath is empty");
        return -1;
    }

    LOG_DEBUG("AGVideoOpen: {}", videoPath);

    ActivityJniContext context;
    jmethodID method = context.GetMethod("videoOpen", "(Ljava/lang/String;)Z");
    if (method == nullptr) {
        return -1;
    }

    jstring javaPath = context.mEnv->NewStringUTF(videoPath);
    if (javaPath == nullptr || CheckAndClearJniException(context.mEnv, "NewStringUTF")) {
        return -1;
    }

    const jboolean result = context.mEnv->CallBooleanMethod(context.mActivity, method, javaPath);

    context.mEnv->DeleteLocalRef(javaPath);

    if (CheckAndClearJniException(context.mEnv, "videoOpen")) {
        return -1;
    }

    LOG_DEBUG("AGVideoOpen result: {}", result == JNI_TRUE);

    // Preserve the original AG convention: 0 means success.
    return result == JNI_TRUE ? 0 : -1;
}

bool AGVideoIsPlaying() {
    return CallBooleanMethod("videoIsPlaying");
}

bool AGVideoShow(bool show) {
    LOG_DEBUG("AGVideoShow: {}", show);

    ActivityJniContext context;
    jmethodID method = context.GetMethod("videoShow", "(Z)V");
    if (method == nullptr) {
        return false;
    }

    context.mEnv->CallVoidMethod(context.mActivity, method, show ? JNI_TRUE : JNI_FALSE);

    return !CheckAndClearJniException(context.mEnv, "videoShow");
}

bool AGVideoPlay() {
    LOG_DEBUG("AGVideoPlay");
    return CallBooleanMethod("videoPlay");
}

bool AGVideoStop() {
    LOG_DEBUG("AGVideoStop");
    return CallBooleanMethod("videoStop");
}

bool AGVideoClose() {
    LOG_DEBUG("AGVideoClose");
    return CallBooleanMethod("videoClose");
}

int AGVideoEnable(bool enable) {
    // The Java implementation does not have a separate enable state.
    // Surface creation and visibility are controlled by videoShow().
    LOG_DEBUG("AGVideoEnable: {} (no-op)", enable);
    return 0;
}

void AGVideoResetCompleted() {
    gVideoCompleted.store(false, std::memory_order_release);
}

bool AGVideoConsumeCompleted() {
    return gVideoCompleted.exchange(false, std::memory_order_acq_rel);
}

/*
 * Java declaration:
 *
 * public static native void nativeIntroVideoCompleted();
 *
 * MediaPlayer invokes this callback on Android's main/UI thread. Do not touch
 * TitleScreen or LawnApp directly here. TitleScreen::Update() consumes the
 * atomic flag on the game thread and then calls TitleScreen::VideoCompleted().
 */
extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeIntroVideoCompleted(JNIEnv *, jclass) {
    gVideoCompleted.store(true, std::memory_order_release);
}
