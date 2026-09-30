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

#ifndef PVZ_LAWN_WIDGET_MAIL_SCREEN_H
#define PVZ_LAWN_WIDGET_MAIL_SCREEN_H

#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/System/Mailbox.h"
#include "PvZ/Symbols.h"

#include "LawnDialog.h"

class MailScreen : public LawnDialog {
public:
    // MailScreen::MessageDrawInfo (size 16)
    struct MessageDrawInfo {
        float mAnimTime;    // +0  animation timer
        int mAnimState;     // +4  0:left out, 1:left in, 2:shift left, 3:shift right, 4:static
        int mMessageIndex;  // +8  mailbox message index
        float mDrawXOffset; // +12 draw X offset
    };

    int mInited;              // 191 (byte in original; packed in int slot here)
    int mCurrentListPos;      // 192
    int mCurrentMessageIndex; // 193
    int mPage;                // 194 (0: inbox/unread, 1: read)

    int mUnreadListBegin;       // 195: vector<MessageDrawInfo>::begin
    int mUnreadListEnd;         // 196: vector<MessageDrawInfo>::end
    int mUnreadListCapacityEnd; // 197: vector<MessageDrawInfo>::capacity end

    int mReadListBegin;       // 198: vector<MessageDrawInfo>::begin
    int mReadListEnd;         // 199: vector<MessageDrawInfo>::end
    int mReadListCapacityEnd; // 200: vector<MessageDrawInfo>::capacity end

    int mNumUnread; // 201
    int mNumRead;   // 202

    float mRightHintAnimTime;  // 203
    float mLeftHintAnimTime;   // 204
    float mRightHintAnimValue; // 205
    float mLeftHintAnimValue;  // 206
                               // 115: 207, 111: 209

    MailScreen(LawnApp *theApp) = delete;
    ~MailScreen() = delete;

    void KeyDown(Sexy::KeyCode theKey) {
        reinterpret_cast<void (*)(MailScreen *, Sexy::KeyCode)>(MailScreen_KeyDownAddr)(this, theKey);
    }

    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void RemovedFromManager(Sexy::WidgetManager *widgetManager);

    void MouseDown(int x, int y, int theClickCount);
    void MouseUp(int x, int y);
    void MouseDrag(int x, int y);
    void ButtonPress(int theId);
    void ButtonDepress(int theId);

protected:
    friend void InitHookFunction();

    void _constructor(LawnApp *theApp);
    void _destructor2();
};

// inline void (*MailScreen_KeyDown)(MailScreen *mailScreen, int keyCode, int, int);


inline void (*old_MailScreen_MailScreen)(MailScreen *mailScreen, LawnApp *lawnApp);

inline void (*old_MailScreen_AddedToManager)(MailScreen *mailScreen, Sexy::WidgetManager *widgetManager);

inline void (*old_MailScreen_RemovedFromManager)(MailScreen *mailScreen, Sexy::WidgetManager *widgetManager);

inline void (*old_MailScreen_Delete2)(MailScreen *mailScreen);

inline void (*old_MailScreen_ButtonPress)(MailScreen *mailScreen, int id);

inline void (*old_MailScreen_ButtonDepress)(MailScreen *mailScreen, int id);

inline void (*old_MailScreen_MouseDown)(MailScreen *mailScreen, int x, int y, int theClickCount);

inline void (*old_MailScreen_MouseDrag)(MailScreen *mailScreen, int x, int y);

inline void (*old_MailScreen_MouseUp)(MailScreen *mailScreen, int x, int y);

#endif // PVZ_LAWN_WIDGET_MAIL_SCREEN_H
