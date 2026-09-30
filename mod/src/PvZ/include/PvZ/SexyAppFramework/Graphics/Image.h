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

#ifndef PVZ_SEXYAPPFRAMEWORK_GRAPHICS_IMAGE_H
#define PVZ_SEXYAPPFRAMEWORK_GRAPHICS_IMAGE_H

#include "PvZ/SexyAppFramework/Misc/SexyVertex2D.h"
#include "../Misc/Common.h"
#include "../Misc/Point.h"
#include "../Misc/Rect.h"
#include "Color.h"

class LawnApp;

namespace Sexy {

class SexyMatrix3;
class DeviceImage;

class Image {
public:
    struct ImageVTable {
        void (*completeDestructor)(Image *self);                                                                                                                               // 0x00
        void (*deletingDestructor)(Image *self);                                                                                                                               // 0x04
        MemoryImage *(*AsMemoryImage)(Image *self);                                                                                                                            // 0x08
        DeviceImage *(*AsDeviceImage)(Image *self);                                                                                                                            // 0x0C
        int (*GetWidth)(Image *self);                                                                                                                                          // 0x10
        int (*GetHeight)(Image *self);                                                                                                                                         // 0x14
        int (*GetPitch)(Image *self);                                                                                                                                          // 0x18
        int (*GetStride)(Image *self);                                                                                                                                         // 0x1C
        PixelFormat (*GetPixelFormat)(Image *self);                                                                                                                            // 0x20
        PixelFormat (*GetPixelFormatHint)(Image *self);                                                                                                                        // 0x24
        void (*SetPixelFormatHint)(Image *self, PixelFormat format);                                                                                                           // 0x28
        void (*Finalize)(Image *self);                                                                                                                                         // 0x2C
        void (*SetBits)(Image *self, unsigned int *bits, int width, int height, bool ownsBits);                                                                                // 0x30
        unsigned int *(*GetBits)(Image *self);                                                                                                                                 // 0x34
        unsigned int (*GetPixel)(Image *self, int x, int y);                                                                                                                   // 0x38
        unsigned int *(*GetPixels)(Image *self);                                                                                                                               // 0x3C
        void (*PolyFill3D)(Image *self, const TPoint<int> *points, int pointCount, const TRect<int> *clipRect, const Color &color, int drawMode, int tx, int ty, bool convex); // 0x40
        void (*FillRect)(Image *self, const TRect<int> &rect, const Color &color, int drawMode);                                                                               // 0x44
        void (*DrawRect)(Image *self, const TRect<int> &rect, const Color &color, int drawMode);                                                                               // 0x48
        void (*ClearRect)(Image *self, const TRect<int> &rect);                                                                                                                // 0x4C
        void (*DrawLine)(Image *self, double x1, double y1, double x2, double y2, const Color &color, int drawMode);                                                           // 0x50
        void (*DrawLineAA)(Image *self, double x1, double y1, double x2, double y2, const Color &color, int drawMode);                                                         // 0x54
        void (*FillScanLines)(Image *self, int *spans, int spanCount, const Color &color, int drawMode);                                                                       // 0x58
        void (*FillScanLinesWithCoverage)(
            Image *self, int *spans, int spanCount, const Color &color, int drawMode, const unsigned char *coverage, int coverageX, int coverageY, int coverageWidth, int coverageHeight); // 0x5C
        void (*Blt)(Image *self, Image *source, int x, int y, const TRect<int> &sourceRect, const Color &color, int drawMode);                                                             // 0x60
        void (*BltF)(Image *self, Image *source, float x, float y, const TRect<int> &sourceRect, const TRect<int> &clipRect, const Color &color, int drawMode);                            // 0x64
        void (*BltRotated)(Image *self,
                           Image *source,
                           float x,
                           float y,
                           const TRect<int> &sourceRect,
                           const TRect<int> &clipRect,
                           const Color &color,
                           int drawMode,
                           double rotation,
                           float rotationCenterX,
                           float rotationCenterY); // 0x68
        void (*StretchBlt)(
            Image *self, Image *source, const TRect<int> &destinationRect, const TRect<int> &sourceRect, const TRect<int> &clipRect, const Color &color, int drawMode, bool fastStretch); // 0x6C
        void (*BltMatrix)(Image *self,
                          Image *source,
                          float x,
                          float y,
                          const SexyMatrix3 &matrix,
                          const TRect<int> &sourceRect,
                          const Color &color,
                          int drawMode,
                          const TRect<int> &clipRect,
                          bool linearBlend,
                          bool theMirror); // 0x70
        void (*BltTrianglesTex)(
            Image *self, Image *source, const SexyVertex2D (*triangles)[3], int triangleCount, const TRect<int> &clipRect, const Color &color, int drawMode, float tx, float ty, bool blend); // 0x74
        void (*BltMirror)(Image *self, Image *source, int x, int y, const TRect<int> &sourceRect, const Color &color, int drawMode);                                                          // 0x78
        void (*StretchBltMirror)(
            Image *self, Image *source, const TRect<int> &destinationRect, const TRect<int> &sourceRect, const TRect<int> &clipRect, const Color &color, int drawMode, bool fastStretch); // 0x7C
        void (*Palletize)(Image *self);                                                                                                                                                   // 0x80
        void (*Flip)(Image *self, unsigned int flags);                                                                                                                                    // 0x84
        bool (*HasTransform)(Image *self);                                                                                                                                                // 0x88
        void (*PushTransform)(Image *self, const SexyMatrix3 &transform, bool concatenate);                                                                                               // 0x8C
        void (*PopTransform)(Image *self);                                                                                                                                                // 0x90
        void (*SetWrapMode)(Image *self, int wrapModeX, int wrapModeY);                                                                                                                   // 0x94
        bool (*Is3D)(Image *self);                                                                                                                                                        // 0x98
    };

public:
    void **vTable;                              // 0
    int placeHolder[2];                         // 1 ~ 2
    bool unkBool;                               // 12
    int unkMmWidthems1[2];                      // 4 ~ 5
    homura::Storage<pvzstl::string> stringUnk1; // 6
    homura::Storage<pvzstl::string> stringUnk2; // 7
    homura::Storage<pvzstl::string> stringUnk3; // 8
    int mWidth;                                 // 9
    int mHeight;                                // 10
    int mPitch;                                 // 11
    int mStride;                                // 12
    PixelFormat mFormat;                        // 13
    PixelFormat mPixelFormatHint;               // 14
    int mNumRows;                               // 15
    int mNumCols;                               // 16
    int *mAnimInfo;                             // 17
    int unk[6];                                 // 18 ~ 23
    int mHorizonWrapMode;                       // 24
    int mVertWrapMode;                          // 25
    int mTag;                                   // 26
                                                // 大小27个整数

    const ImageVTable *GetVTable() const {
        return (ImageVTable *)vTable;
    }
    int GetWidth() const;
    int GetHeight() const;
    int GetCelWidth() const;  // returns the width of just 1 cel in a strip of images
    int GetCelHeight() const; // like above but for vertical strips
    void PushTransform(const SexyMatrix3 &theTransform, bool concatenate);
    void PopTransform();

protected:
    Image() = default;
    ~Image() = default;
};

} // namespace Sexy

inline void (*old_Sexy_Image_PushTransform)(Sexy::Image *image, const Sexy::SexyMatrix3 &theTransform, bool concatenate);

inline void (*old_Sexy_Image_PopTransform)(Sexy::Image *image);


#endif // PVZ_SEXYAPPFRAMEWORK_GRAPHICS_IMAGE_H
