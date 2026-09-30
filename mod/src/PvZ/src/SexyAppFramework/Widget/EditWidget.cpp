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

#include "PvZ/SexyAppFramework/Widget/EditWidget.h"
#include "PvZ/Symbols.h"

namespace {

constexpr int kTextFieldIndex = 66; // this + 264
constexpr int kCursorIndex = 79;    // this + 316
constexpr int kSelectionIndex = 80; // this + 320
constexpr int kEditSinkIndex = 93;  // this + 372
constexpr int kKeyboardShownByteOffset = 416;
constexpr int kEditInterfaceByteOffset = 256;
constexpr int kSinkSyncVtableOffset = 24;

static bool gSyncingEditSink = false;

static int GetStdStringLen(int *self, int index) {
    int strData = self[index];
    if (!strData)
        return 0;
    return *reinterpret_cast<int *>(strData - 12);
}

static void SyncEditSink(Sexy::EditWidget *self) {
    if (!self || gSyncingEditSink)
        return;

    int *data = reinterpret_cast<int *>(self);
    int sink = data[kEditSinkIndex];
    if (!sink)
        return;

    unsigned char keyboardShown = *reinterpret_cast<unsigned char *>(reinterpret_cast<char *>(self) + kKeyboardShownByteOffset);
    if (!keyboardShown)
        return;

    int sinkVtable = *reinterpret_cast<int *>(sink);
    if (!sinkVtable)
        return;

    auto syncFn = reinterpret_cast<void (*)(int, char *)>(*reinterpret_cast<int *>(sinkVtable + kSinkSyncVtableOffset));
    if (!syncFn)
        return;

    gSyncingEditSink = true;
    syncFn(sink, reinterpret_cast<char *>(self) + kEditInterfaceByteOffset);
    gSyncingEditSink = false;
}

} // namespace

int Sexy::EditWidget::ProcessKey(KeyCode theKey, int theChar) {
    int *data = reinterpret_cast<int *>(this);
    int oldLen = GetStdStringLen(data, kTextFieldIndex);
    int oldCursor = data[kCursorIndex];
    int oldSelection = data[kSelectionIndex];

    int ret = old_Sexy_EditWidget_ProcessKey(this, theKey, theChar);

    int newLen = GetStdStringLen(data, kTextFieldIndex);
    int newCursor = data[kCursorIndex];
    int newSelection = data[kSelectionIndex];

    bool changed = oldLen != newLen || oldCursor != newCursor || oldSelection != newSelection;
    if (ret && changed) {
        // Keep IME/native edit sink state aligned with EditWidget local buffer.
        SyncEditSink(this);
    }

    return ret;
}
