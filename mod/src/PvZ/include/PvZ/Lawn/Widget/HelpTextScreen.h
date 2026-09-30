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

#ifndef PVZ_LAWN_WIDGET_HELP_TEXT_SCREEN_H
#define PVZ_LAWN_WIDGET_HELP_TEXT_SCREEN_H

#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/Widget/LawnMenuWidget.h"

class HelpBarWidget;

class HelpTextScreen : public LawnMenuWidget {
public:
    LawnApp *mApp;
    float float128;
    float float12C;
    float float130;
    int mPage;
    HelpBarWidget *mHelpBarWidget;

    HelpTextScreen(LawnApp *theApp, int thePage) {
        _constructor(theApp, thePage);
    }

    ~HelpTextScreen() = delete;

    void KeyDown(Sexy::KeyCode theKey) {
        reinterpret_cast<void (*)(HelpTextScreen *, Sexy::KeyCode)>(HelpTextScreen_KeyDownAddr)(this, theKey);
    }

    void AddedToManager(Sexy::WidgetManager *theWidgetNanager);
    void RemovedFromManager(Sexy::WidgetManager *theWidgetNanager);
    void Update();
    void Draw(Sexy::Graphics *g);
    void ButtonDepress(int theId);

    void MouseDown(int x, int y, int theClickCount);

protected:
    friend void InitHookFunction();

    void _constructor(LawnApp *theApp, int thePage);
    void _destructor();
};


inline void (*old_HelpTextScreen__constructor)(HelpTextScreen *, LawnApp *, int);

inline void (*old_HelpTextScreen__destructor)(HelpTextScreen *);

inline void (*old_HelpTextScreen_AddedToManager)(HelpTextScreen *, Sexy::WidgetManager *);

inline void (*old_HelpTextScreen_RemovedFromManager)(HelpTextScreen *, Sexy::WidgetManager *);

inline void (*old_HelpTextScreen_Update)(HelpTextScreen *);

inline void (*old_HelpTextScreen_Draw)(HelpTextScreen *, Sexy::Graphics *);

inline void (*old_HelpTextScreen_MouseDown)(HelpTextScreen *, int, int, int);

inline void (*old_HelpTextScreen_ButtonDepress)(HelpTextScreen *, int);

// Switch port: the help screen's Controls page in a smaller BrianneTod (HelpTextScreen.cpp)
void HelpTextWidget_Draw(Sexy::Widget *theWidget, Sexy::Graphics *g);
inline void (*old_HelpTextWidget_Draw)(Sexy::Widget *, Sexy::Graphics *);

#endif // PVZ_LAWN_WIDGET_HELP_TEXT_SCREEN_H
