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

#ifndef PVZ_SEXYAPPFRAMEWORK_WIDGET_WIDGET_CONTAINER_H
#define PVZ_SEXYAPPFRAMEWORK_WIDGET_WIDGET_CONTAINER_H

#include "PvZ/STL/map.h"
#include "PvZ/SexyAppFramework/Misc/Common.h"
#include "PvZ/SexyAppFramework/Misc/Flags.h"
#include "PvZ/SexyAppFramework/Misc/Rect.h"
#include "PvZ/Symbols.h"

namespace Sexy {

class Graphics;
class WidgetManager;
class Widget;
struct WidgetListNode {
    WidgetListNode *mNext;
    WidgetListNode *mPrev;
    Widget *mWidget;
};

struct WidgetListHead {
    WidgetListNode *mNext;
    WidgetListNode *mPrev;
    int mSize;
    int mReserved[2];
};

using WidgetUserDataMap32 = pvzstl::map<int, void *>;


class WidgetContainer {
public:
    struct WidgetContainerVTable {
        // 0x00 — WidgetContainer::~WidgetContainer()
        void (*completeDestructor)(WidgetContainer *self);
        // 0x04 — deleting destructor
        void (*deletingDestructor)(WidgetContainer *self);
        // 0x08
        Rect (*GetRect)(WidgetContainer *self);
        // 0x0C
        bool (*Intersects)(WidgetContainer *self, WidgetContainer *other);
        // 0x10
        void (*SortWidgets)(WidgetContainer *self);
        // 0x14
        void (*SortWidgetsWithWidget)(WidgetContainer *self, Widget *widget);
        // 0x18
        void (*AddWidget)(WidgetContainer *self, Widget *widget);
        // 0x1C
        void (*RemoveWidget)(WidgetContainer *self, Widget *widget);
        // 0x20
        bool (*HasWidget)(WidgetContainer *self, Widget *widget);
        // 0x24
        Widget *(*FindWidget)(WidgetContainer *self, int index);
        // 0x28
        void (*DisableWidget)(WidgetContainer *self, Widget *widget);
        // 0x2C
        void (*RemoveAllWidgets)(WidgetContainer *self, bool deleteWidgets, bool recursive, WidgetManager *manager);
        // 0x30
        void (*SetFocus)(WidgetContainer *self, Widget *widget);
        // 0x34
        bool (*IsBelow)(WidgetContainer *self, Widget *first, Widget *second);
        // 0x38
        void (*MarkAllDirty)(WidgetContainer *self);
        // 0x3C
        void (*BringToFront)(WidgetContainer *self, Widget *widget);
        // 0x40
        void (*BringToBack)(WidgetContainer *self, Widget *widget);
        // 0x44
        void (*PutBehind)(WidgetContainer *self, Widget *first, Widget *second);
        // 0x48
        void (*PutInfront)(WidgetContainer *self, Widget *first, Widget *second);
        // 0x4C — const 成员函数
        Point (*GetAbsPos)(const WidgetContainer *self);
        // 0x50 — const 成员函数
        Point (*GetAbsPosInManager)(const WidgetContainer *self);
        // 0x54 — const 成员函数
        Point (*GetAbsPosInScreen)(const WidgetContainer *self);
        // 0x58 — const 成员函数
        Point (*GetCenter)(const WidgetContainer *self);
        // 0x5C — const 成员函数
        Point (*GetAbsCenter)(const WidgetContainer *self);
        // 0x60 — const 成员函数
        Point (*GetAbsCenterInScreen)(const WidgetContainer *self);
        // 0x64
        void (*MarkDirty)(WidgetContainer *self);
        // 0x68
        void (*MarkDirtyFull)(WidgetContainer *self);
        // 0x6C
        void (*MarkDirtyFullWithContainer)(WidgetContainer *self, WidgetContainer *container);
        // 0x70
        void (*MarkDirtyWithContainer)(WidgetContainer *self, WidgetContainer *container);
        // 0x74
        void (*AddedToManager)(WidgetContainer *self, WidgetManager *manager);
        // 0x78
        void (*RemovedFromManager)(WidgetContainer *self, WidgetManager *manager);
        // 0x7C
        void (*Update)(WidgetContainer *self);
        // 0x80
        void (*UpdateAll)(WidgetContainer *self, ModalFlags *modalFlags);
        // 0x84
        void (*UpdateF)(WidgetContainer *self, float delta);
        // 0x88
        void (*UpdateFAll)(WidgetContainer *self, ModalFlags *modalFlags, float delta);
        // 0x8C
        void (*DrawPre)(WidgetContainer *self, Graphics *graphics);
        // 0x90
        void (*Draw)(WidgetContainer *self, Graphics *graphics);
        // 0x94
        void (*DrawOther)(WidgetContainer *self, Graphics *graphics);
        // 0x98
        void (*DrawAll)(WidgetContainer *self, ModalFlags *modalFlags, Graphics *graphics);
        // 0x9C
        void (*SysColorChangedAll)(WidgetContainer *self);
        // 0xA0
        void (*SysColorChanged)(WidgetContainer *self);
    };

public:
    void **vTable;                                  // 0
    WidgetListHead mWidgets;                        // 1 ~ 5
    WidgetManager *mWidgetManager;                  // 6
    WidgetContainer *mParent;                       // 7
    bool mUpdateIteratorModified;                   // 32
    int *mUpdateIterator;                           // 9
    int mLastWMUpdateCount;                         // 10
    int mUpdateCnt;                                 // 11
    bool mDirty;                                    // 12
    int mX;                                         // 13
    int mY;                                         // 14
    int mWidth;                                     // 15
    int mHeight;                                    // 16
    bool mHasAlpha;                                 // 68
    bool mClip;                                     // 69
    FlagsMod mWidgetFlagsMod;                       // 18 ~ 19
    int mPriority;                                  // 20
    int mZOrder;                                    // 21
    homura::Storage<WidgetUserDataMap32> mUserData; // 22 ~ 27
    int mWidgetId;                                  // 28
    // 大小未知，目前认为是29个整数。反正Widget是64个整数，足够了。

    void SetFocus(Widget *theWidget) { // vTable + 48
        reinterpret_cast<void (*)(WidgetContainer *, Widget *)>(Sexy_WidgetContainer_SetFocusAddr)(this, theWidget);
    }
    void MarkDirty() { // vTable + 100
        reinterpret_cast<void (*)(WidgetContainer *)>(Sexy_WidgetContainer_MarkDirtyAddr)(this);
    }
    void AddWidget(Widget *theWidget) { // vTable + 24
        reinterpret_cast<void (*)(WidgetContainer *, Widget *)>(Sexy_WidgetContainer_AddWidgetAddr)(this, theWidget);
    }
    Widget *GetWidgetAtHelper(int x, int y, int theFlags, bool *found, int *theWidgetX, int *theWidgetY) {
        return reinterpret_cast<Widget *(*)(WidgetContainer *, int, int, int, bool *, int *, int *)>(Sexy_WidgetContainer_GetWidgetAtHelperAddr)(this, x, y, theFlags, found, theWidgetX, theWidgetY);
    }
    void RemoveWidget(Widget *theWidget) { // vTable + 28
        reinterpret_cast<void (*)(WidgetContainer *, Widget *)>(Sexy_WidgetContainer_RemoveWidgetAddr)(this, theWidget);
    }
    void AddedToManager(WidgetManager *theWidgetManager) {
        reinterpret_cast<void (*)(WidgetContainer *, WidgetManager *)>(Sexy_WidgetContainer_AddedToManagerAddr)(this, theWidgetManager);
    }
    void RemovedFromManager(WidgetManager *theWidgetManager) {
        reinterpret_cast<void (*)(WidgetContainer *, WidgetManager *)>(Sexy_WidgetContainer_RemovedFromManagerAddr)(this, theWidgetManager);
    }
    void BringToFront(Widget *theWidget) { // vTable + 60
        reinterpret_cast<void (*)(WidgetContainer *, Widget *)>(Sexy_WidgetContainer_BringToFrontAddr)(this, theWidget);
    }
    void BringToBack(Widget *theWidget) { // vTable + 64
        reinterpret_cast<void (*)(WidgetContainer *, Widget *)>(Sexy_WidgetContainer_BringToBackAddr)(this, theWidget);
    }
    Widget *FindWidget(int theId) {
        return reinterpret_cast<Widget *(*)(WidgetContainer *, int)>(Sexy_WidgetContainer_FindWidgetAddr)(this, theId);
    }
    const WidgetContainerVTable *GetVTable() {
        return (WidgetContainerVTable *)vTable;
    }

protected:
    WidgetContainer() = default;
    ~WidgetContainer() = default;
};

} // namespace Sexy

#endif // PVZ_SEXYAPPFRAMEWORK_WIDGET_WIDGET_CONTAINER_H
