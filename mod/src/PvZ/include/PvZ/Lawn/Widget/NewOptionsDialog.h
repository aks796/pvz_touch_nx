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

#ifndef PVZ_LAWN_WIDGET_NEWOPTIONSDIALOG_H
#define PVZ_LAWN_WIDGET_NEWOPTIONSDIALOG_H

#include "LawnDialog.h"

class NewOptionsDialog : public LawnDialog {
protected:
    enum {
        NewOptionsDialog_House,
        NewOptionsDialog_Useless,
        NewOptionsDialog_HelpDialog,
        NewOptionsDialog_Almanac,
        NewOptionsDialog_MainMenu,
        NewOptionsDialog_Restart,
        NewOptionsDialog_Update,
        NewOptionsDialog_ClearSecondPlayer,
    };

public:
    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    void ButtonDepress(int theId);
};

inline void (*old_NewOptionsDialog_AddedToManager)(NewOptionsDialog *a1, Sexy::WidgetManager *a2);

inline void (*old_NewOptionsDialog_RemovedFromManager)(NewOptionsDialog *a1, Sexy::WidgetManager *a2);

inline void (*old_NewOptionsDialog_ButtonDepress)(NewOptionsDialog *a1, int a2);

#endif // PVZ_LAWN_WIDGET_NEWOPTIONSDIALOG_H
