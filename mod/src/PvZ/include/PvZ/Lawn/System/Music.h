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

#ifndef PVZ_LAWN_SYSTEM_MUSIC_H
#define PVZ_LAWN_SYSTEM_MUSIC_H

#include "PvZ/Lawn/Common/SyncObject.h"
#include "PvZ/STL/string.h"
#include "PvZ/SexyAppFramework/Sound/MusicInterface.h"
#include "PvZ/Symbols.h"

class LawnApp;

enum MusicTune {
    MUSIC_TUNE_NONE = -1,
    MUSIC_TUNE_DAY_GRASSWALK = 1,            // 白天草地关卡
    MUSIC_TUNE_NIGHT_MOONGRAINS,             // 黑夜草地关卡
    MUSIC_TUNE_POOL_WATERYGRAVES,            // 白天泳池关卡
    MUSIC_TUNE_FOG_RIGORMORMIST,             // 黑夜泳池关卡
    MUSIC_TUNE_ROOF_GRAZETHEROOF,            // 屋顶关卡
    MUSIC_TUNE_CHOOSE_YOUR_SEEDS,            // 选卡界面/小游戏界面
    MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME,  // 主菜单
    MUSIC_TUNE_ZEN_GARDEN,                   // 禅境花园
    MUSIC_TUNE_PUZZLE_CEREBRAWL,             // 解谜模式
    MUSIC_TUNE_MINIGAME_LOONBOON,            // 小游戏
    MUSIC_TUNE_CONVEYER,                     // 传送带关卡
    MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC,   // 僵王博士关卡
    MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN, // MV
    NUM_MUSIC_TUNES
};

enum MusicFile {
    MUSIC_FILE_NONE = -1,
    MUSIC_FILE_MAIN_MUSIC = 1,
    MUSIC_FILE_DRUMS = 2,
    MUSIC_FILE_DRUMS_NIGHTMOONGRAINS = 3,
    MUSIC_FILE_HIHATS = 4,
    MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN = 5,
    NUM_MUSIC_FILES = 6,
};

enum MusicBurstState {
    MUSIC_BURST_OFF = 0,
    MUSIC_BURST_STARTING = 1,
    MUSIC_BURST_ON = 2,
    MUSIC_BURST_FINISHING = 3,
};

enum MusicDrumsState {
    MUSIC_DRUMS_OFF = 0,
    MUSIC_DRUMS_ON_QUEUED = 1,
    MUSIC_DRUMS_ON = 2,
    MUSIC_DRUMS_OFF_QUEUED = 3,
    MUSIC_DRUMS_FADING = 4,
};

class Music : public SyncObject { // 加载XBOX版xm格式音乐时用。优：音质好、有鼓点。缺：鼓点BUG多，xm格式难以修改
public:
    struct MusicVTable {
        void (*completeDestructor)(Music *self);                                              // 0x00
        void (*deletingDestructor)(Music *self);                                              // 0x04
        void (*MusicInit)(Music *self);                                                       // 0x08
        void (*MusicDispose)(Music *self);                                                    // 0x0C
        void (*MusicUpdate)(Music *self);                                                     // 0x10
        void (*StopAllMusic)(Music *self);                                                    // 0x14
        void (*StartGameMusic)(Music *self, bool startPaused);                                // 0x18
        void (*LoadSong)(Music *self, MusicFile musicFile, const std::string &fileName);      // 0x1C
        void (*MusicResync)(Music *self);                                                     // 0x20
        void (*UpdateMusicBurst)(Music *self);                                                // 0x24
        void (*StartBurst)(Music *self);                                                      // 0x28
        void (*GameMusicPause)(Music *self, bool paused);                                     // 0x2C
        void (*MusicResyncChannel)(Music *self, MusicFile musicFile1, MusicFile musicFile2);  // 0x30
        void (*MusicTitleScreenInit)(Music *self);                                            // 0x34
        void (*MakeSureMusicIsPlaying)(Music *self, MusicTune musicTune);                     // 0x38
        void (*FadeOut)(Music *self, int fadeTime);                                           // 0x3C
        void (*SetupMusicFileForTune)(Music *self, MusicFile musicFile, MusicTune musicTune); // 0x40
        int (*GetMusicOrder)(Music *self, MusicFile musicFile);                               // 0x44
        int (*GetNumLoadingTasks)(Music *self);                                               // 0x48
        void (*MusicLoadCreditsSong)(Music *self);                                            // 0x4C
        void (*PlayFromOffset)(Music *self, MusicFile musicFile, int offset, double volume);  // 0x50
    };

public:
    LawnApp *mApp;                         // 4
    Sexy::MusicInterface *mMusicInterface; // 5
    MusicTune mCurMusicTune;               // 6
    MusicTune mLastMusicTune;              // 7
    MusicFile mCurMusicFileMain;           // 8
    MusicFile mCurMusicFileDrums;          // 9
    MusicFile mCurMusicFileHihats;         // 10
    int mBurstOverride;                    // 11
    int mBaseBPM;                          // 12
    int mBaseModSpeed;                     // 13
    MusicBurstState mMusicBurstState;      // 14
    int mBurstStateCounter;                // 15
    MusicDrumsState mMusicDrumsState;      // 16
    int mQueuedDrumTrackPackedOrder;       // 17
    int mDrumsStateCounter;                // 18
    int mPauseOffset;                      // 19
    int mPauseOffsetDrums;                 // 20
    bool mPaused;                          // 84
    bool mMusicDisabled;                   // 85
    int mFadeOutCounter;                   // 22
    int mFadeOutDuration;                  // 23
    float mPauseVolume;                    // 24
    float mNormalVolume;                   // 25
    // 大小26个整数

    void StopAllMusic() {
        reinterpret_cast<void (*)(Music *)>(Music_StopAllMusicAddr)(this);
    }
    unsigned long GetMusicOrder(MusicFile theMusicFile) {
        return reinterpret_cast<unsigned long (*)(Music *, MusicFile)>(Music_GetMusicOrderAddr)(this, theMusicFile);
    }
    void SetupMusicFileForTune(MusicFile theMusicFile, MusicTune theMusicTune) {
        reinterpret_cast<void (*)(Music *, MusicFile, MusicTune)>(Music_SetupMusicFileForTuneAddr)(this, theMusicFile, theMusicTune);
    }
    void LoadSong(MusicFile theMusicFile, const pvzstl::string &theFileName) {
        reinterpret_cast<void (*)(Music *, MusicFile, const pvzstl::string &)>(Music_LoadSongAddr)(this, theMusicFile, theFileName);
    }
    void MakeSureMusicIsPlaying(MusicTune theMusicTune) {
        reinterpret_cast<void (*)(Music *, MusicTune)>(Music_MakeSureMusicIsPlayingAddr)(this, theMusicTune);
    }
    const MusicVTable *GetVTable() const {
        return (MusicVTable *)vTable;
    }
    void PlayMusic(MusicTune theMusicTune, int theOffset, int theDrumsOffset);
    void MusicUpdate();
    void UpdateMusicBurst();
    void MusicResync();
    void StartBurst();
    void PlayFromOffset(MusicFile theMusicFile, int theOffset, double theVolume);
    void UpdateMusicBurst2();
    void ResyncChannel(MusicFile theFile1, MusicFile theFile2);
    void StartGameMusic(bool theStart);
    void MusicTitleScreenInit();

protected:
    Music() = default;
    ~Music() = default;

    void _constructor() {
        reinterpret_cast<void (*)(Music *)>(Music_MusicAddr)(this);
    }
};

class Music2 : public Music { // 加载TV版ogg格式音乐时用。无鼓点。
public:
    struct Music2VTable {
        void (*completeDestructor)(Music2 *self);                                               // 0x00
        void (*deletingDestructor)(Music2 *self);                                               // 0x04
        void (*MusicInit)(Music2 *self);                                                        // 0x08
        void (*MusicDispose)(Music2 *self);                                                     // 0x0C
        void (*MusicUpdate)(Music2 *self);                                                      // 0x10
        void (*StopAllMusic)(Music2 *self);                                                     // 0x14
        void (*StartGameMusic)(Music2 *self, bool startPaused);                                 // 0x18
        void (*LoadSongByFile)(Music *self, MusicFile musicFile, const std::string &fileName);  // 0x1C
        void (*MusicResync)(Music2 *self);                                                      // 0x20
        void (*UpdateMusicBurst)(Music2 *self);                                                 // 0x24
        void (*StartBurst)(Music2 *self);                                                       // 0x28
        void (*GameMusicPause)(Music2 *self, bool paused);                                      // 0x2C
        void (*MusicResyncChannel)(Music2 *self, MusicFile musicFile1, MusicFile musicFile2);   // 0x30
        void (*MusicTitleScreenInit)(Music2 *self);                                             // 0x34
        void (*MakeSureMusicIsPlaying)(Music2 *self, MusicTune musicTune);                      // 0x38
        void (*FadeOut)(Music2 *self, int fadeTime);                                            // 0x3C
        void (*SetupMusicFileForTune)(Music *self, MusicFile musicFile, MusicTune musicTune);   // 0x40
        int (*GetMusicOrder)(Music2 *self, MusicFile musicFile);                                // 0x44
        int (*GetNumLoadingTasks)(Music2 *self);                                                // 0x48
        void (*MusicLoadCreditsSong)(Music2 *self);                                             // 0x4C
        void (*PlayFromOffset)(Music2 *self, MusicFile musicFile, int offset, double volume);   // 0x50
        void (*LoadSongByTune)(Music2 *self, MusicTune musicTune, const std::string &fileName); // 0x54
    };

public:
    void MakeSureMusicIsPlaying(MusicTune theMusicTune) { // vTable[14]
        reinterpret_cast<void (*)(Music *, MusicTune)>(Music2_MakeSureMusicIsPlayingAddr)(this, theMusicTune);
    }
    const Music2VTable *GetVTable() const {
        return (Music2VTable *)vTable;
    }
    void StopAllMusic();
    void StartGameMusic(bool theStart);
    void GameMusicPause(bool thePause);
    void FadeOut(int theFadeOutDuration);

protected:
    friend void InitHookFunction();

    Music2() = default;
    ~Music2() = default;

    void _constructor();
    void _destructor() {
        reinterpret_cast<void (*)(Music2 *)>(Music2__destructorAddr)(this);
    }
};

inline void (*old_Music_StartGameMusic)(Music *music, bool a2);

inline void (*old_Music_UpdateMusicBurst)(Music *music);

inline void (*old_Music2_Music2)(Music2 *music);

inline void (*old_Music2_StopAllMusic)(Music2 *music);

inline void (*old_Music2_StartGameMusic)(Music2 *music, bool start);

inline void (*old_Music2_GameMusicPause)(Music2 *music, bool pause);

inline void (*old_Music2_FadeOut)(Music2 *music, int aFadeOutDuration);

#endif // PVZ_LAWN_SYSTEM_MUSIC_H
