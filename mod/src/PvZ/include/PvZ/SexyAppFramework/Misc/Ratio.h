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

#ifndef PVZ_SEXYAPPFRAMEWORK_MISC_RATIO_H
#define PVZ_SEXYAPPFRAMEWORK_MISC_RATIO_H

namespace Sexy {

struct Ratio {
    int mNumerator;
    int mDenominator;

    constexpr Ratio() noexcept
        : mNumerator{1}
        , mDenominator{1} {}

    constexpr Ratio(int theNumerator, int theDenominator) {
        Set(theNumerator, theDenominator);
    }

    constexpr void Set(int theNumerator, int theDenominator) {
        // find the greatest-common-denominator of theNumerator and theDenominator.
        int a = theNumerator;
        int b = theDenominator;
        while (b != 0) {
            int t = b;
            b = a % b;
            a = t;
        }

        // divide by the g-c-d to reduce mNumerator/mDenominator to lowest terms.
        mNumerator = theNumerator / a;
        mDenominator = theDenominator / a;
    }

    constexpr bool operator==(const Ratio &theRatio) const = default;

    constexpr bool operator<(const Ratio &theRatio) const {
        return (mNumerator * theRatio.mDenominator / mDenominator < theRatio.mNumerator) || (mNumerator < theRatio.mNumerator * mDenominator / theRatio.mDenominator);
    }

    constexpr int operator*(int theInt) const {
        return theInt * mNumerator / mDenominator;
    }

    constexpr int operator/(int theInt) const {
        return theInt * mDenominator / mNumerator;
    }
};

constexpr int operator*(int theInt, const Ratio &theRatio) {
    return theInt * theRatio.mNumerator / theRatio.mDenominator;
}

constexpr int operator/(int theInt, const Ratio &theRatio) {
    return theInt * theRatio.mDenominator / theRatio.mNumerator;
}

} // namespace Sexy

#endif // PVZ_SEXYAPPFRAMEWORK_MISC_RATIO_H
