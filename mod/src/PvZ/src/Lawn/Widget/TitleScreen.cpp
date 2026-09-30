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

#include "PvZ/Lawn/Widget/TitleScreen.h"
#include "Homura/Logger.h"
#include "PvZ/Android/IntroVideo.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/Music.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodCommon.h"
#include "PvZ/TodLib/Common/TodStringFile.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

namespace {

constexpr uintptr_t kTitleScreenButtonListenerVtableOffset = 0x1F8;

} // namespace

void TitleScreen::Draw(Sexy::Graphics *graphics) {
    old_TitleScreen_Draw(this, graphics);
    // LOGD("%d",Sexy::IMAGE_ESRB_RATING);
    // if (Sexy::IMAGE_ESRB_RATING)
    // DrawImage(graphics,Sexy::IMAGE_ESRB_RATING,0,0);
    // LOGD("draw");
    // int *q = nullptr;
    // q[1] = 1;
}

void TitleScreen::Update() {
    Sexy::Widget::Update();

    if (mApp->mShutDown) {
        return;
    }

    if (AGVideoConsumeCompleted()) {
        VideoCompleted();
    }

    // Switch port: the intro video is drawn over the game's picture before each
    // present (Android shows it on a surface of its own), so the screen keeps
    // being drawn while it plays.
    MarkDirty();

    if (!mNeedRegister) {
        return;
    }

    if (mTitleState == WaitingForFirstDraw) {
        mApp->StartLoadingThread();

        // mNeedPlayLogo 只控制 Logo。
        // needVideo 只控制视频。
        const bool shouldPlayVideo = playVideo && !Sexy::GetEnvOption("LAWN_NO_INTRO_VIDEO", false);

        // Logo 和视频只要有一个需要显示，就进入该阶段。
        if (mNeedPlayLogo || shouldPlayVideo) {
            // 不播放 Logo、只播放视频时，不需要额外等待 Logo 的 500 帧。
            SwitchState(PopcapLogo_OR_IntroVideo, mNeedPlayLogo ? 500 : 0);

            mIntroVideoAge = 0;
            mVideoCompleted = false;
            mIsPlayingIntroVideo = false;
            AGVideoResetCompleted();

            // 视频播放不再位于 if (mNeedPlayLogo) 的控制之下。
            if (shouldPlayVideo) {
                constexpr const char *kIntroVideoPath = "movies/intro.mp4";

                AGVideoEnable(true);
                AGVideoShow(true);

                const bool opened = AGVideoOpen(kIntroVideoPath) == 0;
                const bool acceptedForPlayback = opened && AGVideoPlay();

                mIsPlayingIntroVideo = acceptedForPlayback;

                if (!acceptedForPlayback) {
                    AGVideoClose();
                    AGVideoShow(false);
                    AGVideoEnable(false);
                }
            }
        } else {
            // Logo 和视频都不需要，直接进入加载界面。
            SwitchState(Loading, 100);
        }
    }

    ++mTitleAge;

    if (mLoadingThreadComplete) {
        mStartButton->mTextDrawMode = 2;
    }

    if (mTitleStateCounter > 0) {
        --mTitleStateCounter;
    }

    if (mTitleState == ESRBLogo) {
        if (mTitleStateCounter != 0) {
            return;
        }

        SwitchState(Loading, 100);
        return;
    }

    if (mTitleState == PopcapLogo_OR_IntroVideo) {
        if (mIsPlayingIntroVideo) {
            ++mIntroVideoAge;

            if (mIntroVideoAge <= 2999 && !mVideoCompleted) {
                return;
            }

            AGVideoClose();
            AGVideoShow(false);
            AGVideoEnable(false);
            mIsPlayingIntroVideo = false;

            if (mTitleAge <= 99) {
                mTitleAge += 500;
            }
        } else if (mTitleStateCounter != 0) {
            return;
        }

        if (mGuide) {
            SwitchState(GuideLogo, 400);
            return;
        }

        SwitchState(Loading, 100);
        return;
    }

    if (mTitleState == GuideLogo && mTitleStateCounter == 0) {
        // 原版会在同一帧继续执行下面的加载界面更新。
        SwitchState(Loading, 100);
    }

    if (!mLoaderScreenIsLoaded) {
        return;
    }

    if (!mMusicInitFinished) {
        mApp->mMusic->MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME);
        mMusicInitFinished = true;
    }

    const auto currentProgress = float(mApp->GetLoadingThreadProgress());

    if (mNeedToInit && mTitleState == Loading) {
        mNeedToInit = false;
        *mStartButton->mLabel = TodStringTranslate("[LOADING]");
        mStartButton->SetFont(Sexy::FONT_DWARVENTODCRAFT24);
        mStartButton->Resize(182, 800, int(mTotalBarWidth), 64);
        mStartButton->mVisible = true;
        float estimatedTotalLoadTime;
        if (currentProgress > 0.000001f) {
            estimatedTotalLoadTime = float(mTitleAge) / currentProgress;
        } else {
            estimatedTotalLoadTime = 3000.0f;
        }
        const float remainingLoadTime = ClampFloat(estimatedTotalLoadTime * (1.0f - currentProgress), 100.0f, 3000.0f);
        mBarStartProgress = std::min(currentProgress, 0.9f);
        mBarVel = mTotalBarWidth / remainingLoadTime;
    }

    const int buttonY = mTitleStateCounter <= 10 ? TodAnimateCurve(10, 0, mTitleStateCounter, 500, 529, CURVE_BOUNCE) : TodAnimateCurve(120, 10, mTitleStateCounter, 800, 500, CURVE_EASE_IN);
    mStartButton->Resize(mStartButton->mX, buttonY, int(mTotalBarWidth), mStartButton->mHeight);
    if (mTitleStateCounter > 0) {
        return;
    }
    mApp->mEffectSystem->Update();
    const float previousBarWidth = mCurBarWidth;
    const float totalBarWidth = mTotalBarWidth;
    mCurBarWidth += mBarVel;

    if (mLoadingThreadComplete) {
        if (mCurBarWidth > totalBarWidth) {
            const pvzstl::string clickToStart = TodStringTranslate("[CLICK_TO_START]");
            const pvzstl::string okButton = TodStringTranslate("[OK_BUTTON]");
            *mStartButton->mLabel = TodReplaceString(clickToStart, "<A>", okButton);
            mCurBarWidth = totalBarWidth;
        }
    } else if (mCurBarWidth > totalBarWidth * 0.99f) {
        mCurBarWidth = totalBarWidth * 0.99f;
    }

    const float normalizedProgress = (currentProgress - mBarStartProgress) / (1.0f - mBarStartProgress);

    if (normalizedProgress > mPrevLoadingPercent + 0.01f || mLoadingThreadComplete) {
        const float desiredBarWidth = TodAnimateCurveFloatTime(0.0f, 1.0f, normalizedProgress, 0.0f, totalBarWidth, CURVE_EASE_IN);
        const float barError = desiredBarWidth - mCurBarWidth;
        const float acceleration = mLoadingThreadComplete ? 0.0001f : TodAnimateCurveFloatTime(0.0f, 1.0f, normalizedProgress, 0.0001f, 0.00001f, CURVE_LINEAR);
        // 原始代码先把误差转换成 int，然后再取绝对值。
        const int integerError = std::abs(int(barError));
        mBarVel += float(integerError) * barError * acceleration;
        const float normalMinVelocity = TodAnimateCurveFloatTime(0.0f, 1.0f, normalizedProgress, 0.2f, 0.01f, CURVE_LINEAR);
        const float minVelocity = mApp->mTodCheatKeys ? 0.0f : normalMinVelocity;
        const float maxVelocity = mApp->mTodCheatKeys ? 100.0f : 2.0f;
        mBarVel = std::clamp(mBarVel, minVelocity, maxVelocity);
        mPrevLoadingPercent = normalizedProgress;
    }
    if (!mLoadingThreadComplete && mApp->mLoadingThreadCompleted) {
        mLoadingThreadComplete = true;
        // 对应伪代码的虚表槽 48(false) 与 42(true)。
        mStartButton->SetDisabled(false);
        mStartButton->SetVisible(true);
    }


    const float milestones[5] = {
        totalBarWidth * 0.11f,
        totalBarWidth * 0.32f,
        totalBarWidth * 0.54f,
        totalBarWidth * 0.72f,
        totalBarWidth * 0.95f,
    };

    for (int milestoneIndex = 0; milestoneIndex < 5; ++milestoneIndex) {
        const float milestone = milestones[milestoneIndex];
        // 本帧必须刚好跨过该位置：
        // previousBarWidth < milestone <= mCurBarWidth
        if (previousBarWidth >= milestone || milestone > mCurBarWidth) {
            continue;
        }
        const bool isZombieMilestone = milestoneIndex == 4;
        Reanimation *reanimation = mApp->AddReanimation(milestone + 140.0f, 481.0f, 0, ReanimationType(isZombieMilestone));
        reanimation->mLoopType = REANIM_PLAY_ONCE_AND_HOLD;
        reanimation->SetAnimRate(18.0f);
        if (milestoneIndex == 1 || milestoneIndex == 3) {
            reanimation->OverrideScale(-1.0f, 1.0f);
        } else if (milestoneIndex == 2) {
            reanimation->SetPosition(milestone + 140.0f, 476.0f);
            reanimation->OverrideScale(1.1f, 1.3f);
        } else if (isZombieMilestone) {
            reanimation->SetPosition(milestone + 120.0f, 481.0f);
            mApp->GetVTable()->PlaySampleWithLoopFlag(mApp, Sexy::SOUND_LOADINGBAR_FLOWER, true);
        }
        mApp->GetVTable()->PlaySampleWithLoopFlag(mApp, isZombieMilestone ? Sexy::SOUND_LOADINGBAR_ZOMBIE : Sexy::SOUND_LOADINGBAR_FLOWER, true);
    }
}


void TitleScreen::SwitchState(TitleState state, int duration) {
    mTitleState = state;
    mTitleStateDuration = duration;
    mTitleStateCounter = duration;
}

void TitleScreen::_constructor(LawnApp *theApp) {
    old_TitleScreen_TitleScreen(this, theApp);
    if (jumpLogo) {
        mNeedPlayLogo = false;
    }
}
void TitleScreen::_destructor() {
    Widget::vTable = reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(vTableForTitleScreenAddr) + 8);
    ButtonListener::vTable = reinterpret_cast<const Sexy::ButtonListener::VTable *>(reinterpret_cast<uintptr_t>(Widget::vTable) + kTitleScreenButtonListenerVtableOffset);
    delete mStartButton;
    if (mPopcapLogo) {
        mPopcapLogo->GetVTable()->deletingDestructor(mPopcapLogo);
    }
    if (mGuide) {
        mGuide->GetVTable()->deletingDestructor(mGuide);
    }
    mApp->mResourceManager->ReplaceImage("IMAGE_TITLESCREEN", nullptr);
    mApp->mResourceManager->ReplaceImage("IMAGE_TITLESCREEN_BALL", nullptr);
    mApp->mResourceManager->ReplaceImage("IMAGE_TITLESCREEN_GLOW", nullptr);
    mApp->mResourceManager->ReplaceImage("IMAGE_LOADBAR_DIRT", nullptr);
    mApp->mResourceManager->ReplaceImage("IMAGE_LOADBAR_GRASS", nullptr);
    mApp->mResourceManager->ReplaceImage("IMAGE_ESRB_RATING", nullptr);
    if (mIsPlayingIntroVideo) {
        AGVideoClose();
        AGVideoShow(false);
        AGVideoEnable(false);
        mIsPlayingIntroVideo = false;
    }
    ButtonListener::vTable = reinterpret_cast<const Sexy::ButtonListener::VTable *>(reinterpret_cast<uintptr_t>(vTableForSexyButtonListenerAddr) + 8);
    Sexy::Widget::_destructor();
}

void TitleScreen::VideoCompleted() {
    mVideoCompleted = true;
}

void TitleScreen::ButtonPress(int theId) {
    mApp->GetVTable()->PlaySampleWithLoopFlag(mApp, Sexy::SOUND_BUTTONCLICK, true);
    if (mLoadingThreadComplete && mCurBarWidth == mTotalBarWidth) {
        if (mStartButton->mGamepadIndex > 0) {
            mApp->SwapGamepadId(0, mStartButton->mGamepadIndex);
        }
        mApp->LoadingCompleted();
    }
}