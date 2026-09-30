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

#ifndef PVZ_LAWN_WIDGET_CHEAT_DIALOGS_H
#define PVZ_LAWN_WIDGET_CHEAT_DIALOGS_H

#include "PvZ/STL/string.h"

class LawnDialog;

// Switch port: the cheat code and level jump dialogs. Their edit field's Enter
// (EditWidgetText) presses OK, and the Switch keyboard's OK arrives as that
// Enter -- so the dialog closed before the code could be seen. Here Enter moves
// to the dialog's OK button instead: the text stays, A on OK applies it.
void CheatCodeDialog_EditWidgetText(LawnDialog *theDialog, int theId, const pvzstl::string &theText);
void CheatDialog_EditWidgetText(LawnDialog *theDialog, int theId, const pvzstl::string &theText);

inline void (*old_CheatCodeDialog_EditWidgetText)(LawnDialog *theDialog, int theId, const pvzstl::string &theText);
inline void (*old_CheatDialog_EditWidgetText)(LawnDialog *theDialog, int theId, const pvzstl::string &theText);

#endif // PVZ_LAWN_WIDGET_CHEAT_DIALOGS_H
