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

#ifndef PVZ_LAWN_WIDGET_REPLAY_MANAGE_WIDGET_H
#define PVZ_LAWN_WIDGET_REPLAY_MANAGE_WIDGET_H

#include "PvZ/SexyAppFramework/Misc/KeyCodes.h"
#include "PvZ/SexyAppFramework/Widget/Widget.h"

class GameButton;
class LawnApp;
class ReplayListContentWidget;

namespace Sexy {
class ButtonListener;
class ScrollWidget;
} // namespace Sexy

class ReplayManageWidget : public Sexy::Widget {
public:
    enum {
        ReplayManageWidget_Close = 1100,
        ReplayManageWidget_Import = 1101,
        ReplayManageWidget_Export = 1102,
        ReplayManageWidget_Delete = 1103,
        ReplayManageWidget_Play = 1104,
    };

    LawnApp *mApp;
    Sexy::ButtonListener *mButtonListener;
    Sexy::ScrollWidget *mScrollWidget;
    ReplayListContentWidget *mScrollContent;
    GameButton *mCloseButton;
    GameButton *mImportButton;
    GameButton *mExportButton;
    GameButton *mDeleteButton;
    GameButton *mPlayButton;
    bool mZombieBackground;
    int mSelectedReplayIndex;
    bool mNeedRefreshList;

    explicit ReplayManageWidget(LawnApp *app, Sexy::ButtonListener *buttonListener);
    ~ReplayManageWidget();

    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    void Draw(Sexy::Graphics *g);
    void SelectReplayIndex(int index);
    void RefreshReplayList();
    void RequestImportReplay();
    void RequestExportReplay() const;
    void DeleteSelectedReplay();
    void PlaySelectedReplay();
    void StartReplayByIndex(int index) const;

    // Switch port: the controller (WaitForSecondPlayerDialog::KeyDown): up/down
    // pick a replay, left/right a button, A presses it (or plays), B closes.
    bool PadKeyDown(Sexy::KeyCode theKey);

protected:
    void _destructor();
    void _destructor2();
};

namespace replayui {
void OnReplayImportFinished(bool success, const char *message);
void OnReplayExportFinished(bool success, const char *message);
} // namespace replayui

#endif // PVZ_LAWN_WIDGET_REPLAY_MANAGE_WIDGET_H
