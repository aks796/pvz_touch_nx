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

#include "PvZ/Lawn/SwitchPad.h"

#include <algorithm>
#include <vector>
#include "PvZ/Lawn/Widget/LeaderboardsWidget.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/TrashBin.h"
#include "PvZ/TodLib/Common/TodStringFile.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

using namespace Sexy;

static float gLeaderboardAchievementsPosition[12][2] = {
    {198, 496},
    {210, 492},
    {222, 306},
    {405, 501},
    {368, 500},
    {690, 478},
    {615, 487},
    {524, 326},
    {756, 373},
    {678, 390},
    {791, 278},
    {430, 362},
};

static Sexy::Rect gLeaderboardAchievementsRect[12][2] = {
    {{253, 594, 485, 62}, {833, 528, 198, 68}},
    {{209, 488, 91, 106}, {0, 0, 0, 0}},
    {{269, 330, 185, 122}, {298, 453, 91, 91}},
    {{452, 550, 151, 76}, {446, 512, 68, 37}},
    {{373, 504, 83, 34}, {389, 533, 63, 37}},
    {{744, 516, 86, 97}, {0, 0, 0, 0}},
    {{632, 490, 52, 40}, {636, 530, 60, 33}},
    {{561, 340, 77, 196}, {525, 404, 145, 56}},
    {{880, 384, 78, 82}, {891, 467, 26, 68}},
    {{715, 416, 104, 81}, {765, 384, 54, 35}},
    {{817, 332, 113, 171}, {850, 298, 46, 31}},
    {{456, 362, 43, 122}, {461, 484, 71, 30}},
};

// Switch port: the house with the controller. A highlight goes between the
// zombie pile, the decorations earned on the house, the plant bin and Close,
// by where they are (D-pad / stick); A does what a tap does. The decoration
// under it shows as the game shows one tapped (flashing, named).
namespace {
enum HouseSpotKind { HOUSE_ZOMBIE_PILE, HOUSE_ACHIEVEMENT, HOUSE_PLANT_BIN, HOUSE_CLOSE };

struct HouseSpot {
    Sexy::Rect rect;
    HouseSpotKind kind;
    int index; // the decoration's number
};

struct {
    bool active = false;
    HouseSpotKind kind = HOUSE_ZOMBIE_PILE;
    int index = 0;
    int tick = 0;
} gHousePad;

void CollectHouseSpots(LeaderboardsWidget &w, std::vector<HouseSpot> &v);
int FindHouseSpot(const std::vector<HouseSpot> &v);
} // namespace

int GameStats::ChangeMiscStat(MiscStat theMiscStat, int theChangeIndex) {
    return mMiscStats[theMiscStat] + theChangeIndex;
}

int LeaderboardsWidget_GetAchievementIdByReanimationType(ReanimationType type) {
    AchievementType id = AchievementType::ACHIEVEMENT_HOME_SECURITY;
    switch (type) {
        case ReanimationType::REANIM_ACHIEVEMENT_HOME_SECURITY:
            id = AchievementType::ACHIEVEMENT_HOME_SECURITY;
            break;
        case ReanimationType::REANIM_ACHIEVEMENT_MORTICULTURALIST:
            id = AchievementType::ACHIEVEMENT_MORTICULTURALIST;
            break;
        case ReanimationType::REANIM_ACHIEVEMENT_SMARTY_BRANCHES:
            id = AchievementType::ACHIEVEMENT_TREE;
            break;
        case ReanimationType::REANIM_ACHIEVEMENT_CRASH_OF_THE_TITAN:
            id = AchievementType::ACHIEVEMENT_GARG;
            break;
        case ReanimationType::REANIM_ACHIEVEMENT_ZFFS_4_EVR:
            id = AchievementType::ACHIEVEMENT_COOP;
            break;
        case ReanimationType::REANIM_ACHIEVEMENT_ALIVE_AND_PLANTING:
            id = AchievementType::ACHIEVEMENT_IMMORTAL;
            break;
        case ReanimationType::REANIM_ACHIEVEMENT_VERSUS:
            id = AchievementType::ACHIEVEMENT_VERSUS;
            break;
        case ReanimationType::REANIM_ACHIEVEMENT_SOIL_YOUR_PLANTS:
            id = AchievementType::ACHIEVEMENT_SOILPLANTS;
            break;
        case ReanimationType::REANIM_ACHIEVEMENT_EXPLODONATOR:
            id = AchievementType::ACHIEVEMENT_EXPLODONATOR;
            break;
        case ReanimationType::REANIM_ACHIEVEMENT_CLOSE_SHAVE:
            id = AchievementType::ACHIEVEMENT_CLOSESHAVE;
            break;
        case ReanimationType::REANIM_ACHIEVEMENT_SHOP:
            id = AchievementType::ACHIEVEMENT_SHOP;
            break;
        case ReanimationType::REANIM_ACHIEVEMENT_NOM_NOM_NOM:
            id = AchievementType::ACHIEVEMENT_CHOMP;
            break;
        default:
            break;
    }
    return id - AchievementType::ACHIEVEMENT_HOME_SECURITY;
}

int LeaderboardsWidget_GetAchievementIdByDrawOrder(int drawOrder) {
    ReanimationType type = ReanimationType::REANIM_ACHIEVEMENT_HOME_SECURITY;
    switch (drawOrder) {
        case 0:
            type = ReanimationType::REANIM_ACHIEVEMENT_CLOSE_SHAVE;
            break;
        case 1:
            type = ReanimationType::REANIM_ACHIEVEMENT_SHOP;
            break;
        case 2:
            type = ReanimationType::REANIM_ACHIEVEMENT_EXPLODONATOR;
            break;
        case 3:
            type = ReanimationType::REANIM_ACHIEVEMENT_ALIVE_AND_PLANTING;
            break;
        case 4:
            type = ReanimationType::REANIM_ACHIEVEMENT_SMARTY_BRANCHES;
            break;
        case 5:
            type = ReanimationType::REANIM_ACHIEVEMENT_NOM_NOM_NOM;
            break;
        case 6:
            type = ReanimationType::REANIM_ACHIEVEMENT_SOIL_YOUR_PLANTS;
            break;
        case 7:
            type = ReanimationType::REANIM_ACHIEVEMENT_VERSUS;
            break;
        case 8:
            type = ReanimationType::REANIM_ACHIEVEMENT_ZFFS_4_EVR;
            break;
        case 9:
            type = ReanimationType::REANIM_ACHIEVEMENT_CRASH_OF_THE_TITAN;
            break;
        case 10:
            type = ReanimationType::REANIM_ACHIEVEMENT_MORTICULTURALIST;
            break;
        case 11:
            type = ReanimationType::REANIM_ACHIEVEMENT_HOME_SECURITY;
            break;
    }
    return type - ReanimationType::REANIM_ACHIEVEMENT_HOME_SECURITY;
}

LeaderboardsWidget::LeaderboardsWidget(LawnApp *theApp) {
    new (this) DaveHelp{theApp};
    Resize(-240, -60, 1280, 720);
    mLeaderboardReanimations = (LeaderboardReanimations *)operator new(sizeof(LeaderboardReanimations));
    for (int i = 0; i < 5; ++i) {
        // Reanimation *reanim = (Reanimation *)operator new(sizeof(Reanimation));
        // Reanimation_Reanimation(reanim);
        Reanimation *reanim = new Reanimation;
        reanim->ReanimationInitializeType(0.0, 0.0, (ReanimationType)(ReanimationType::REANIM_LEADERBOARDS_HOUSE + i));
        reanim->SetAnimRate(0.0f);
        reanim->mLoopType = ReanimLoopType::REANIM_LOOP;
        if (i == 0) {
            mApp->SetHouseReanim(reanim);
            reanim->SetPosition(456.9f, 129.3f);
        } else if (i == 1 || i == 2 || i == 3) {
            reanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 12.0f);
        } else if (i == 4) {
            reanim->PlayReanim("anim_float", ReanimLoopType::REANIM_LOOP, 0, 2.0f); // 云儿飘得慢一些
        }
        reanim->Update(); // 一次Update是必要的，否则绘制出来是Empty
        mLeaderboardReanimations->backgroundReanim[i] = reanim;
    }
    mLeaderboardReanimations->backgroundReanim[1]->AssignRenderGroupToTrack("survival button 1", 1);                   // 设置无尽模式按钮
    mLeaderboardReanimations->backgroundReanim[1]->SetImageOverride("survival button 1", addonImages.survival_button); // 设置无尽模式按钮
    mLeaderboardReanimations->backgroundReanim[1]->HideTrack("house 1", true);                                         // 隐藏默认房屋
    mLeaderboardReanimations->backgroundReanim[1]->HideTrack("house achievements 1", true);                            // 隐藏默认房屋
    // Reanimation_HideTrack(this_->mLeaderboardReanimations->backgroundReanim[1],"house 2",true); // 隐藏默认房屋
    // Reanimation_HideTrack(this_->mLeaderboardReanimations->backgroundReanim[1],"house achievements 2",true); // 隐藏默认房屋

    int zombieTrackIndex = mLeaderboardReanimations->backgroundReanim[0]->FindTrackIndex("zombie_trash");
    SexyTransform2D zombieSexyTransform2D;
    mLeaderboardReanimations->backgroundReanim[0]->GetTrackMatrix(zombieTrackIndex, zombieSexyTransform2D);
    mZombieTrashBin = new TrashBin(TrashBin::ZOMBIE_PILE, theApp->mPlayerInfo->mGameStats.mMiscStats[GameStats::ZOMBIES_KILLED] / 125.0f);
    mZombieTrashBin->Move(zombieSexyTransform2D.m[0][2], zombieSexyTransform2D.m[1][2]);

    int plantTrackIndex = mLeaderboardReanimations->backgroundReanim[0]->FindTrackIndex("plant_trash");
    SexyTransform2D plantSexyTransform2D;
    mLeaderboardReanimations->backgroundReanim[0]->GetTrackMatrix(plantTrackIndex, plantSexyTransform2D);
    mPlantTrashBin = new TrashBin(TrashBin::PLANT_PILE, theApp->mPlayerInfo->mGameStats.mMiscStats[GameStats::PLANTS_KILLED] / 125.0f);
    mPlantTrashBin->Move(plantSexyTransform2D.m[0][2], plantSexyTransform2D.m[1][2]);

    for (int i = 0; i < AchievementType::NUM_ACHIEVEMENT_TYPES; ++i) {
        mAchievements[i] = theApp->mPlayerInfo->mAchievements[LeaderboardsWidget_GetAchievementIdByReanimationType((ReanimationType)(ReanimationType::REANIM_ACHIEVEMENT_HOME_SECURITY + i))];
        // Reanimation *reanim = (Reanimation *)operator new(sizeof(Reanimation));
        // Reanimation_Reanimation(reanim);
        Reanimation *reanim = new Reanimation;
        reanim->ReanimationInitializeType(0.0, 0.0, (ReanimationType)(ReanimationType::REANIM_ACHIEVEMENT_HOME_SECURITY + i));
        reanim->SetPosition(gLeaderboardAchievementsPosition[i][0], gLeaderboardAchievementsPosition[i][1]);
        reanim->mLoopType = ReanimLoopType::REANIM_LOOP;
        reanim->Update(); // 一次Update是必要的，否则绘制出来是Empty
        mLeaderboardReanimations->achievementReanim[i] = reanim;
    }

    mLongestRecordPool = theApp->mPlayerInfo->mChallengeRecords[GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_3 - 2];
    // this_->mLongestRecordPool = theApp->mPlayerInfo->mGameStats.mMiscStats[GameStats::ENDLESS_FLAGS];

    mBackButton = MakeButton(1000, mButtonListener, this, switchpad::Label("[SWITCH_CLOSE]", "[CLOSE]")); // Switch port: B closes it too
    mBackButton->Resize(1040, 590, 120, 50);

    mFocusedAchievementIndex = 0;
    mHighLightAchievement = false;

    mApp->TryHelpTextScreen(HelpTextPage::HELP_TEXT_PAGE_HOUSE);
}

void LeaderboardsWidget::_destructor() {
    delete mZombieTrashBin;
    delete mPlantTrashBin;
    for (auto &i : mLeaderboardReanimations->backgroundReanim) {
        delete i;
    }
    for (auto &i : mLeaderboardReanimations->achievementReanim) {
        delete i;
    }
    delete mBackButton;
    delete mLeaderboardReanimations;

    Widget::_constructor();
}

void LeaderboardsWidget::AddedToManager(Sexy::WidgetManager *theWidgetManager) {
    Widget::AddedToManager(theWidgetManager);
    AddWidget(mBackButton);
}

void LeaderboardsWidget::RemovedFromManager(Sexy::WidgetManager *theWidgetManager) {
    Widget::RemovedFromManager(theWidgetManager);
    RemoveWidget(mBackButton);
}

void LeaderboardsWidget::ButtonDepress(this LeaderboardsWidget &self, int id) {
    if (id == 1000) {
        LawnApp *lawnApp = gLawnApp;
        lawnApp->KillLeaderboards();
        lawnApp->ShowMainMenuScreen();
    }
}

void LeaderboardsWidget::Update() {
    for (auto &reanim : mLeaderboardReanimations->backgroundReanim) {
        reanim->Update();
    }

    for (int i = 0; i < AchievementType::NUM_ACHIEVEMENT_TYPES; ++i) {
        if (!mAchievements[i])
            continue;
        mLeaderboardReanimations->achievementReanim[i]->Update();
    }
    MarkDirty();
}

void LeaderboardsWidget::Draw(Sexy::Graphics *g) {
    for (int i = 4; i >= 0; i--) {
        mLeaderboardReanimations->backgroundReanim[i]->DrawRenderGroup(g, 0);
    }

    mPlantTrashBin->TrashBin::Draw(g);
    mZombieTrashBin->TrashBin::Draw(g);

    for (int i = 0; i < AchievementType::NUM_ACHIEVEMENT_TYPES; ++i) {
        int num = LeaderboardsWidget_GetAchievementIdByDrawOrder(i);
        if (!mAchievements[num])
            continue;
        if (mHighLightAchievement && num == mFocusedAchievementIndex) {
            auto id = AchievementType(LeaderboardsWidget_GetAchievementIdByReanimationType(ReanimationType(num + ReanimationType::REANIM_ACHIEVEMENT_HOME_SECURITY))
                                      + AchievementType::ACHIEVEMENT_HOME_SECURITY);
            Sexy::Image *image = GetIconByAchievementId(id);
            Color color = GetFlashingColor(mApp->mAppCounter, 120);
            g->SetColorizeImages(true);
            g->SetColor(color);
            mLeaderboardReanimations->achievementReanim[num]->Draw(g);
            g->SetColorizeImages(false);
            int offsetX = gLeaderboardAchievementsPosition[num][0] + 20;
            int offsetY = gLeaderboardAchievementsPosition[num][1] - 200;
            g->DrawImage(image, offsetX, offsetY);
            pvzstl::string str = StrFormat("[%s]", GetNameByAchievementId(id));
            Sexy::Rect rect = {offsetX - 42, offsetY + 125, 200, 200};
            Color theColor = {0, 255, 0, 255};
            TodDrawStringWrapped(g, str, rect, Sexy::FONT_HOUSEOFTERROR28, theColor, DrawStringJustification::DS_ALIGN_CENTER, false);
        } else {
            mLeaderboardReanimations->achievementReanim[num]->Draw(g);
        }
    }

    if (mApp->HasFinishedAdventure()) {
        mLeaderboardReanimations->backgroundReanim[1]->DrawRenderGroup(g, 1);
        pvzstl::string aStr = TodReplaceNumberString(TodStringTranslate("[LEADERBOARD_STREAK]"), "{STREAK}", mLongestRecordPool);
        Sexy::Rect aRect = {317, 658, 120, 50};
        Sexy::Font *aFont = Sexy::FONT_CONTINUUMBOLD14;
        TodDrawStringWrapped(g, aStr, aRect, aFont, gColorYellow, DrawStringJustification::DS_ALIGN_CENTER, false);
    }

    // DrawImage(g, addonImages.survival_button, 270, 579);

    // Switch port: the controller's highlight on a pile or on Close (a
    // decoration shows as tapped, above); Close's glow before the button
    if (gHousePad.active) {
        std::vector<HouseSpot> spots;
        CollectHouseSpots(*this, spots);
        const int at = FindHouseSpot(spots);
        if (at >= 0 && spots[at].kind == HOUSE_CLOSE) {
            switchpad::DrawStoneGlow(g, mBackButton->mX, mBackButton->mY, mBackButton->mWidth, mBackButton->mIsDown, ++gHousePad.tick);
        } else if (at >= 0 && spots[at].kind != HOUSE_ACHIEVEMENT) {
            switchpad::DrawWarmGlow(g, spots[at].rect, ++gHousePad.tick);
        }
    }

    Sexy::Rect aRect = {240, 70, 800, 70};
    pvzstl::string aStr = TodReplaceString(TodStringTranslate("[PLAYERS_HOUSE]"), "{PLAYER}", mApp->mPlayerInfo->mName);
    Sexy::Font *aFont = Sexy::FONT_HOUSEOFTERROR28;
    TodDrawStringWrapped(g, aStr, aRect, aFont, gColorWhite, DrawStringJustification::DS_ALIGN_CENTER, false);

    // int plantHeight = plantPileHeight * mPlantTrashBin->mPileNum;
    // int zombieHeight = zombiePileHeight * mZombieTrashBin->mPileNum;
    // Rect plantTrashBinRect = {mPlantTrashBin->mX,mPlantTrashBin->mY - plantHeight,addonImages.plant_can->mWidth,addonImages.plant_can->mHeight +
    // plantHeight}; Rect zombieTrashBinRect = {mZombieTrashBin->mX,mZombieTrashBin->mY -
    // zombieHeight,addonImages.zombie_can->mWidth,addonImages.zombie_can->mHeight + zombieHeight};
    //
    // SetColor(g, &yellow);
    // DrawRect(g, &plantTrashBinRect);
    //
    // SetColor(g, &green);
    // DrawRect(g, &zombieTrashBinRect);


    // SetColor(g, &yellow);
    // Rect rect = {xx,yy,xw,yh};
    // DrawRect(g, &rect);
    //
    // SetColor(g, &green);
    // Rect rect2 = {xx1,yy1,xw1,yh1};
    // DrawRect(g, &rect2);
    // if (LawnApp_EarnedGoldTrophy(mApp)) {
    // DrawImageCeliiii(g, Sexy::IMAGE_SUNFLOWER_TROPHY, 1110, 290, 1, 0);
    // } else if (LawnApp_HasFinishedAdventure(mApp)) {
    // DrawImageCeliiii(g, Sexy::IMAGE_SUNFLOWER_TROPHY, 1110, 290, 0, 0);
    // }
}

namespace {
Sexy::Rect PileRect(TrashBin *theBin, Sexy::Image *theCan, int thePileHeight) {
    const int height = thePileHeight * theBin->mPileNum;
    return {theBin->mX, theBin->mY - height, theCan->mWidth, theCan->mHeight + height};
}

void CollectHouseSpots(LeaderboardsWidget &w, std::vector<HouseSpot> &v) {
    v.clear();
    v.push_back({PileRect(w.mZombieTrashBin, addonImages.zombie_can, zombiePileHeight), HOUSE_ZOMBIE_PILE, 0});
    for (int num = 0; num < AchievementType::NUM_ACHIEVEMENT_TYPES; ++num) {
        if (w.mAchievements[num]) {
            v.push_back({gLeaderboardAchievementsRect[num][0], HOUSE_ACHIEVEMENT, num});
        }
    }
    v.push_back({PileRect(w.mPlantTrashBin, addonImages.plant_can, plantPileHeight), HOUSE_PLANT_BIN, 0});
    if (w.mBackButton != nullptr) {
        v.push_back({Sexy::Rect(w.mBackButton->mX, w.mBackButton->mY, w.mBackButton->mWidth, w.mBackButton->mHeight), HOUSE_CLOSE, 0});
    }
}

int FindHouseSpot(const std::vector<HouseSpot> &v) {
    for (int i = 0; i < static_cast<int>(v.size()); ++i) {
        if (v[i].kind == gHousePad.kind && (v[i].kind != HOUSE_ACHIEVEMENT || v[i].index == gHousePad.index)) {
            return i;
        }
    }
    return -1;
}

// the spot that way from `from`: the nearest, straight ahead beating off to the side
int StepHouseSpot(const std::vector<HouseSpot> &v, int from, int dx, int dy) {
    const Sexy::Rect &f = v[from].rect;
    const int x = f.mX + f.mWidth / 2, y = f.mY + f.mHeight / 2;
    int best = -1;
    long bestCost = 0;
    for (int i = 0; i < static_cast<int>(v.size()); ++i) {
        if (i == from) {
            continue;
        }
        const long ox = v[i].rect.mX + v[i].rect.mWidth / 2 - x, oy = v[i].rect.mY + v[i].rect.mHeight / 2 - y;
        const long ahead = dx != 0 ? ox * dx : oy * dy;
        const long aside = dx != 0 ? (oy < 0 ? -oy : oy) : (ox < 0 ? -ox : ox);
        if (ahead <= 4) {
            continue;
        }
        const long cost = ahead + aside * 2;
        if (best < 0 || cost < bestCost) {
            best = i;
            bestCost = cost;
        }
    }
    return best;
}

void ShowPileCount(LawnApp *theApp, bool thePlants) {
    theApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
    pvzstl::string str1 = TodStringTranslate(thePlants ? "[PLANTS_KILLED]" : "[ZOMBIES_KILLED]");
    pvzstl::string str2 = TodReplaceNumberString(str1, thePlants ? "{PLANTS}" : "{ZOMBIES}",
                                                 theApp->mPlayerInfo->mGameStats.mMiscStats[thePlants ? GameStats::PLANTS_KILLED : GameStats::ZOMBIES_KILLED]);
    theApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, str2.c_str(), "", "[DIALOG_BUTTON_OK]", "", 3);
}

// the highlight moved to a spot: a decoration shows as tapped
void HousePadLand(LeaderboardsWidget &w, const HouseSpot &theSpot) {
    gHousePad.kind = theSpot.kind;
    gHousePad.index = theSpot.index;
    if (theSpot.kind == HOUSE_ACHIEVEMENT) {
        w.mFocusedAchievementIndex = theSpot.index;
        w.mHighLightAchievement = true;
    } else {
        w.mHighLightAchievement = false;
    }
}
} // namespace

void LeaderboardsWidget::MouseDown(int x, int y, int theClickCount) {
    gHousePad.active = false; // Switch port: a finger: the highlight goes until the next button
    for (int i = 0; i < AchievementType::NUM_ACHIEVEMENT_TYPES; ++i) {
        int num = LeaderboardsWidget_GetAchievementIdByDrawOrder(AchievementType::NUM_ACHIEVEMENT_TYPES - 1 - i);
        if (!mAchievements[num])
            continue;
        if (gLeaderboardAchievementsRect[num][0].Contains(x, y) || gLeaderboardAchievementsRect[num][1].Contains(x, y)) {
            if (mFocusedAchievementIndex == num && mHighLightAchievement) {
                mHighLightAchievement = false;
            } else {
                mFocusedAchievementIndex = num;
                mHighLightAchievement = true;
            }
            return;
        }
    }

    int plantHeight = plantPileHeight * mPlantTrashBin->mPileNum;
    Sexy::Rect plantTrashBinRect = {mPlantTrashBin->mX, mPlantTrashBin->mY - plantHeight, addonImages.plant_can->mWidth, addonImages.plant_can->mHeight + plantHeight};

    if (plantTrashBinRect.Contains(x, y)) {
        mApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
        pvzstl::string str1 = TodStringTranslate("[PLANTS_KILLED]");
        pvzstl::string str2 = TodReplaceNumberString(str1, "{PLANTS}", mApp->mPlayerInfo->mGameStats.mMiscStats[GameStats::PLANTS_KILLED]);
        mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, str2.c_str(), "", "[DIALOG_BUTTON_OK]", "", 3);
        return;
    }

    int zombieHeight = zombiePileHeight * mZombieTrashBin->mPileNum;
    Sexy::Rect zombieTrashBinRect = {mZombieTrashBin->mX, mZombieTrashBin->mY - zombieHeight, addonImages.zombie_can->mWidth, addonImages.zombie_can->mHeight + zombieHeight};

    if (zombieTrashBinRect.Contains(x, y)) {
        mApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
        pvzstl::string str1 = TodStringTranslate("[ZOMBIES_KILLED]");
        pvzstl::string str2 = TodReplaceNumberString(str1, "{ZOMBIES}", mApp->mPlayerInfo->mGameStats.mMiscStats[GameStats::ZOMBIES_KILLED]);
        mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, str2.c_str(), "", "[DIALOG_BUTTON_OK]", "", 3);
        return;
    }

    // tmp = !tmp;
    // if (tmp) {
    // xx = x;
    // yy = y;
    // xw = 0;
    // yh = 0;
    // LOGD("%d %d", x, y);
    // }else{
    // xx1 = x;
    // yy1 = y;
    // xw1 = 0;
    // yh1 = 0;
    // LOGD("%d %d", x, y);
    // }

    // Rect rect = {1066, 574, 72, 72};
    // if (rect.Contains(x, y)) {
    // mTouchDownInBackRect = true;
    // mApp,->PlaySample(Sexy_SOUND_GRAVEBUTTON_Addr);
    // }
}

void LeaderboardsWidget::MouseDrag(int x, int y) {
    // if (tmp) {
    // xw = x - xx;
    // yh = y - yy;
    // LOGD("%d: %d, %d, %d, %d",mFocusedAchievementIndex,xx,yy,xw,yh);
    // }else{
    // xw1 = x - xx1;
    // yh1 = y - yy1;
    // LOGD("%d: %d, %d, %d, %d",mFocusedAchievementIndex,xx1,yy1,xw1,yh1);
    // }
}

void LeaderboardsWidget::MouseUp(int x, int y) {}

void LeaderboardsWidget::DealClick(Sexy::KeyCode theKey) {}

void LeaderboardsWidget::KeyDown(Sexy::KeyCode theKey) {
    if (theKey == Sexy::KEYCODE_ESCAPE || theKey == Sexy::KEYCODE_GAMEPAD_B) {
        if (mHighLightAchievement) {
            mHighLightAchievement = false;
            gHousePad.active = false;
            return;
        }
        mApp->KillLeaderboards();
        mApp->ShowMainMenuScreen();
        return;
    }
    // Switch port: the D-pad / stick move a highlight by where things are; A
    // does what a tap does
    theKey = switchpad::AsArrowKey(theKey);
    const int dx = theKey == Sexy::KEYCODE_LEFT ? -1 : theKey == Sexy::KEYCODE_RIGHT ? 1 : 0;
    const int dy = theKey == Sexy::KEYCODE_UP ? -1 : theKey == Sexy::KEYCODE_DOWN ? 1 : 0;
    const bool press = theKey == Sexy::KEYCODE_GAMEPAD_A || theKey == Sexy::KEYCODE_RETURN;
    if (dx != 0 || dy != 0 || press) {
        std::vector<HouseSpot> spots;
        CollectHouseSpots(*this, spots);
        int at = gHousePad.active ? FindHouseSpot(spots) : -1;
        if (at < 0) { // the first press shows the highlight: the house's first decoration, else the zombie pile
            gHousePad.active = true;
            const auto first = std::find_if(spots.begin(), spots.end(), [](const HouseSpot &h) { return h.kind == HOUSE_ACHIEVEMENT; });
            HousePadLand(*this, first != spots.end() ? *first : spots.front());
            return;
        }
        if (!press) {
            const int to = StepHouseSpot(spots, at, dx, dy);
            if (to >= 0) {
                HousePadLand(*this, spots[to]);
            }
            return;
        }
        switch (spots[at].kind) {
            case HOUSE_ZOMBIE_PILE:
                ShowPileCount(mApp, false);
                break;
            case HOUSE_PLANT_BIN:
                ShowPileCount(mApp, true);
                break;
            case HOUSE_CLOSE:
                mApp->KillLeaderboards();
                mApp->ShowMainMenuScreen();
                break;
            default: // a decoration: shown, as when tapped; A again hides it
                mHighLightAchievement = !mHighLightAchievement;
                break;
        }
        return;
    }
    if (theKey == Sexy::KEYCODE_UP || theKey == Sexy::KEYCODE_DOWN || theKey == Sexy::KEYCODE_LEFT || theKey == Sexy::KEYCODE_RIGHT) {
        bool flag = false;
        for (bool aAchievement : mAchievements) {
            if (aAchievement) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            return;
        }

        mHighLightAchievement = true;
        int aFocusedIndex = mFocusedAchievementIndex;
        if (theKey == Sexy::KEYCODE_UP || theKey == Sexy::KEYCODE_LEFT) {
            do {
                aFocusedIndex++;
                if (aFocusedIndex > 11) {
                    aFocusedIndex = 0;
                }
            } while (!mAchievements[aFocusedIndex]);
        } else {
            do {
                aFocusedIndex--;
                if (aFocusedIndex < 0) {
                    aFocusedIndex = 11;
                }
            } while (!mAchievements[aFocusedIndex]);
        }
        mFocusedAchievementIndex = aFocusedIndex;
        return;
    }
    if (theKey == Sexy::KEYCODE_QUICK_DIG) {
        mApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
        pvzstl::string str1 = TodStringTranslate("[PLANTS_KILLED]");
        pvzstl::string str2 = TodReplaceNumberString(str1, "{PLANTS}", mApp->mPlayerInfo->mGameStats.mMiscStats[GameStats::PLANTS_KILLED]);
        mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, str2.c_str(), "", "[DIALOG_BUTTON_OK]", "", 3);
        return;
    }
    if (theKey == Sexy::KEYCODE_X_BUTTON) {
        mApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
        pvzstl::string str1 = TodStringTranslate("[ZOMBIES_KILLED]");
        pvzstl::string str2 = TodReplaceNumberString(str1, "{ZOMBIES}", mApp->mPlayerInfo->mGameStats.mMiscStats[GameStats::ZOMBIES_KILLED]);
        mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, str2.c_str(), "", "[DIALOG_BUTTON_OK]", "", 3);
        return;
    }
}
