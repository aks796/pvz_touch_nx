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

#include "PvZ/SexyAppFramework/SexyAppBase.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/PlayerInfo.h"

#include <unistd.h>

void Sexy::SexyAppBase::_constructor() {
    old_Sexy_SexyAppBase_SexyAppBase(this);
}

bool Sexy::SexyAppBase::UpdateApp() {
    return old_Sexy_SexyAppBase_UpdateApp(this);
}

bool Sexy::SexyAppBase::EraseFile(const pvzstl::string &theFileName) {
    if (disableDeleteUserdata && theFileName.ends_with(".dat")) {
        return true;
    }
    return unlink(theFileName.c_str()) == 0;
}
