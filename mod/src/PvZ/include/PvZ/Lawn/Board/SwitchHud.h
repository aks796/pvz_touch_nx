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

#ifndef PVZ_LAWN_BOARD_SWITCH_HUD_H
#define PVZ_LAWN_BOARD_SWITCH_HUD_H

class Board;
class LawnApp;
class GamepadControls;
class GameButton;

namespace Sexy {
class Graphics;
}

// Switch port: the lawn's controller extras.
//
// FAST FORWARD (Y, or a tap on its button): the game at twice the speed, as
// Plants vs. Zombies: Replanted's speed button -- one more Board update per
// frame, the way the mod's own game speed cheat runs (Board::Update). Only
// where nobody else shares the lawn's clock: single player and co-op on this
// console, not VS (local or over the network), not replays. When the cheat's
// speed is set, the cheat's wins. Its button and the game's Menu (pause)
// button are the game's own stone buttons in the screen's top right corner,
// as Plants vs. Zombies 2 has its pause button, each with its controller
// button at its foot. Fast forward on: the stone pressed in deeper, in shadow,
// its label lit by the game's own lawn-stone glow.
//
// THE SUN VACUUM (ZL / ZR held), the Xbox 360 edition's: every sun and coin on
// the lawn flies to the cursor while a trigger is held; the cursor passing
// near one picks it up as well (the engine's own). Sun and coins are not
// collected by themselves, as the TV edition does (Coin::Update). In VS the
// plant player's pulls the sun and the zombie player's the brains (the
// computer opponent collects its own, VSActionAIExecutor). With the controller
// scheme, offline. Online the game's own ZL / ZR (every sun at once) and its
// auto-collection stay: each console collects its own copy of every sun when
// it is old enough, which is what keeps the two in step -- no message in the
// game's network protocol says who picked up a sun.
namespace switchhud {

bool FastForwardAllowed(LawnApp *theApp);
bool FastForwardOn(LawnApp *theApp); // on and allowed here
void ToggleFastForward(LawnApp *theApp);

// the Xbox 360 edition's sun collecting applies (the controller scheme, offline)
bool XboxSunCollecting(LawnApp *theApp);
// ZL / ZR pressed on the lawn: the vacuum, or (online) the game's own gathering
void SunTrigger(GamepadControls *theControls);
// each update of a player's controls: the vacuum while its trigger is held
void VacuumUpdate(GamepadControls *theControls);

// The level's Menu button (Board::mBoardMenuButton) goes to the corner.
// Board::UpdateButtons.
void PlaceMenuButton(Board *theBoard);

void Draw(Board *theBoard, Sexy::Graphics *g);
// after the game draws a button (GameButton::Draw): the Menu button's + on it
void DrawOnButton(GameButton *theButton, Sexy::Graphics *g);
// the hook on GameButton::Draw: the game's drawing, then DrawOnButton
void GameButton_Draw(GameButton *theButton, Sexy::Graphics *g);
inline void (*old_GameButton_Draw)(GameButton *theButton, Sexy::Graphics *g);
// a tap on the fast forward button: taken (true) or not; the rest of that
// touch (drag, up) is then taken too
bool MouseDown(Board *theBoard, int x, int y);
bool TouchTaken(bool theEnds);

} // namespace switchhud

#endif // PVZ_LAWN_BOARD_SWITCH_HUD_H
