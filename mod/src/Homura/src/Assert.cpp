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

#include "Homura/Assert.h"
#include "Homura/Logger.h"

#include <android/log.h>

#include <cinttypes>
#include <cstdlib>

void homura::detail::AssertionFailedImpl(std::source_location location, const char *expression) {
    __android_log_print(ANDROID_LOG_FATAL,
                        Logger::PVZ_LOG_TAG,
                        "[%s(%" PRIuLEAST32 ":%" PRIuLEAST32 ")][%s] Assertion \"%s\" failed",
                        location.file_name(),
                        location.line(),
                        location.column(),
                        location.function_name(),
                        expression);
    std::abort();
}

void homura::detail::AssertionFailedImpl(std::source_location location, const char *expression, const char *message) {
    __android_log_print(ANDROID_LOG_FATAL,
                        Logger::PVZ_LOG_TAG,
                        "[%s(%" PRIuLEAST32 ":%" PRIuLEAST32 ")][%s] Assertion \"%s\" failed: %s",
                        location.file_name(),
                        location.line(),
                        location.column(),
                        location.function_name(),
                        expression,
                        message);
    std::abort();
}
