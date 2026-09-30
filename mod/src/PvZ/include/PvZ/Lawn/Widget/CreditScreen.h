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

#ifndef PVZ_LAWN_WIDGET_CREDIT_SCREEN_H
#define PVZ_LAWN_WIDGET_CREDIT_SCREEN_H

#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/SexyAppFramework/Widget/Widget.h"
// #include "../../SexyAppFramework/PerfTimer.h"
#include "PvZ/SexyAppFramework/Misc/PerfTimer.h"
#include "PvZ/SexyAppFramework/Widget/ButtonListener.h"

#include "GameButton.h"

class CreditScreen : public Sexy::Widget, public Sexy::ButtonListener {
public:
    enum CreditsPhase {
        CREDITS_MAIN1 = 0,
        CREDITS_MAIN2 = 1,
        CREDITS_MAIN3 = 2,
        CREDITS_END = 3,
    };

public:
    LawnApp *mApp;                    // 65
    CreditsPhase mCreditsPhase;       // 66
    int mCreditsPhaseCounter;         // 67
    int mCreditsReanimID;             // 68
    int mFogParticleID;               // 69
    int mBlinkCountdown;              // 70
    Sexy::Widget *mOverlayWidget;     // 71
    bool mDrawBrain;                  // 288
    float mBrainPosX;                 // 73
    float mBrainPosY;                 // 74
    int mUpdateCount;                 // 75
    int mDrawCount;                   // 76
    int unkInt1;                      // 77
    Sexy::PerfTimer mTimerSinceStart; // 78 ~ 82
    int unkInt2;                      // 83
    bool mDontSync;                   // 336
    bool mCreditsPaused;              // 337
    int unkInt3[3];                   // 85 ~ 87
    bool mPreloaded;                  // 352
    int unkInt4;                      // 89
    double mScrollPositionY1;         // 90 ~ 91
    double mScrollPositionY2;         // 92 ~ 93
    bool mIsFromMainMenu;             // 376,即94

    // 大小95个整数

    void PauseCredits() {
        reinterpret_cast<void (*)(CreditScreen *)>(CreditScreen_PauseCreditsAddr)(this);
    }

    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    void ButtonDepress(int theId);

protected:
    friend void InitHookFunction();

    CreditScreen() = default;
    ~CreditScreen() = default;

    void _constructor(LawnApp *theApp, bool theBool);
    void _destructor();
};

inline GameButton *gCreditScreenBackButton;

inline void (*old_CreditScreen__constructor)(CreditScreen *, LawnApp *, bool);

inline void (*old_CreditScreen__destructor)(CreditScreen *);

inline void (*old_CreditScreen_AddedToManager)(CreditScreen *, Sexy::WidgetManager *);

inline void (*old_CreditScreen_RemovedFromManager)(CreditScreen *, Sexy::WidgetManager *);

// Switch port: the credits' pause box (CreditScreen::PauseCredits, a
// LawnMessageBox) says "Exit" where the game's table has the main menu's
// "Exit Game" / "Main Menu": LawnApp::LawnMessageBox, hooked.
int CreditScreen_LawnMessageBox(LawnApp *theApp, Dialogs theDialogId, const char *theHeaderName, const char *theLinesName, const char *theButton1Name, const char *theButton2Name, int theButtonMode);

inline int (*old_LawnApp_LawnMessageBox)(LawnApp *, Dialogs, const char *, const char *, const char *, const char *, int);

#endif // PVZ_LAWN_WIDGET_CREDIT_SCREEN_H
