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

#ifndef PVZ_LAWN_WIDGET_LEADERBOARDS_WIDGET_H
#define PVZ_LAWN_WIDGET_LEADERBOARDS_WIDGET_H

#include "PvZ/SexyAppFramework/Widget/ButtonListener.h"
#include "PvZ/SexyAppFramework/Widget/Widget.h"

#include "GameButton.h"
#include "HouseChooserDialog.h"

class Reanimation;
class TrashBin;

class GameStats {
public:
    enum MiscStat {
        ZOMBIES_KILLED = 0,
        PLANTS_KILLED = 1,
        MONEY = 2,
        TREE_HEIGHT = 4,
        ENDLESS_FLAGS = 5,
        MOWED_ZOMBIES = 6,
        STINKY_CHOCOLATES = 7,
    };

    HouseType mHouseType;      // 0
    int unk;                   // 1
    int mMiscStats[9];         // 2 ~ 10
    char mFavoritePlants[49];  // 44 ~ 92
    char mFavoriteZombies[47]; // 93 ~ 139
    float mSurvivalCompletion; // 35
    float mMiniGameCompletion; // 36
    float mPuzzleCompletion;   // 37

    int ChangeMiscStat(MiscStat theMiscStat, int theChangeIndex);
}; // 大小38个整数

struct LeaderboardReanimations {
    Reanimation *backgroundReanim[5];
    Reanimation *achievementReanim[12];
};

class LeaderboardsWidget : public Sexy::Widget {
public:
    LawnApp *mApp;                                     // 64
    TrashBin *mZombieTrashBin;                         // 65
    TrashBin *mPlantTrashBin;                          // 66
    bool mAchievements[12];                            // 67 ~ 69
    LeaderboardReanimations *mLeaderboardReanimations; // 70
    int mLongestRecordPool;                            // 71
    GameButton *mBackButton;
    Sexy::ButtonListener *mButtonListener = &sButtonListener;
    int mFocusedAchievementIndex;
    bool mHighLightAchievement;

    LeaderboardsWidget(LawnApp *theApp);
    ~LeaderboardsWidget() = delete;

    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    void Update();
    void Draw(Sexy::Graphics *g);
    void MouseDown(int x, int y, int theClickCount);
    void MouseDrag(int x, int y);
    void MouseUp(int x, int y);
    void DealClick(Sexy::KeyCode theKey);
    void KeyDown(Sexy::KeyCode theKey);

    void ButtonPress(this LeaderboardsWidget &self, int id, int theCount) {}
    void ButtonDepress(this LeaderboardsWidget &self, int id);

protected:
    friend void InitHookFunction();

    void _constructor(LawnApp *theApp);
    void _destructor();

private:
    static inline const Sexy::ButtonListener::VTable sButtonListenerVtable{
        // .ButtonPress = (void *)LeaderboardsWidget_ButtonPress;
        .ButtonPress2 = (void *)&LeaderboardsWidget::ButtonPress,
        .ButtonDepress = (void *)&LeaderboardsWidget::ButtonDepress,
    };

    static inline Sexy::ButtonListener sButtonListener{&sButtonListenerVtable};
};

// 使用 LeaderboardsWidget 取代 DaveHelp
class DaveHelp : public Sexy::Widget {
public:
    DaveHelp(LawnApp *theApp) {
        _constructor(theApp);
    }

    ~DaveHelp() = delete;

protected:
    void _constructor(LawnApp *theApp) {
        reinterpret_cast<void (*)(DaveHelp *, LawnApp *)>(DaveHelp_DaveHelpAddr)(this, theApp);
    }
};

#endif // PVZ_LAWN_WIDGET_LEADERBOARDS_WIDGET_H
