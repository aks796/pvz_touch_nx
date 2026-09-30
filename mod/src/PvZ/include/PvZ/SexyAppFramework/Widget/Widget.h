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

#ifndef PVZ_SEXYAPPFRAMEWORK_WIDGET_WIDGET_H
#define PVZ_SEXYAPPFRAMEWORK_WIDGET_WIDGET_H

#include "Homura/TypeUtils.h"
#include "PvZ/SexyAppFramework/Graphics/Color.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Misc/Common.h"
#include "PvZ/SexyAppFramework/Misc/KeyCodes.h"

#include "PvZ/SexyAppFramework/Event.h"
#include "Insets.h"
#include "WidgetContainer.h"

#include <vector>

namespace Sexy {

class WidgetManager;


class Widget : public WidgetContainer {
    struct WidgetVTable {
        void (*completeDestructor)(Widget *self);                                                                                                                            // 0x000
        void (*deletingDestructor)(Widget *self);                                                                                                                            // 0x004
        Rect (*GetRect)(WidgetContainer *self);                                                                                                                              // 0x008
        bool (*Intersects)(WidgetContainer *self, WidgetContainer *other);                                                                                                   // 0x00C
        void (*SortWidgets)(WidgetContainer *self);                                                                                                                          // 0x010
        void (*SortWidgetsWithWidget)(WidgetContainer *self, Widget *widget);                                                                                                // 0x014
        void (*AddWidget)(WidgetContainer *self, Widget *widget);                                                                                                            // 0x018
        void (*RemoveWidget)(WidgetContainer *self, Widget *widget);                                                                                                         // 0x01C
        bool (*HasWidget)(WidgetContainer *self, Widget *widget);                                                                                                            // 0x020
        Widget *(*FindWidget)(WidgetContainer *self, int index);                                                                                                             // 0x024
        void (*DisableWidget)(WidgetContainer *self, Widget *widget);                                                                                                        // 0x028
        void (*RemoveAllWidgets)(WidgetContainer *self, bool deleteWidgets, bool recursive, WidgetManager *manager);                                                         // 0x02C
        void (*SetFocusContainer)(WidgetContainer *self, Widget *widget);                                                                                                    // 0x030
        bool (*IsBelow)(WidgetContainer *self, Widget *first, Widget *second);                                                                                               // 0x034
        void (*MarkAllDirty)(WidgetContainer *self);                                                                                                                         // 0x038
        void (*BringToFront)(WidgetContainer *self, Widget *widget);                                                                                                         // 0x03C
        void (*BringToBack)(WidgetContainer *self, Widget *widget);                                                                                                          // 0x040
        void (*PutBehind)(WidgetContainer *self, Widget *first, Widget *second);                                                                                             // 0x044
        void (*PutInfront)(WidgetContainer *self, Widget *first, Widget *second);                                                                                            // 0x048
        Point (*GetAbsPos)(const WidgetContainer *self);                                                                                                                     // 0x04C
        Point (*GetAbsPosInManager)(const WidgetContainer *self);                                                                                                            // 0x050
        Point (*GetAbsPosInScreen)(const WidgetContainer *self);                                                                                                             // 0x054
        Point (*GetCenter)(const WidgetContainer *self);                                                                                                                     // 0x058
        Point (*GetAbsCenter)(const WidgetContainer *self);                                                                                                                  // 0x05C
        Point (*GetAbsCenterInScreen)(const WidgetContainer *self);                                                                                                          // 0x060
        void (*MarkDirty)(WidgetContainer *self);                                                                                                                            // 0x064
        void (*MarkDirtyFull)(WidgetContainer *self);                                                                                                                        // 0x068
        void (*MarkDirtyFullWithContainer)(WidgetContainer *self, WidgetContainer *container);                                                                               // 0x06C
        void (*MarkDirtyWithContainer)(WidgetContainer *self, WidgetContainer *container);                                                                                   // 0x070
        void (*AddedToManager)(WidgetContainer *self, WidgetManager *manager);                                                                                               // 0x074
        void (*RemovedFromManager)(WidgetContainer *self, WidgetManager *manager);                                                                                           // 0x078
        void (*Update)(Widget *self);                                                                                                                                        // 0x07C
        void (*UpdateAll)(WidgetContainer *self, ModalFlags *modalFlags);                                                                                                    // 0x080
        void (*UpdateF)(Widget *self, float delta);                                                                                                                          // 0x084
        void (*UpdateFAll)(WidgetContainer *self, ModalFlags *modalFlags, float delta);                                                                                      // 0x088
        void (*DrawPre)(Widget *self, Graphics *graphics);                                                                                                                   // 0x08C
        void (*Draw)(Widget *self, Graphics *graphics);                                                                                                                      // 0x090
        void (*DrawOther)(Widget *self, Graphics *graphics);                                                                                                                 // 0x094
        void (*DrawAll)(WidgetContainer *self, ModalFlags *modalFlags, Graphics *graphics);                                                                                  // 0x098
        void (*SysColorChangedAll)(WidgetContainer *self);                                                                                                                   // 0x09C
        void (*SysColorChanged)(WidgetContainer *self);                                                                                                                      // 0x0A0
        void (*OrderInManagerChanged)(Widget *self);                                                                                                                         // 0x0A4
        void (*SetVisible)(Widget *self, bool visible);                                                                                                                      // 0x0A8
        void (*SetColors3)(Widget *self, int (*colors)[3], int count);                                                                                                       // 0x0AC
        void (*SetColors4)(Widget *self, int (*colors)[4], int count);                                                                                                       // 0x0B0
        void (*SetColor)(Widget *self, int index, const Color &color);                                                                                                       // 0x0B4
        Color (*GetColor)(Widget *self, int index);                                                                                                                          // 0x0B8
        Color (*GetColorWithDefault)(Widget *self, int index, const Color &defaultColor);                                                                                    // 0x0BC
        void (*SetDisabled)(Widget *self, bool disabled);                                                                                                                    // 0x0C0
        void (*ShowFinger)(Widget *self, bool show);                                                                                                                         // 0x0C4
        void (*Resize)(Widget *self, int x, int y, int width, int height);                                                                                                   // 0x0C8
        void (*ResizeRect)(Widget *self, const TRect<int> &rect);                                                                                                            // 0x0CC
        void (*Move)(Widget *self, int x, int y);                                                                                                                            // 0x0D0
        bool (*WantsFocus)(Widget *self);                                                                                                                                    // 0x0D4
        void (*DrawOverlay)(Widget *self, Graphics *graphics);                                                                                                               // 0x0D8
        void (*DrawOverlayWithPriority)(Widget *self, Graphics *graphics, int priority);                                                                                     // 0x0DC
        void (*SetDefaultFocus)(Widget *self, Widget *widget);                                                                                                               // 0x0E0
        void (*SetFocus)(Widget *self, Widget *widget, bool force);                                                                                                          // 0x0E4
        void (*SetFocusLink)(Widget *self, int direction, Widget *widget);                                                                                                   // 0x0E8
        void (*SetFocusLinks)(Widget *self, Widget *up, Widget *down, Widget *left, Widget *right);                                                                          // 0x0EC
        void (*FocusDirectionHint)(Widget *self, int direction);                                                                                                             // 0x0F0
        void (*GotFocus)(Widget *self);                                                                                                                                      // 0x0F4
        void (*LostFocus)(Widget *self);                                                                                                                                     // 0x0F8
        bool (*HasWindowFocus)(Widget *self);                                                                                                                                // 0x0FC
        void (*SetWindowFocus)(Widget *self, bool focused);                                                                                                                  // 0x100
        void (*GotWindowFocus)(Widget *self);                                                                                                                                // 0x104
        void (*LostWindowFocus)(Widget *self);                                                                                                                               // 0x108
        void (*KeyChar)(Widget *self, char character);                                                                                                                       // 0x10C
        void (*KeyUnicode)(Widget *self, int character);                                                                                                                     // 0x110
        void (*KeyDownEvent)(Widget *self, const Event &event);                                                                                                              // 0x114
        void (*KeyUpEvent)(Widget *self, const Event &event);                                                                                                                // 0x118
        void (*KeyDown)(Widget *self, KeyCode keyCode);                                                                                                                      // 0x11C
        void (*KeyUp)(Widget *self, KeyCode keyCode);                                                                                                                        // 0x120
        void (*MouseEnter)(Widget *self);                                                                                                                                    // 0x124
        void (*MouseLeave)(Widget *self);                                                                                                                                    // 0x128
        void (*MouseMove)(Widget *self, int x, int y);                                                                                                                       // 0x12C
        void (*MouseDown3)(Widget *self, int x, int y, int clickCount);                                                                                                      // 0x130
        void (*MouseDown4)(Widget *self, int x, int y, int button, int clickCount);                                                                                          // 0x134
        void (*MouseUp2)(Widget *self, int x, int y);                                                                                                                        // 0x138
        void (*MouseUp3)(Widget *self, int x, int y, int clickCount);                                                                                                        // 0x13C
        void (*MouseUp4)(Widget *self, int x, int y, int button, int clickCount);                                                                                            // 0x140
        void (*MouseDrag)(Widget *self, int x, int y);                                                                                                                       // 0x144
        void (*MouseWheel)(Widget *self, int delta);                                                                                                                         // 0x148
        void (*MouseWheelWithPosition)(Widget *self, int x, int y);                                                                                                          // 0x14C
        void (*TouchEnter)(Widget *self);                                                                                                                                    // 0x150
        void (*TouchLeave)(Widget *self);                                                                                                                                    // 0x154
        void (*TouchDown3)(Widget *self, int touchId, int x, int y);                                                                                                         // 0x158
        void (*TouchMove3)(Widget *self, int touchId, int x, int y);                                                                                                         // 0x15C
        void (*TouchUp3)(Widget *self, int touchId, int x, int y);                                                                                                           // 0x160
        void (*TouchCancel3)(Widget *self, int touchId, int x, int y);                                                                                                       // 0x164
        void (*TouchEvents)(Widget *self, const std::vector<Event> &events);                                                                                                 // 0x168
        void (*TouchDownEvents)(Widget *self, const std::vector<Event> &events);                                                                                             // 0x16C
        void (*TouchMoveEvents)(Widget *self, const std::vector<Event> &events);                                                                                             // 0x170
        void (*TouchUpEvents)(Widget *self, const std::vector<Event> &events);                                                                                               // 0x174
        void (*TouchCancelEvents)(Widget *self, const std::vector<Event> &events);                                                                                           // 0x178
        void (*TouchBeganVector)(Widget *self, std::vector<Touch> &touches);                                                                                                 // 0x17C
        void (*TouchMovedVector)(Widget *self, std::vector<Touch> &touches);                                                                                                 // 0x180
        void (*TouchEndedVector)(Widget *self, std::vector<Touch> &touches);                                                                                                 // 0x184
        void (*TouchCanceledVector)(Widget *self, std::vector<Touch> &touches);                                                                                              // 0x188
        void (*TouchBegan)(Widget *self, Touch *touch);                                                                                                                      // 0x18C
        void (*TouchMoved)(Widget *self, Touch *touch);                                                                                                                      // 0x190
        void (*TouchEnded)(Widget *self, Touch *touch);                                                                                                                      // 0x194
        void (*TouchCanceled)(Widget *self, Touch *touch);                                                                                                                   // 0x198
        void (*TouchesCanceled)(Widget *self);                                                                                                                               // 0x19C
        void (*AxisMoved)(Widget *self, const Event &event);                                                                                                                 // 0x1A0
        void (*UserEvent)(Widget *self, const Event &event);                                                                                                                 // 0x1A4
        bool (*IsPointVisible)(Widget *self, int x, int y);                                                                                                                  // 0x1A8
        bool (*IsOnScreen)(Widget *self);                                                                                                                                    // 0x1AC
        void (*WriteCenteredLine)(Widget *self, Graphics *graphics, int y, const std::string &text);                                                                         // 0x1B0
        void (*WriteCenteredLineEx)(Widget *self, Graphics *graphics, int y, const std::string &text, Color color1, Color color2, const TPoint<int> &offset);                // 0x1B4
        void (*WriteString)(Widget *self, Graphics *graphics, const std::string &text, int x, int y, int width, int justification, bool drawString, int offset, int length); // 0x1B8
        void (*WriteWordWrapped)(Widget *self, Graphics *graphics, const TRect<int> &rect, const std::string &text, int lineSpacing, int justification);                     // 0x1BC
        int (*GetWordWrappedHeight)(Widget *self, Graphics *graphics, int width, const std::string &text, int lineSpacing);                                                  // 0x1C0
        int (*GetNumDigits)(Widget *self, int number);                                                                                                                       // 0x1C4
        void (*WriteNumberFromStrip)(Widget *self, Graphics *graphics, int number, int x, int y, Image *image, int digitWidth);                                              // 0x1C8
        bool (*Contains)(Widget *self, int x, int y);                                                                                                                        // 0x1CC
        Rect (*GetInsetRect)(Widget *self);                                                                                                                                  // 0x1D0
        Widget *(*FindFocusableWidget)(Widget *self, int direction, Widget *relativeWidget);                                                                                 // 0x1D4
        Widget *(*FindClosest)(Widget *self, Widget *widget);                                                                                                                // 0x1D8
        void (*OnArrowKeys)(Widget *self, KeyCode keyCode);                                                                                                                  // 0x1DC
        void (*OnKeyReturn)(Widget *self);                                                                                                                                   // 0x1E0
        void (*OnKeyEscape)(Widget *self);                                                                                                                                   // 0x1E4
    };

public:
    bool mVisible;                               // 116
    bool mMouseVisible;                          // 117
    bool mDisabled;                              // 118
    bool mHasFocus;                              // 119
    bool mIsDown;                                // 120
    bool mIsOver;                                // 121
    bool mHasTransparencies;                     // 122
    homura::Storage<std::vector<Color>> mColors; // 31 ~ 33
    Insets mMouseInsets;                         // 34 ~ 37
    bool mDoFinger;                              // 152
    bool mWantsFocus;                            // 153
    bool mUnknownBool;                           // 154
    int unk1[10];                                // 38 ~ 47
    Widget *mFocusLinks[4];                      // 48 ~ 52 ，上下左右
    int unk2[2];                                 // 53 ~ 54
    Widget *mFocusedChildWidget;                 // 55
    int unk56;                                   // 56
    bool mHasWindowFocus;                        // 57
    int unk58;                                   // 58
    Sexy::Event *mEvent;                         // 59
    int *mAnimatorForState[4];                   // 60 ~ 63
    // 大小64个整数！

    void Resize(int theX, int theY, int theWidth, int theHeight) {
        reinterpret_cast<void (*)(Widget *, int, int, int, int)>(Sexy_Widget_ResizeAddr)(this, theX, theY, theWidth, theHeight);
    }
    void SetVisible(bool isVisible) { // 42
        reinterpret_cast<void (*)(Widget *, bool)>(Sexy_Widget_SetVisibleAddr)(this, isVisible);
    }
    void Move(int theNewX, int theNewY) {
        reinterpret_cast<void (*)(Widget *, int, int)>(Sexy_Widget_MoveAddr)(this, theNewX, theNewY);
    }
    void DeferOverlay(int thePriority = 0) {
        reinterpret_cast<void (*)(Widget *, int)>(Sexy_Widget_DeferOverlayAddr)(this, thePriority);
    }
    void SetColor(int index, const Color theColor) {
        reinterpret_cast<void (*)(Widget *, int, const Color)>(Sexy_Widget_SetColorAddr)(this, index, theColor);
    }
    void Update() {
        reinterpret_cast<void (*)(Widget *)>(Sexy_Widget_UpdateAddr)(this);
    }
    const WidgetVTable *GetVTable() const {
        return (WidgetVTable *)vTable;
    }

protected:
    Widget() = default;
    ~Widget() = default;

    void _constructor() {
        reinterpret_cast<void (*)(Widget *)>(Sexy_Widget__constructorAddr)(this);
    }
    void _destructor() {
        reinterpret_cast<void (*)(Widget *)>(Sexy_Widget__destructorAddr)(this);
    }
};

} // namespace Sexy

#endif // PVZ_SEXYAPPFRAMEWORK_WIDGET_WIDGET_H
