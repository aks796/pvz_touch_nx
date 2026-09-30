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

#include "PvZ/TodLib/Effect/FilterEffect.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/SexyAppFramework/Graphics/MemoryImage.h"
#include "PvZ/SexyAppFramework/SexyAppBase.h"
#include "PvZ/TodLib/Common/TodCommon.h"

#include <unordered_map>

using namespace Sexy;

MemoryImage *FilterEffectCreateImage(Image *theImage, FilterEffect theFilterEffect) {
    // MemoryImage* aImage = new MemoryImage();
    MemoryImage *aImage = static_cast<MemoryImage *>((gLawnApp)->CopyImage(theImage));
    aImage->mWidth = theImage->mWidth;
    aImage->mHeight = theImage->mHeight;
    FixPixelsOnAlphaEdgeForBlending(aImage);

    switch (theFilterEffect) {
        case FilterEffect::FILTEREFFECT_WASHED_OUT:
            FilterEffectDoWashedOut(aImage);
            break;
        case FilterEffect::FILTEREFFECT_LESS_WASHED_OUT:
            FilterEffectDoLessWashedOut(aImage);
            break;
        case FilterEffect::FILTEREFFECT_WHITE:
            FilterEffectDoWhite(aImage);
            break;
        case FilterEffect::FILTEREFFECT_CUSTOM:
            FilterEffectDoCustom(aImage, {142, 146, 232, 92}); // 仅MainMenu显示房子雾蒙蒙效果时用到
            break;
        default:
            break;
    }
    // ++memoryImage->mBitsChangedCount;
    aImage->BitsChanged();
    aImage->mNumCols = theImage->mNumCols;
    aImage->mNumRows = theImage->mNumRows;
    return aImage;
}

static std::unordered_map<Sexy::Image *, Sexy::Image *> gFilterEffectMaps[FilterEffect::NUM_FILTEREFFECT];
Image *FilterEffectGetImage(Image *theImage, FilterEffect theFilterEffect) {
    // 变灰的植物贴图在这里处理
    if (theImage == nullptr) {
        return nullptr;
    }

    if (theFilterEffect == FilterEffect::FILTEREFFECT_NONE) {
        return theImage;
    }

    std::unordered_map<Sexy::Image *, Sexy::Image *> &currentMap = gFilterEffectMaps[theFilterEffect];
    auto it = currentMap.find(theImage);
    if (it != currentMap.end()) {
        return it->second;
    } else {
        Sexy::Image *aFilterEffectImage = FilterEffectCreateImage(theImage, theFilterEffect);
        currentMap.emplace(theImage, aFilterEffectImage);
        return aFilterEffectImage;
    }
}

void FilterEffectDoCustom(Sexy::MemoryImage *image, const Sexy::Color &color) {
    const int mix = color.mAlpha; // 92
    const int keep = 255 - mix;   // 163

    for (int y = 0; y < image->mHeight; ++y) {
        for (int x = 0; x < image->mWidth; ++x) {
            unsigned long &pixel = image->mBits[y * image->mWidth + x];

            const unsigned int a = (pixel >> 24) & 0xFF;
            const unsigned int r = (pixel >> 16) & 0xFF;
            const unsigned int g = (pixel >> 8) & 0xFF;
            const unsigned int b = pixel & 0xFF;

            const unsigned int outR = (r * keep + color.mRed * mix + 127) / 255;
            const unsigned int outG = (g * keep + color.mGreen * mix + 127) / 255;
            const unsigned int outB = (b * keep + color.mBlue * mix + 127) / 255;

            pixel = (a << 24) | (outR << 16) | (outG << 8) | outB;
        }
    }
}

void FilterEffectDisposeForApp() {}