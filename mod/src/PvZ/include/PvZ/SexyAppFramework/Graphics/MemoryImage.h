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

#ifndef PVZ_SEXYAPPFRAMEWORK_GRAPHICS_MEMORY_IMAGE_H
#define PVZ_SEXYAPPFRAMEWORK_GRAPHICS_MEMORY_IMAGE_H

#include "PvZ/STL/map.h"
#include "PvZ/STL/set.h"
#include "Image.h"

namespace Sexy {

const unsigned long MEMORYCHECK_ID = 0x4BEEFADE;

class SexyMatrix3;
class DeviceImage;

class MemoryImage : public Image {
    struct MemoryImageVTable {
        void (*completeDestructor)(MemoryImage *self);                                                                                                                         // 0x000
        void (*deletingDestructor)(MemoryImage *self);                                                                                                                         // 0x004
        MemoryImage *(*AsMemoryImage)(MemoryImage *self);                                                                                                                      // 0x008
        DeviceImage *(*AsDeviceImage)(Image *self);                                                                                                                            // 0x00C
        int (*GetWidth)(Image *self);                                                                                                                                          // 0x010
        int (*GetHeight)(Image *self);                                                                                                                                         // 0x014
        int (*GetPitch)(MemoryImage *self);                                                                                                                                    // 0x018
        int (*GetStride)(MemoryImage *self);                                                                                                                                   // 0x01C
        PixelFormat (*GetPixelFormat)(MemoryImage *self);                                                                                                                      // 0x020
        PixelFormat (*GetPixelFormatHint)(Image *self);                                                                                                                        // 0x024
        void (*SetPixelFormatHint)(Image *self, PixelFormat format);                                                                                                           // 0x028
        void (*Finalize)(MemoryImage *self);                                                                                                                                   // 0x02C
        void (*SetBits)(MemoryImage *self, unsigned int *bits, int width, int height, bool commitBits);                                                                        // 0x030
        unsigned int *(*GetBits)(MemoryImage *self);                                                                                                                           // 0x034
        unsigned int (*GetPixel)(MemoryImage *self, int x, int y);                                                                                                             // 0x038
        void *(*GetPixels)(MemoryImage *self);                                                                                                                                 // 0x03C
        void (*PolyFill3D)(Image *self, const TPoint<int> *points, int pointCount, const TRect<int> *clipRect, const Color &color, int drawMode, int tx, int ty, bool convex); // 0x040
        void (*FillRect)(MemoryImage *self, const TRect<int> &rect, const Color &color, int drawMode);                                                                         // 0x044
        void (*DrawRect)(Image *self, const TRect<int> &rect, const Color &color, int drawMode);                                                                               // 0x048
        void (*ClearRect)(MemoryImage *self, const TRect<int> &rect);                                                                                                          // 0x04C
        void (*DrawLine)(MemoryImage *self, double x1, double y1, double x2, double y2, const Color &color, int drawMode);                                                     // 0x050
        void (*DrawLineAA)(MemoryImage *self, double x1, double y1, double x2, double y2, const Color &color, int drawMode);                                                   // 0x054
        void (*FillScanLines)(Image *self, int *spans, int spanCount, const Color &color, int drawMode);                                                                       // 0x058
        void (*FillScanLinesWithCoverage)(
            Image *self, int *spans, int spanCount, const Color &color, int drawMode, const unsigned char *coverage, int coverageX, int coverageY, int coverageWidth, int coverageHeight); // 0x05C
        void (*Blt)(MemoryImage *self, Image *source, int x, int y, const TRect<int> &sourceRect, const Color &color, int drawMode);                                                       // 0x060
        void (*BltF)(MemoryImage *self, Image *source, float x, float y, const TRect<int> &sourceRect, const TRect<int> &clipRect, const Color &color, int drawMode);                      // 0x064
        void (*BltRotated)(MemoryImage *self,
                           Image *source,
                           float x,
                           float y,
                           const TRect<int> &sourceRect,
                           const TRect<int> &clipRect,
                           const Color &color,
                           int drawMode,
                           double rotation,
                           float centerX,
                           float centerY); // 0x068
        void (*StretchBlt)(
            MemoryImage *self, Image *source, const TRect<int> &destinationRect, const TRect<int> &sourceRect, const TRect<int> &clipRect, const Color &color, int drawMode, bool fastStretch); // 0x06C
        void (*BltMatrix)(MemoryImage *self,
                          Image *source,
                          float x,
                          float y,
                          const SexyMatrix3 &matrix,
                          const TRect<int> &sourceRect,
                          const Color &color,
                          int drawMode,
                          const TRect<int> &clipRect,
                          bool linearBlend,
                          bool mirror); // 0x070
        void (*BltTrianglesTex)(MemoryImage *self,
                                Image *source,
                                const SexyVertex2D (*triangles)[3],
                                int triangleCount,
                                const TRect<int> &clipRect,
                                const Color &color,
                                int drawMode,
                                float tx,
                                float ty,
                                bool blend);                                                                                               // 0x074
        void (*BltMirror)(MemoryImage *self, Image *source, int x, int y, const TRect<int> &sourceRect, const Color &color, int drawMode); // 0x078
        void (*StretchBltMirror)(
            MemoryImage *self, Image *source, const TRect<int> &destinationRect, const TRect<int> &sourceRect, const TRect<int> &clipRect, const Color &color, int drawMode, bool fastStretch); // 0x07C
        void (*Palletize)(MemoryImage *self);                                                                                                                                                   // 0x080
        void (*Flip)(Image *self, unsigned int flags);                                                                                                                                          // 0x084
        bool (*HasTransform)(Image *self);                                                                                                                                                      // 0x088
        void (*PushTransform)(Image *self, const SexyMatrix3 &transform, bool concatenate);                                                                                                     // 0x08C
        void (*PopTransform)(Image *self);                                                                                                                                                      // 0x090
        void (*SetWrapMode)(Image *self, int horizontal, int vertical);                                                                                                                         // 0x094
        bool (*Is3D)(Image *self);                                                                                                                                                              // 0x098
        void *(*GetNativeAlphaData)(MemoryImage *self, int *display);                                                                                                                           // 0x09C
        unsigned char *(*GetRLAlphaData)(MemoryImage *self);                                                                                                                                    // 0x0A0
        unsigned char *(*GetRLAdditiveData)(MemoryImage *self, int *display);                                                                                                                   // 0x0A4
        void (*PurgeBits)(MemoryImage *self);                                                                                                                                                   // 0x0A8
        void (*DeleteSWBuffers)(MemoryImage *self);                                                                                                                                             // 0x0AC
        void (*Delete3DBuffers)(MemoryImage *self);                                                                                                                                             // 0x0B0
        void (*DeleteExtraBuffers)(MemoryImage *self);                                                                                                                                          // 0x0B4
        void (*ReInit)(MemoryImage *self);                                                                                                                                                      // 0x0B8
        void (*BitsChanged)(MemoryImage *self);                                                                                                                                                 // 0x0BC
        void (*CommitBits)(MemoryImage *self);                                                                                                                                                  // 0x0C0
        void (*DeleteNativeData)(MemoryImage *self);                                                                                                                                            // 0x0C4
        void (*Attach)(MemoryImage *self, MemoryImage *subImage, int x, int y, int width, int height);                                                                                          // 0x0C8
        void (*Detach)(MemoryImage *self, bool copyBits);                                                                                                                                       // 0x0CC
        void (*DetachSubImages)(MemoryImage *self, bool copyBits);                                                                                                                              // 0x0D0
        MemoryImage *(*CreateSubImage)(MemoryImage *self, int x, int y, int width, int height);                                                                                                 // 0x0D4
        void (*NormalStretchBltMirror)(
            MemoryImage *self, Image *source, const TRect<int> &destinationRect, const TRect<int> &sourceRect, const TRect<int> &clipRect, const Color &color, bool fastStretch); // 0x0D8
        void (*AdditiveStretchBltMirror)(
            MemoryImage *self, Image *source, const TRect<int> &destinationRect, const TRect<int> &sourceRect, const TRect<int> &clipRect, const Color &color, bool fastStretch); // 0x0DC
        void (*FillScanLinesWithCoverageRenderDevice)(MemoryImage *self,
                                                      int *spans,
                                                      int spanCount,
                                                      const Color &color,
                                                      int drawMode,
                                                      const unsigned char *coverage,
                                                      int coverageX,
                                                      int coverageY,
                                                      int coverageWidth,
                                                      int coverageHeight);                                                                   // 0x0E0
        void (*Clear)(MemoryImage *self);                                                                                                    // 0x0E4
        void (*TakeBits)(MemoryImage *self, unsigned int *bits, int width, int height, bool commitBits);                                     // 0x0E8
        void (*TakePixels)(MemoryImage *self, PixelFormat format, void *pixels, int width, int height, bool commitBits);                     // 0x0EC
        void (*Create)(MemoryImage *self, int width, int height);                                                                            // 0x0F0
        void (*DrawLineEx)(MemoryImage *self, double x1, double y1, double x2, double y2, const Color &color, int drawMode, bool antiAlias); // 0x0F4
        void (*BltTriangles)(MemoryImage *self,
                             Image *source,
                             const SexyVertex2D (*triangles)[3],
                             int triangleCount,
                             const Color &color,
                             int drawMode,
                             float tx,
                             float ty,
                             bool blend,
                             const TRect<int> *clipRect); // 0x0F8
        void (*BltStretched)(MemoryImage *self,
                             Image *source,
                             const TRect<int> &destinationRect,
                             const TRect<int> &sourceRect,
                             const TRect<int> &clipRect,
                             const Color &color,
                             int drawMode,
                             bool fastStretch,
                             bool mirror);                                                                                                                            // 0x0FC
        void (*SetImageMode)(MemoryImage *self, bool mode1, bool mode2);                                                                                              // 0x100
        void (*SetVolatile)(MemoryImage *self, bool isVolatile);                                                                                                      // 0x104
        void (*ForcePalletize)(MemoryImage *self);                                                                                                                    // 0x108
        unsigned int *(*GetPallete)(MemoryImage *self);                                                                                                               // 0x10C
        int *(*Get3D)(MemoryImage *self);                                                                                                                             // 0x110
        bool (*CanFillPoly)(MemoryImage *self);                                                                                                                       // 0x114
        bool (*CreateContext)(MemoryImage *self, Image *image, const int &context);                                                                                   // 0x118
        void (*DeleteContext)(MemoryImage *self, const int &context);                                                                                                 // 0x11C
        void (*SetCurrentContext)(MemoryImage *self, const int &context);                                                                                             // 0x120
        int (*GetCurrentContext)(const MemoryImage *self);                                                                                                            // 0x124
        void (*PushState)(MemoryImage *self);                                                                                                                         // 0x128
        void (*PopState)(MemoryImage *self);                                                                                                                          // 0x12C
        void (*FillPoly)(MemoryImage *self, const TPoint<int> *points, int pointCount, const TRect<int> *clipRect, const Color &color, int drawMode, int tx, int ty); // 0x130
        void (*WriteToPng)(MemoryImage *self, std::string fileName);                                                                                                  // 0x134
        void (*WriteToJPEG)(MemoryImage *self, std::string fileName);                                                                                                 // 0x138
    };

public:
    void **mRenderDeviceVTable;                             // 27
    LawnApp *mApp;                                          // 28
    unsigned int *mPixels;                                  // 29
    unsigned long *mBits;                                   // 30
    int mBitsChangedCount;                                  // 31
    int *mD3DData;                                          // 0x80，DWORD[32]
    unsigned int mD3DFlags;                                 // 0x84，DWORD[33]
    int *mColorTable;                                       // 0x88，DWORD[34]，256 个 uint32_t
    int *mColorIndices;                                     // 0x8C，DWORD[35]，宽×高个 uint8_t
    bool mForcedMode;                                       // 144
    bool mHasTrans;                                         // 145
    bool mHasAlpha;                                         // 146
    bool mIsAlphaOnly;                                      // 147
    bool mIsVolatile;                                       // 148
    bool mPurgeBits;                                        // 149
    bool mWantPal;                                          // 150
    int *mNativeAlphaData;                                  // 0x98，DWORD[38]
    int *mRLAlphaData;                                      // 0x9C，DWORD[39]
    int *mRLAdditiveData;                                   // 0xA0，DWORD[40]
    bool mBitsChanged;                                      // 0xA4
    MemoryImage *mParent;                                   // 0xA8，DWORD[42]
    int mParentX;                                           // 0xAC，DWORD[43]
    int mParentY;                                           // 0xB0，DWORD[44]
    homura::Storage<pvzstl::set<MemoryImage *>> mSubImages; // 0xB4，DWORD[45] ~ DWORD[50]
    int *mSubImagesCritSect;                                // 0xCC，DWORD[51]
    homura::Storage<std::vector<void *>> mTriRep[2];        // 0xD0，DWORD[52] ~ DWORD[57]
    // 大小58个整数

    unsigned long *GetBits() {
        return reinterpret_cast<unsigned long *(*)(MemoryImage *)>(Sexy_MemoryImage_GetBitsAddr)(this);
    }
    void Create(int theWidth, int theHeight) {
        reinterpret_cast<void (*)(MemoryImage *, int, int)>(Sexy_MemoryImage_CreateAddr)(this, theWidth, theHeight);
    }
    void SetImageMode(bool hasTrans, bool hasAlpha) {
        reinterpret_cast<void (*)(MemoryImage *, bool, bool)>(Sexy_MemoryImage_SetImageModeAddr)(this, hasTrans, hasAlpha);
    }
    void WriteToPng(pvzstl::string theString) {
        reinterpret_cast<void (*)(MemoryImage *, pvzstl::string)>(Sexy_MemoryImage_WriteToPngAddr)(this, std::move(theString));
    }
    void WriteToJPEG(int *theString) {
        reinterpret_cast<void (*)(MemoryImage *, int *)>(Sexy_MemoryImage_WriteToJPEGAddr)(this, theString);
    }
    void BitsChanged() {
        reinterpret_cast<void (*)(MemoryImage *)>(Sexy_MemoryImage_BitsChangedAddr)(this);
    }

    const MemoryImageVTable *GetVTable() const {
        return (MemoryImageVTable *)vTable;
    }

    MemoryImage() {
        _constructor();
    }
    ~MemoryImage() {
        _destructor();
    }

    void ClearRect(const Rect &theRect);
    void PushTransform(const SexyMatrix3 &theTransform, bool concatenate);
    void PopTransform();

protected:
    void _constructor() {
        reinterpret_cast<void (*)(MemoryImage *)>(Sexy_MemoryImage_MemoryImageAddr)(this);
    }
    void _destructor() {
        reinterpret_cast<void (*)(MemoryImage *)>(Sexy_MemoryImage_DeleteAddr)(this);
    }
};

} // namespace Sexy

inline void (*old_Sexy_MemoryImage_PushTransform)(Sexy::MemoryImage *image, const Sexy::SexyMatrix3 &theTransform, bool concatenate);

inline void (*old_Sexy_MemoryImage_PopTransform)(Sexy::MemoryImage *image);


#endif // PVZ_SEXYAPPFRAMEWORK