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

#ifndef PVZ_LAWN_SWITCH_PAD_H
#define PVZ_LAWN_SWITCH_PAD_H

#include "PvZ/STL/string.h"
#include "PvZ/SexyAppFramework/Misc/KeyCodes.h"
#include "PvZ/SexyAppFramework/Misc/Rect.h"

class LawnApp;

namespace Sexy {
class Graphics;
} // namespace Sexy

// Switch port: the controller in the lists the engine moves through by the
// arrow keys alone (the challenge and VS mode lists). The D-pad comes as the
// arrow keys and repeats while held (the wrapper); the left stick comes as
// its own key codes, once per push.
namespace switchpad {

// the left stick's and the D-pad's own key codes as the arrow keys; any other
// key as it is
Sexy::KeyCode AsArrowKey(Sexy::KeyCode theKey);

// the left stick held up (-1), down (1) or neither (0), on any controller
int StickHeldVertical(LawnApp *theApp);

// A string of this port's ([SWITCH_CLOSE]: "<B> Close", the English layer's),
// else the game's own (theFallbackKey): the port's keys are only in the
// English layer, so not in Chinese.
pvzstl::string Label(const char *theSwitchKey, const char *theFallbackKey);

// The controller's highlight on a stone button: the purple glow the main
// menu's signs light up with (Adventure, More Ways to Play), close around it
// and breathing slowly. theTick: frames, for the breath.
void DrawPurpleGlow(Sexy::Graphics *g, const Sexy::Rect &theRect, int theTick);
// The Zombatar editor's highlight: the thing lit from within, a soft warm halo.
void DrawWarmGlow(Sexy::Graphics *g, const Sexy::Rect &theRect, int theTick);

// A stone button as the game draws one (DrawStoneButton: its left end, whole
// middle pieces -- as many as fit best -- and its right end, from its top
// left): where the stone is, for a button of width theWidth at x, y.
Sexy::Rect StoneRect(int x, int y, int theWidth, bool theDown);
// The main menu signs' purple glow around such a stone, following its shape
// (the stone's own pieces, purple, a little out all round). Drawn before the
// button, which covers the middle of it. theTick: frames, for its breath.
void DrawStoneGlow(Sexy::Graphics *g, int x, int y, int theWidth, bool theDown, int theTick);

// A direction held: nothing at first (its push has moved once already), then
// the direction again and again, as a key held on a keyboard.
class Repeater {
public:
    int Step(int theHeld); // theHeld: -1, 0 or 1, each update; the step to take, or 0

private:
    int mDir = 0;
    int mTicks = 0;
};

} // namespace switchpad

#endif // PVZ_LAWN_SWITCH_PAD_H
