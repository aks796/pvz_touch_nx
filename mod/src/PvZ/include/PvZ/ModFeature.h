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

#ifndef PVZ_MOD_FEATURE_H
#define PVZ_MOD_FEATURE_H

#include <string>

// One cheat menu entry (cheat::lang::*::featureList) changed: featNum is the
// entry's number, value a Spinner's option index or an InputValue, boolean a
// Toggle/CheckBox state (true for an OnceCheckBox), str an InputText's text.
// Main.cpp; the Java menu reaches it through Preferences.Changes, the game's
// own mod menu (ModMenuDialog) directly.
void ApplyModFeature(int featNum, int value, bool boolean, const std::string &str);

#endif // PVZ_MOD_FEATURE_H
