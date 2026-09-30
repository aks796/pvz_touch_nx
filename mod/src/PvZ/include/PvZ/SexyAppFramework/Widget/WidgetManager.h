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

#ifndef PVZ_SEXYAPPFRAMEWORK_WIDGET_WIDGET_MANAGER_H
#define PVZ_SEXYAPPFRAMEWORK_WIDGET_WIDGET_MANAGER_H

#include "../Misc/Common.h"
#include "../Misc/KeyCodes.h"
#include "WidgetContainer.h"

namespace Sexy {

class Widget;

struct DeferredOverlayVector {
    void *mStart;        // +0x00
    void *mFinish;       // +0x04
    void *mEndOfStorage; // +0x08
};

using KeyDownMapLayout = pvzstl::map<int, bool>;

class WidgetManager : public WidgetContainer {
public:
    struct WidgetManagerVTable {
        void (*completeDestructor)(WidgetManager *self);                                                             // 0x00
        void (*deletingDestructor)(WidgetManager *self);                                                             // 0x04
        Rect (*GetRect)(WidgetContainer *self);                                                                      // 0x08
        bool (*Intersects)(WidgetContainer *self, WidgetContainer *other);                                           // 0x0C
        void (*SortWidgets)(WidgetContainer *self);                                                                  // 0x10
        void (*SortWidgetsWithWidget)(WidgetContainer *self, Widget *widget);                                        // 0x14
        void (*AddWidget)(WidgetContainer *self, Widget *widget);                                                    // 0x18
        void (*RemoveWidget)(WidgetContainer *self, Widget *widget);                                                 // 0x1C
        bool (*HasWidget)(WidgetContainer *self, Widget *widget);                                                    // 0x20
        Widget *(*FindWidget)(WidgetContainer *self, int index);                                                     // 0x24
        void (*DisableWidget)(WidgetManager *self, Widget *widget);                                                  // 0x28
        void (*RemoveAllWidgets)(WidgetContainer *self, bool deleteWidgets, bool recursive, WidgetManager *manager); // 0x2C
        void (*SetFocus)(WidgetManager *self, Widget *widget);                                                       // 0x30
        bool (*IsBelow)(WidgetContainer *self, Widget *first, Widget *second);                                       // 0x34
        void (*MarkAllDirty)(WidgetContainer *self);                                                                 // 0x38
        void (*BringToFront)(WidgetContainer *self, Widget *widget);                                                 // 0x3C
        void (*BringToBack)(WidgetContainer *self, Widget *widget);                                                  // 0x40
        void (*PutBehind)(WidgetContainer *self, Widget *first, Widget *second);                                     // 0x44
        void (*PutInfront)(WidgetContainer *self, Widget *first, Widget *second);                                    // 0x48
        Point (*GetAbsPos)(const WidgetContainer *self);                                                             // 0x4C
        Point (*GetAbsPosInManager)(const WidgetContainer *self);                                                    // 0x50
        Point (*GetAbsPosInScreen)(const WidgetContainer *self);                                                     // 0x54
        Point (*GetCenter)(const WidgetContainer *self);                                                             // 0x58
        Point (*GetAbsCenter)(const WidgetContainer *self);                                                          // 0x5C
        Point (*GetAbsCenterInScreen)(const WidgetContainer *self);                                                  // 0x60
        void (*MarkDirty)(WidgetContainer *self);                                                                    // 0x64
        void (*MarkDirtyFull)(WidgetContainer *self);                                                                // 0x68
        void (*MarkDirtyFullWithContainer)(WidgetContainer *self, WidgetContainer *container);                       // 0x6C
        void (*MarkDirtyWithContainer)(WidgetContainer *self, WidgetContainer *container);                           // 0x70
        void (*AddedToManager)(WidgetContainer *self, WidgetManager *manager);                                       // 0x74
        void (*RemovedFromManager)(WidgetContainer *self, WidgetManager *manager);                                   // 0x78
        void (*Update)(WidgetContainer *self);                                                                       // 0x7C
        void (*UpdateAll)(WidgetContainer *self, ModalFlags *modalFlags);                                            // 0x80
        void (*UpdateF)(WidgetContainer *self, float delta);                                                         // 0x84
        void (*UpdateFAll)(WidgetContainer *self, ModalFlags *modalFlags, float delta);                              // 0x88
        void (*DrawPre)(WidgetContainer *self, Graphics *graphics);                                                  // 0x8C
        void (*Draw)(WidgetContainer *self, Graphics *graphics);                                                     // 0x90
        void (*DrawOther)(WidgetContainer *self, Graphics *graphics);                                                // 0x94
        void (*DrawAll)(WidgetContainer *self, ModalFlags *modalFlags, Graphics *graphics);                          // 0x98
        void (*SysColorChangedAll)(WidgetContainer *self);                                                           // 0x9C
        void (*SysColorChanged)(WidgetContainer *self);                                                              // 0xA0
    };

public:
    Graphics *mCurG;     // +0x00
    SexyAppBase *mApp;   // +0x04
    MemoryImage *mImage; // +0x08
    MemoryImage *mTransientImage;
    bool mLastHadTransients;
    Widget *mPopupCommandWidget;
    DeferredOverlayVector mDeferredOverlayWidgets;
    int mMinDeferredOverlayPriority;
    bool mHasFocus;
    Widget *mFocusWidget; // 40
    Widget *mLastDownWidget;
    Widget *mOverWidget;
    Widget *mBaseModalWidget;
    FlagsMod mLostFocusFlagsMod;
    FlagsMod mBelowModalFlagsMod;
    FlagsMod mDefaultBelowModalFlagsMod;
    int mPreModalInfoList[2]; // std::list<PreModalInfo> mPreModalInfoList;
    Rect mMouseDestRect;
    Rect mMouseSourceRect;
    bool mMouseIn;
    int mLastMouseX;
    int mLastMouseY;
    int mDownButtons;
    int mActualDownButtons;
    int mLastInputUpdateCnt;
    homura::Storage<KeyDownMapLayout> mKeyDown;
    int mLastDownButtonId;
    int mWidgetFlags;
    char unkMem6[60]; // 推测和AxisMove有关

    // 大小89个整数
    void SetFocus(Widget *aWidget) {
        reinterpret_cast<void (*)(WidgetManager *, Widget *)>(Sexy_WidgetManager_SetFocusAddr)(this, aWidget);
    }
    Widget *GetWidgetAt(int x, int y, int *theWidgetX, int *theWidgetY) {
        return reinterpret_cast<Widget *(*)(WidgetManager *, int, int, int *, int *)>(Sexy_WidgetManager_GetWidgetAtAddr)(this, x, y, theWidgetX, theWidgetY);
    }
    int GetWidgetFlags() {
        return reinterpret_cast<int (*)(WidgetManager *)>(Sexy_WidgetManager_GetWidgetFlagsAddr)(this);
    }
    void RehupMouse() {
        reinterpret_cast<void (*)(WidgetManager *)>(Sexy_WidgetManager_RehupMouseAddr)(this);
    }
    const WidgetManagerVTable *GetWidgetManagerVTable() const noexcept {
        return (WidgetManagerVTable *)vTable;
    }
    void MouseDown(int x, int y, int theClickCount);
    void MouseDrag(int x, int y);
    void MouseUp(int x, int y, int theClickCount);
};

} // namespace Sexy

inline void (*old_Sexy_WidgetManager_MouseDown)(Sexy::WidgetManager *, int x, int y, int theClickCount);
inline void (*old_Sexy_WidgetManager_MouseDrag)(Sexy::WidgetManager *, int x, int y);
inline void (*old_Sexy_WidgetManager_MouseUp)(Sexy::WidgetManager *, int x, int y, int theClickCount);


#endif // PVZ_SEXYAPPFRAMEWORK_WIDGET_WIDGET_MANAGER_H
