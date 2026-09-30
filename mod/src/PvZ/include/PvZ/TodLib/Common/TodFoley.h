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

#ifndef PVZ_SEXYAPPFRAMEWORK_TODLIB_COMMON_TOD_FOLEY_H
#define PVZ_SEXYAPPFRAMEWORK_TODLIB_COMMON_TOD_FOLEY_H

#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/Common/Resources.h"
#include "PvZ/Symbols.h"

inline constexpr int MAX_FOLEY_TYPES = 110;
inline constexpr int MAX_FOLEY_INSTANCES = 8;

// ######################################################################################################################################################
// ############################################################# 以下为拟音音效定义相关内容 #############################################################
// ######################################################################################################################################################

class FoleyParams {
public:
    FoleyType mFoleyType;
    float mPitchRange;
    int *mSfxID[10];
    unsigned int mFoleyFlags;
};

FoleyParams *LookupFoley(FoleyType theFoleyType);
void TodFoleyInitialize(FoleyParams *theFoleyParamArray, int theFoleyParamArraySize);

auto GetNewLawnFoleyParamArray() -> FoleyParams (&)[FoleyType::EXTENDED_NUM_FOLEY];

// ######################################################################################################################################################
// ############################################################ 以下正式开始拟音音效相关声明 ############################################################
// ######################################################################################################################################################


namespace Sexy {
class AudiereSoundInstance {

public:
    struct AudiereSoundInstanceVTable {
        void (*completeDestructor)(Sexy::AudiereSoundInstance *self);                   // 0x00
        void (*deletingDestructor)(Sexy::AudiereSoundInstance *self);                   // 0x04
        void (*Release)(Sexy::AudiereSoundInstance *self);                              // 0x08
        void (*SetBaseVolume)(Sexy::AudiereSoundInstance *self, double volume);         // 0x0C
        void (*SetBasePan)(Sexy::AudiereSoundInstance *self, int pan);                  // 0x10
        void (*SetBaseRate)(Sexy::AudiereSoundInstance *self, double rate);             // 0x14
        void (*AdjustPitch)(Sexy::AudiereSoundInstance *self, double pitch);            // 0x18
        void (*SetVolume)(Sexy::AudiereSoundInstance *self, double volume);             // 0x1C
        void (*SetMasterVolumeIdx)(Sexy::AudiereSoundInstance *self, int volumeIndex);  // 0x20
        void (*SetPan)(Sexy::AudiereSoundInstance *self, int pan);                      // 0x24
        bool (*Play)(Sexy::AudiereSoundInstance *self, bool looping, bool autoRelease); // 0x28
        void (*Stop)(Sexy::AudiereSoundInstance *self);                                 // 0x2C
        bool (*IsPlaying)(Sexy::AudiereSoundInstance *self);                            // 0x30
        bool (*IsReleased)(Sexy::AudiereSoundInstance *self);                           // 0x34
        double (*GetVolume)(Sexy::AudiereSoundInstance *self);                          // 0x38
    };

    const AudiereSoundInstanceVTable *GetVTable() const {
        return (AudiereSoundInstanceVTable *)vTable;
    }

public:
    void **vTable;
};
} // namespace Sexy

class FoleyInstance {
public:
    Sexy::AudiereSoundInstance *mInstance;
    int mRefCount;
    bool _paused;
    int mStartTime;
    int mPauseOffset;
};

struct FoleyTypeData {
    FoleyInstance mFoleyInstances[MAX_FOLEY_INSTANCES];
    int mLastVariationPlayed;

    FoleyTypeData()
        : mLastVariationPlayed(-1) {}
};

class TodFoley {
public:
    //    FoleyTypeData mTypeData[MAX_FOLEY_TYPES];
    FoleyTypeData mTypeData[EXTENDED_NUM_FOLEY];

    bool IsFoleyPlaying(FoleyType theFoleyType) {
        return reinterpret_cast<bool (*)(TodFoley *, FoleyType)>(TodFoley_IsFoleyPlayingAddr)(this, theFoleyType);
    }
    void StopFoley(FoleyType theFoleyType) {
        reinterpret_cast<void (*)(TodFoley *, FoleyType)>(TodFoley_StopFoleyAddr)(this, theFoleyType);
    }
    void CancelPausedFoley() {
        reinterpret_cast<void (*)(TodFoley *)>(TodFoley_CancelPausedFoleyAddr)(this);
    }
    void ApplyMusicVolume(FoleyInstance *theInstance) {
        reinterpret_cast<void (*)(TodFoley *, FoleyInstance *)>(TodFoley_ApplyMusicVolumeAddr)(this, theInstance);
    }
    void RehookupSoundWithMusicVolume();


    TodFoley() = default;
    ~TodFoley() = default;
};

inline FoleyInstance *SoundSystemFindInstance(TodFoley *theSoundSystem, FoleyType theFoleyType) {
    return reinterpret_cast<FoleyInstance *(*)(TodFoley *, FoleyType)>(SoundSystemFindInstanceAddr)(theSoundSystem, theFoleyType);
}

void SoundSystemReleaseFinishedInstances(TodFoley *theSoundSystem);

#endif // PVZ_SEXYAPPFRAMEWORK_TODLIB_COMMON_TOD_FOLEY_H
