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

#include "PvZ/Lawn/Widget/AchievementsWidget.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <sys/time.h>

#include <cmath>

#include <algorithm>

using namespace Sexy;

namespace {
long NowMs() {
    timeval tp{};
    gettimeofday(&tp, nullptr);
    return tp.tv_sec * 1000L + tp.tv_usec / 1000L;
}

int GetMinScrollY() {
    return 720 + MAIN_MENU_HEIGHT - (ACHIEVEMENT_HOLE_LENGTH + 1) * addonImages.hole->mHeight;
}

int GetMaxScrollY() {
    return MAIN_MENU_HEIGHT;
}

int ClampScrollY(int y) {
    return std::clamp(y, GetMinScrollY(), GetMaxScrollY());
}
} // namespace

AchievementsWidget::AchievementsWidget(LawnApp *theApp) {
    new (this) MaskHelpWidget{theApp};
    mApp = theApp;
    mDragStartPointerScreenY = 0;
    mDragStartWidgetY = 0;
    mLastPointerScreenY = 0;
    mLastSampleTimeMs = 0;
    mVelocityPxPerSec = 0.0f;
    mAccY = 0.0f;
    mIsDragging = false;
    mIsScrolling = false;
}

void AchievementsWidget::_destructor() {
    Widget::_destructor();
}

void AchievementsWidget::Draw(Graphics *g) const {
    int theY = 0;
    int theDiffY = addonImages.hole->mHeight;
    for (int i = 0; i < ACHIEVEMENT_HOLE_LENGTH; i++) {
        if (i == ACHIEVEMENT_HOLE_WORM_POS) {
            g->DrawImage(addonImages.hole_worm, 0, theY);
        } else if (i == ACHIEVEMENT_HOLE_GEMS_POS) {
            g->DrawImage(addonImages.hole_gems, 0, theY);
        } else if (i == ACHIEVEMENT_HOLE_CHUZZLE_POS) {
            g->DrawImage(addonImages.hole_chuzzle, 0, theY);
        } else if (i == ACHIEVEMENT_HOLE_BJORN_POS) {
            g->DrawImage(addonImages.hole_bjorn, 0, theY);
        } else if (i == ACHIEVEMENT_HOLE_PIPE_POS) {
            g->DrawImage(addonImages.hole_pipe, 0, theY);
        } else if (i == ACHIEVEMENT_HOLE_TIKI_POS) {
            g->DrawImage(addonImages.hole_tiki, 0, theY);
        } else if (i == ACHIEVEMENT_HOLE_HEAVY_ROCKS_POS) {
            g->DrawImage(addonImages.hole_heavyrocks, 0, theY);
        } else if (i == ACHIEVEMENT_HOLE_DU_WEI_POS) {
            g->DrawImage(addonImages.hole_duwei, 0, theY);
        } else {
            g->DrawImage(addonImages.hole, 0, theY);
        }
        theY += theDiffY;
    }
    g->DrawImage(addonImages.hole_china, 0, theY);
    g->DrawImage(addonImages.hole_top, 0, 0);
    int theAchievementY = 300;
    for (int i = 0; i < AchievementType::NUM_ACHIEVEMENT_TYPES; ++i) {
        if (!mApp->mPlayerInfo->mAchievements[i]) {
            g->SetColorizeImages(true);
            g->SetColor(gColorGray);
        }
        g->DrawImage(GetIconByAchievementId((AchievementType)i), 330, theAchievementY - 5);
        const char *theAchievementName = GetNameByAchievementId((AchievementType)i);
        pvzstl::string str = StrFormat("[%s]", theAchievementName);
        pvzstl::string str1 = StrFormat("[%s_TEXT]", theAchievementName);
        Color theColor = {0, 255, 0, 255};
        Color theColor1 = {255, 255, 255, 255};
        Rect rect = {460, theAchievementY + 60, 540, 0};
        TodDrawString(g, str, 460, theAchievementY + 40, FONT_HOUSEOFTERROR28, theColor, DrawStringJustification::DS_ALIGN_LEFT);
        if (i == AchievementType::ACHIEVEMENT_SHOP) {
            str = TodReplaceNumberString(str1, "{coin}", mApp->mPlayerInfo->mUsedCoins * 10);
            TodDrawStringWrapped(g, str, rect, FONT_HOUSEOFTERROR20, theColor1, DrawStringJustification::DS_ALIGN_LEFT, false);
        } else {
            TodDrawStringWrapped(g, str1, rect, FONT_HOUSEOFTERROR20, theColor1, DrawStringJustification::DS_ALIGN_LEFT, false);
        }
        g->SetColorizeImages(false);
        theAchievementY += theDiffY * 2 / 3;
    }
    int theAccomplishedNum = 0;
    for (int i = 0; i < AchievementType::NUM_ACHIEVEMENT_TYPES; ++i) {
        if (mApp->mPlayerInfo->mAchievements[i]) {
            theAccomplishedNum++;
        }
    }
    pvzstl::string str = StrFormat("%d/%d", theAccomplishedNum, AchievementType::NUM_ACHIEVEMENT_TYPES);
    Color theColor = {255, 240, 0, 255};
    TodDrawString(g, str, 1060, 173, FONT_DWARVENTODCRAFT18, theColor, DrawStringJustification::DS_ALIGN_CENTER);
}

void AchievementsWidget::MouseDown(int x, int y, int theClickCount) {
    if (gAchievementState != SHOWING)
        return;
    mIsDragging = true;
    mIsScrolling = false;
    mVelocityPxPerSec = 0.0f;
    mDragStartPointerScreenY = mY + y;
    mDragStartWidgetY = mY;
    mLastPointerScreenY = mDragStartPointerScreenY;
    mAccY = static_cast<float>(mY);
    mLastSampleTimeMs = NowMs();
}

void AchievementsWidget::MouseDrag(int x, int y) {
    if (gAchievementState != SHOWING || !mIsDragging)
        return;
    const int pointerScreenY = mY + y;
    const int totalDrag = pointerScreenY - mDragStartPointerScreenY;
    const int theNewY = ClampScrollY(mDragStartWidgetY + totalDrag);
    Move(mX, theNewY);
    mAccY = static_cast<float>(theNewY);

    const long nowMs = NowMs();
    const long dtMs = nowMs - mLastSampleTimeMs;
    if (dtMs > 0) {
        const float sampleVelocity = static_cast<float>(pointerScreenY - mLastPointerScreenY) * 1000.0f / static_cast<float>(dtMs);
        mVelocityPxPerSec = mVelocityPxPerSec * 0.75f + sampleVelocity * 0.25f;
    }
    mLastPointerScreenY = pointerScreenY;
    mLastSampleTimeMs = nowMs;
}

void AchievementsWidget::MouseUp(int x, int y) {
    if (!mIsDragging)
        return;
    mIsDragging = false;
    const int pointerScreenY = mY + y;
    const long nowMs = NowMs();
    const long dtMs = nowMs - mLastSampleTimeMs;
    if (dtMs > 0) {
        const float releaseVelocity = static_cast<float>(pointerScreenY - mLastPointerScreenY) * 1000.0f / static_cast<float>(dtMs);
        mVelocityPxPerSec = mVelocityPxPerSec * 0.6f + releaseVelocity * 0.4f;
    }

    constexpr float kMinFlingSpeedPxPerSec = 90.0f;
    constexpr float kMaxFlingSpeedPxPerSec = 4800.0f;
    mVelocityPxPerSec = std::clamp(mVelocityPxPerSec, -kMaxFlingSpeedPxPerSec, kMaxFlingSpeedPxPerSec);

    if (std::fabs(mVelocityPxPerSec) >= kMinFlingSpeedPxPerSec) {
        mIsScrolling = true;
        mAccY = static_cast<float>(mY);
    } else {
        mIsScrolling = false;
        mVelocityPxPerSec = 0.0f;
    }
}

void AchievementsWidget::Update() {
    if (mIsScrolling && !mIsDragging) {
        constexpr float kFrameDtSec = 1.0f / 60.0f;
        constexpr float kDecelerationPxPerSec2 = 3400.0f;
        const float decel = kDecelerationPxPerSec2 * kFrameDtSec;
        if (mVelocityPxPerSec > 0.0f) {
            mVelocityPxPerSec = std::max(0.0f, mVelocityPxPerSec - decel);
        } else if (mVelocityPxPerSec < 0.0f) {
            mVelocityPxPerSec = std::min(0.0f, mVelocityPxPerSec + decel);
        }

        mAccY += mVelocityPxPerSec * kFrameDtSec;
        int theNewY = ClampScrollY(static_cast<int>(std::lround(mAccY)));
        mAccY = static_cast<float>(theNewY);
        Move(mX, theNewY);

        if ((theNewY == GetMinScrollY() && mVelocityPxPerSec < 0.0f) || (theNewY == GetMaxScrollY() && mVelocityPxPerSec > 0.0f)) {
            mIsScrolling = false;
            mVelocityPxPerSec = 0.0f;
        } else if (std::fabs(mVelocityPxPerSec) < 10.0f) {
            mIsScrolling = false;
            mVelocityPxPerSec = 0.0f;
        }
    }
    MarkDirty();
}
