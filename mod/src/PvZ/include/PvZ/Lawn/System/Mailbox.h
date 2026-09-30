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

#ifndef PVZ_LAWN_MAILBOX_H
#define PVZ_LAWN_MAILBOX_H

#include "PvZ/Symbols.h"

class LawnApp;

class Mailbox {
public:
    // Mailbox::MailMessage (reverse-engineered from Mailbox_export pseudocode)
    struct MailMessage {
        int mPreviewImageRef[3];  // +0  Sexy::SharedImageRef
        int mBodyImageRef[3];     // +12 Sexy::SharedImageRef
        int mSenderText;          // +24 std::string*
        int mSubjectText;         // +28 std::string*
        int mBodyText;            // +32 std::string*
        int mX;                   // +36
        int mY;                   // +40
        int mRequiredLevel;       // +44
        int mUnknown48;           // +48
        unsigned char mIsSeen;    // +52
        unsigned char mPad53[15]; // +53..67
        unsigned char mFlag68;    // +68
        unsigned char mFlag69;    // +69
        unsigned char mIsValid;   // +70
        unsigned char mPad71;     // +71
    };

    LawnApp *mApp; // +0
    int mUnknown4; // +4

    int mUnreadBegin;       // +8  std::vector<int>::begin
    int mUnreadEnd;         // +12 std::vector<int>::end
    int mUnreadCapacityEnd; // +16 std::vector<int>::capacity end

    int mReadBegin;       // +20 std::vector<int>::begin
    int mReadEnd;         // +24 std::vector<int>::end
    int mReadCapacityEnd; // +28 std::vector<int>::capacity end

    int mMessageMapImplUnk32; // +32 rb_tree impl (unknown slot)
    int mMessageMapHeader;    // +36 rb_tree header node
    int mMessageMapRoot;      // +40 rb_tree root
    int mMessageMapLeftmost;  // +44 rb_tree leftmost
    int mMessageMapRightmost; // +48 rb_tree rightmost
    int mMessageMapNodeCount; // +52 rb_tree node count

    int mManifestPath; // +56 std::string*
    int mLoadState;    // +60 3 => loaded

    int GetMessageByIndex(int theIndex, bool theLoadImage) {
        return reinterpret_cast<int (*)(Mailbox *, int, bool)>(Mailbox_GetMessageByIndexAddr)(this, theIndex, theLoadImage);
    }
    void Update() {
        reinterpret_cast<void (*)(Mailbox *)>(Mailbox_UpdateAddr)(this); // 空函数
    }
    void RefreshMessages() {
        reinterpret_cast<void (*)(Mailbox *)>(Mailbox_RefreshMessagesAddr)(this);
    }

    int GetNumUnseenMessages();

protected:
    friend void InitHookFunction();
};

#endif // PVZ_LAWN_MAILBOX_H
