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

#include "PvZ/Lawn/Widget/CheatDialogs.h"

#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/LawnDialog.h"

namespace {
void FocusOkButton(LawnDialog *theDialog) {
    Sexy::Widget *ok = theDialog->mLawnYesButton;
    if (ok == nullptr) {
        return;
    }
    // the dialog's own Widget::SetFocus(child, force) (vtable 0xE4)
    Sexy::Widget *self = theDialog;
    reinterpret_cast<void (*)(Sexy::Widget *, Sexy::Widget *, bool)>(self->vTable[0xE4 / 4])(self, ok, true);
}
} // namespace

void CheatCodeDialog_EditWidgetText(LawnDialog *theDialog, int theId, const pvzstl::string &theText) {
    (void)theId;
    (void)theText;
    FocusOkButton(theDialog);
}

void CheatDialog_EditWidgetText(LawnDialog *theDialog, int theId, const pvzstl::string &theText) {
    (void)theId;
    (void)theText;
    FocusOkButton(theDialog);
}
