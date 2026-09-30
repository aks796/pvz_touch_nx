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

#include "PvZ/Lawn/System/SaveGame.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/CursorObject.h"
#include "PvZ/Lawn/Board/MessageWidget.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/Music.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

bool LawnSaveGame_Original(Board *theBoard, const pvzstl::string &theFilePath) {
    SaveGameContext aContext{};
    aContext.mFailed = false;
    aContext.mReading = false;

    SaveFileHeader aHeader{};
    aHeader.mMagicNumber = SAVE_FILE_MAGIC_NUMBER;
    aHeader.mBuildVersion = SAVE_FILE_VERSION;
    aHeader.mBuildDate = SAVE_FILE_DATE;

    aContext.SyncBytes(&aHeader, sizeof(aHeader));
    SyncBoard(&aContext, theBoard);
    theBoard->mApp->mNeedGoBackToMain = false; // 用于从暂停菜单返回主界面
    return gLawnApp->WriteBufferToFile(theFilePath, &aContext.mBuffer);
}

bool LawnSaveGame(Board *theBoard, const pvzstl::string &theFilePath) {
    if (disableSaveUserdata) {
        theBoard->mApp->mNeedGoBackToMain = false; // 用于从暂停菜单返回主界面
        return true;
    }

    // 结盟模式存档，将SeedBank2的4个种子放到SeedBank1里面。因为原版存档逻辑难以改动，只好出此下策，凑合着存吧。
    if (theBoard->mApp->IsCoopMode()) {
        if (theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOWLING || theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOSS) {
            int aNumSeeds = 6;
            SeedBank *seedBank1 = theBoard->mSeedBank[0];
            SeedBank *seedBank2 = theBoard->mSeedBank[1];
            seedBank1->mX = seedBank2->mX;
            for (int i = 0; i < aNumSeeds; ++i) {
                seedBank1->mSeedPackets[i].mSlotMachiningNextSeed = (SeedType)seedBank2->mSeedPackets[i].mY;
                seedBank1->mSeedPackets[i].mTimesUsed = seedBank2->mSeedPackets[i].mX;
                seedBank1->mSeedPackets[i].mImitaterType = seedBank2->mSeedPackets[i].mPacketType;
                seedBank1->mSeedPackets[i].mRefreshCounter = seedBank2->mSeedPackets[i].mOffsetY;
                seedBank1->mSeedPackets[i].mSlotMachineCountDown = seedBank2->mSeedPackets[i].mIndex;
            }
            bool result = LawnSaveGame_Original(theBoard, theFilePath);
            seedBank1->mX = 0;
            return result;
        } else {
            int theSeedNum = 4;
            SeedBank *seedBank1 = theBoard->mSeedBank[0];
            SeedBank *seedBank2 = theBoard->mSeedBank[1];
            seedBank1->mNumPackets = 2 * theSeedNum;
            seedBank1->mX = seedBank2->mX;
            for (int i = theSeedNum; i < 2 * theSeedNum; ++i) {
                seedBank1->mSeedPackets[i].mX = seedBank2->mSeedPackets[i - theSeedNum].mX;
                seedBank1->mSeedPackets[i].mY = seedBank2->mSeedPackets[i - theSeedNum].mY;
                seedBank1->mSeedPackets[i].mRefreshCounter = seedBank2->mSeedPackets[i - theSeedNum].mRefreshCounter;
                seedBank1->mSeedPackets[i].mRefreshTime = seedBank2->mSeedPackets[i - theSeedNum].mRefreshTime;
                seedBank1->mSeedPackets[i].mIndex = seedBank2->mSeedPackets[i - theSeedNum].mIndex;
                seedBank1->mSeedPackets[i].mOffsetY = seedBank2->mSeedPackets[i - theSeedNum].mOffsetY;
                seedBank1->mSeedPackets[i].mPacketType = seedBank2->mSeedPackets[i - theSeedNum].mPacketType;
                seedBank1->mSeedPackets[i].mImitaterType = seedBank2->mSeedPackets[i - theSeedNum].mImitaterType;
                seedBank1->mSeedPackets[i].mActive = seedBank2->mSeedPackets[i - theSeedNum].mActive;
                seedBank1->mSeedPackets[i].mRefreshing = seedBank2->mSeedPackets[i - theSeedNum].mRefreshing;
                seedBank1->mSeedPackets[i].mTimesUsed = seedBank2->mSeedPackets[i - theSeedNum].mTimesUsed;
                seedBank1->mSeedPackets[i].mSeedBank = seedBank1;
                seedBank1->mSeedPackets[i].mSelectedBy2P = seedBank2->mSeedPackets[i - theSeedNum].mSelectedBy2P;
                seedBank1->mSeedPackets[i].mSelected = seedBank2->mSeedPackets[i - theSeedNum].mSelected;
                seedBank1->mSeedPackets[i].mSelectedByBothPlayer = seedBank2->mSeedPackets[i - theSeedNum].mSelectedByBothPlayer;
            }
            bool result = LawnSaveGame_Original(theBoard, theFilePath);
            seedBank1->mNumPackets = theSeedNum;
            seedBank1->mX = 0;
            return result;
        }
    }
    // Zombie *zombie = NULL;
    // while (Board_IterateZombies(theBoard, &zombie)) {
    // if (zombie->mZombieType == ZombieType::Flag) {
    // LawnApp_RemoveReanimation(zombie->mApp, zombie->mBossFireBallReanimID);
    // zombie->mBossFireBallReanimID = 0;
    // }
    // }
    return LawnSaveGame_Original(theBoard, theFilePath);
}

bool LawnLoadGame_Original(Board *theBoard, SaveGameContext *theContext) {

    SaveFileHeader aHeader{};
    theContext->SyncBytes(&aHeader, sizeof(aHeader));

    // 检查存档魔数和版本范围。
    if (aHeader.mMagicNumber != SAVE_FILE_MAGIC_NUMBER || aHeader.mBuildVersion > SAVE_FILE_VERSION) {
        gLawnApp->HandleCorruptedGameFile();
        return false;
    }
    // 魔数正确，但不是当前支持的版本。
    if (aHeader.mBuildVersion != SAVE_FILE_VERSION) {
        gLawnApp->HandleOldGameFile();
        return false;
    }
    SyncBoard(theContext, theBoard);
    if (gLawnApp->IsAdventureMode()) {
        if (gLawnApp->mPlayerInfo->mLevel != theBoard->mLevel) {
            const int highestLevel = theBoard->mLevel > gLawnApp->mPlayerInfo->mLevel ? theBoard->mLevel : gLawnApp->mPlayerInfo->mLevel;
            gLawnApp->mPlayerInfo->mLevel = highestLevel;
        }
    }
    if (theContext->mFailed) {
        gLawnApp->HandleCorruptedGameFile();
        return false;
    }

    FixBoardAfterLoad(theBoard);
    theBoard->mApp->mGameScene = GameScenes::SCENE_PLAYING;
    return true;
}

void FixBoardAfterLoad(Board *theBoard) {
    LawnApp *app = theBoard->mApp;

    theBoard->mTangleKelpTree->clear();
    theBoard->mFlowerPotTree->clear();
    theBoard->mPumpkinTree->clear();

    Plant *aPlant = nullptr;
    while (theBoard->mPlants.IterateNext(aPlant)) {
        aPlant->mApp = app;
        aPlant->mBoard = theBoard;

        switch (aPlant->mSeedType) {
            case SeedType::SEED_TANGLEKELP:
                theBoard->mTangleKelpTree->emplace(aPlant);
                break;

            case SeedType::SEED_FLOWERPOT:
                theBoard->mFlowerPotTree->emplace(aPlant);
                break;

            case SeedType::SEED_PUMPKINSHELL:
                theBoard->mPumpkinTree->emplace(aPlant);
                break;

            default:
                break;
        }
    }

    Zombie *aZombie = nullptr;
    while (theBoard->mZombies.IterateNext(aZombie)) {
        aZombie->mApp = app;
        aZombie->mBoard = theBoard;

        aZombie->StartZombieSound();
    }

    Projectile *aProjectile = nullptr;
    while (theBoard->mProjectiles.IterateNext(aProjectile)) {
        aProjectile->mApp = app;
        aProjectile->mBoard = theBoard;
    }

    Coin *aCoin = nullptr;
    while (theBoard->mCoins.IterateNext(aCoin)) {
        aCoin->mApp = app;
        aCoin->mBoard = theBoard;
    }

    LawnMower *aLawnMower = nullptr;
    while (theBoard->mLawnMowers.IterateNext(aLawnMower)) {
        aLawnMower->mApp = app;
        aLawnMower->mBoard = theBoard;
    }

    GridItem *aGridItem = nullptr;
    while (theBoard->mGridItems.IterateNext(aGridItem)) {
        aGridItem->mApp = app;
        aGridItem->mBoard = theBoard;
    }

    theBoard->mAdvice->mApp = app;
    theBoard->mCursorObject[0]->mApp = app;
    theBoard->mCursorObject[0]->mBoard = theBoard;
    theBoard->mCursorPreview[0]->mApp = app;
    theBoard->mCursorPreview[0]->mBoard = theBoard;
    SeedBank *aSeedBank = theBoard->mSeedBank[0];
    aSeedBank->mApp = app;
    aSeedBank->mBoard = theBoard;
    for (auto &aPacket : aSeedBank->mSeedPackets) {
        aPacket.mApp = app;
        aPacket.mBoard = theBoard;
        aPacket.mSeedBank = aSeedBank;
    }
    theBoard->mChallenge->mApp = app;
    theBoard->mChallenge->mBoard = theBoard;
    theBoard->mGamepadControls[0]->mGamepadIndex = app->PlayerToGamepadIndex(theBoard->mGamepadControls[0]->mPlayerIndex);
    app->mMusic->mApp = app;
    app->mMusic->mMusicInterface = app->mMusicInterface;

    // 修复读档后的各种问题
    theBoard->FixReanimErrorAfterLoad();
}

bool LawnLoadGame(Board *theBoard, SaveGameContext *theContext) {
    // 结盟模式读档，将SeedBank2的4个种子从SeedBank1里面取出。因为原版读档逻辑难以改动，只好出此下策，凑合着读吧。
    if (theBoard->mApp->IsCoopMode()) {
        if (theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOWLING || theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOSS) {
            bool result = LawnLoadGame_Original(theBoard, theContext);
            int theSeedNum = 6;
            SeedBank *seedBank1 = theBoard->mSeedBank[0];
            SeedBank *seedBank2 = theBoard->mSeedBank[1];
            seedBank2->mNumPackets = theSeedNum;
            seedBank1->mNumPackets = theSeedNum;
            seedBank2->mX = seedBank1->mX;
            seedBank1->mX = 0;
            for (int i = 0; i < theSeedNum; ++i) {
                seedBank2->mSeedPackets[i].mY = seedBank1->mSeedPackets[i].mSlotMachiningNextSeed;
                seedBank2->mSeedPackets[i].mX = seedBank1->mSeedPackets[i].mTimesUsed;
                seedBank2->mSeedPackets[i].mPacketType = seedBank1->mSeedPackets[i].mImitaterType;
                seedBank2->mSeedPackets[i].mOffsetY = seedBank1->mSeedPackets[i].mRefreshCounter;
                seedBank2->mSeedPackets[i].mIndex = seedBank1->mSeedPackets[i].mSlotMachineCountDown;

                seedBank1->mSeedPackets[i].mTimesUsed = 0;
                seedBank1->mSeedPackets[i].mImitaterType = SeedType::SEED_NONE;
                seedBank1->mSeedPackets[i].mRefreshCounter = 0;
                seedBank1->mSeedPackets[i].mSlotMachineCountDown = 0;
                seedBank1->mSeedPackets[i].mSlotMachiningNextSeed = SeedType::SEED_NONE;
            }
            return result;
        } else {
            bool result = LawnLoadGame_Original(theBoard, theContext);
            int theSeedNum = 4;
            SeedBank *seedBank1 = theBoard->mSeedBank[0];
            SeedBank *seedBank2 = theBoard->mSeedBank[1];
            seedBank2->mNumPackets = theSeedNum;
            seedBank1->mNumPackets = theSeedNum;
            seedBank2->mX = seedBank1->mX;
            seedBank1->mX = 0;
            for (int i = theSeedNum; i < 2 * theSeedNum; ++i) {
                seedBank2->mSeedPackets[i - theSeedNum].mX = seedBank1->mSeedPackets[i].mX;
                seedBank2->mSeedPackets[i - theSeedNum].mY = seedBank1->mSeedPackets[i].mY;
                seedBank2->mSeedPackets[i - theSeedNum].mRefreshCounter = seedBank1->mSeedPackets[i].mRefreshCounter;
                seedBank2->mSeedPackets[i - theSeedNum].mRefreshTime = seedBank1->mSeedPackets[i].mRefreshTime;
                seedBank2->mSeedPackets[i - theSeedNum].mIndex = seedBank1->mSeedPackets[i].mIndex;
                seedBank2->mSeedPackets[i - theSeedNum].mOffsetY = seedBank1->mSeedPackets[i].mOffsetY;
                seedBank2->mSeedPackets[i - theSeedNum].mPacketType = seedBank1->mSeedPackets[i].mPacketType;
                seedBank2->mSeedPackets[i - theSeedNum].mImitaterType = seedBank1->mSeedPackets[i].mImitaterType;
                seedBank2->mSeedPackets[i - theSeedNum].mActive = seedBank1->mSeedPackets[i].mActive;
                seedBank2->mSeedPackets[i - theSeedNum].mRefreshing = seedBank1->mSeedPackets[i].mRefreshing;
                seedBank2->mSeedPackets[i - theSeedNum].mTimesUsed = seedBank1->mSeedPackets[i].mTimesUsed;
                seedBank2->mSeedPackets[i - theSeedNum].mSeedBank = seedBank2;
                seedBank2->mSeedPackets[i - theSeedNum].mSelectedBy2P = seedBank1->mSeedPackets[i].mSelectedBy2P;
                seedBank2->mSeedPackets[i - theSeedNum].mSelected = seedBank1->mSeedPackets[i].mSelected;
                seedBank2->mSeedPackets[i - theSeedNum].mSelectedByBothPlayer = seedBank1->mSeedPackets[i].mSelectedByBothPlayer;
            }
            return result;
        }
    }


    return LawnLoadGame_Original(theBoard, theContext);
}


void SaveGameContext::SyncReanimationDef(ReanimatorDefinition *&theDefinition) {
    // 解决大头贴动画的读档问题
    if (mReading) {
        int aReanimType = 0;
        SyncInt(aReanimType);
        if (aReanimType == ReanimationType::REANIM_NONE) {
            theDefinition = nullptr;
        } else if (aReanimType >= 0 && aReanimType < ReanimationType::EXTENDED_NUM_REANIMS) {
            ReanimatorEnsureDefinitionLoaded(ReanimationType(aReanimType), true);
            theDefinition = &gReanimatorDefArray[aReanimType];
        } else {
            mFailed = true;
        }
    } else {
        int aReanimType = ReanimationType::REANIM_NONE;
        for (int i = 0; i < ReanimationType::EXTENDED_NUM_REANIMS; ++i) {
            ReanimatorDefinition *aDef = &gReanimatorDefArray[i];
            if (theDefinition == aDef) {
                aReanimType = i;
                break;
            }
        }
        SyncInt(aReanimType);
    }
}

void SyncReanimation(Board *theBoard, Reanimation *theReanimation, SaveGameContext &theContext) {
    theContext.SyncReanimationDef(theReanimation->mDefinition);

    if (theContext.mReading) {
        theReanimation->mReanimationHolder = theBoard->mApp->mEffectSystem->mReanimationHolder;
    }

    const int aTrackCount = theReanimation->mDefinition->mTrackCount;
    if (aTrackCount == 0) {
        return;
    }

    const std::size_t aTrackInstancesSize = static_cast<std::size_t>(aTrackCount) * sizeof(ReanimatorTrackInstance);

    const std::size_t aTransformsSize = static_cast<std::size_t>(aTrackCount) * sizeof(ReanimatorTransform);

    const std::size_t aAllocationSize = aTrackInstancesSize + aTransformsSize;

    if (theContext.mReading) {
        auto *aMemory = static_cast<unsigned char *>(FindGlobalAllocator(aAllocationSize)->Calloc(aAllocationSize));

        HOMURA_ASSERT(aMemory != nullptr);

        theReanimation->mTrackInstances = reinterpret_cast<ReanimatorTrackInstance *>(aMemory);

        theReanimation->mReanimatorTransforms = reinterpret_cast<ReanimatorTransform *>(aMemory + aTrackInstancesSize);

        theReanimation->unk2[0] = 7;

        theReanimation->SetAnimRate(theReanimation->mAnimRate);
    }

    // 原版存档只同步 TrackInstance，不同步后面的 ReanimatorTransform。
    theContext.SyncBytes(theReanimation->mTrackInstances, static_cast<int>(aTrackInstancesSize));

    for (int aTrackIndex = 0; aTrackIndex < aTrackCount; ++aTrackIndex) {
        ReanimatorTrackInstance &aTrackInstance = theReanimation->mTrackInstances[aTrackIndex];

        theContext.SyncImage(aTrackInstance.mImageOverride);

        if (theContext.mReading) {
            aTrackInstance.mBlendTransform.mText = "";

            HOMURA_ASSERT(aTrackInstance.mBlendTransform.mFont == nullptr);
            HOMURA_ASSERT(aTrackInstance.mBlendTransform.mImage == nullptr);
        } else {
            HOMURA_ASSERT(aTrackInstance.mBlendTransform.mText[0] == '\0');
            HOMURA_ASSERT(aTrackInstance.mBlendTransform.mFont == nullptr);
            HOMURA_ASSERT(aTrackInstance.mBlendTransform.mImage == nullptr);
        }
    }
}