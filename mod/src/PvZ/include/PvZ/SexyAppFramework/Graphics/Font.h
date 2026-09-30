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

#ifndef PVZ_SEXYAPPFRAMEWORK_GRAPHICS_FONT_H
#define PVZ_SEXYAPPFRAMEWORK_GRAPHICS_FONT_H

#include "Homura/MemberUtils.h"

#include "../Misc/Common.h"
#include "../Misc/Rect.h"
#include "Color.h"

namespace Sexy {

class Graphics;

class Glyph {};

class Font {
public:
    struct FontVTable {
        void (*completeDestructor)(Font *self);                                                                                                                                               // 0x00
        void (*deletingDestructor)(Font *self);                                                                                                                                               // 0x04
        int (*GetAscent)(Font *self);                                                                                                                                                         // 0x08
        int (*GetAscentPadding)(Font *self);                                                                                                                                                  // 0x0C
        int (*GetDescent)(Font *self);                                                                                                                                                        // 0x10
        int (*GetHeight)(Font *self);                                                                                                                                                         // 0x14
        int (*GetLineSpacingOffset)(Font *self);                                                                                                                                              // 0x18
        int (*GetLineSpacing)(Font *self);                                                                                                                                                    // 0x1C
        int (*StringWidth)(Font *self, const pvzstl::string &text);                                                                                                                           // 0x20
        int (*StringWidthUnicode)(Font *self, const pvzstl::wstring &text);                                                                                                                   // 0x24
        int (*CharWidth)(Font *self, int character);                                                                                                                                          // 0x28
        int (*CharWidthKern)(Font *self, int character, int nextCharacter);                                                                                                                   // 0x2C
        void (*DrawString)(Font *self, Graphics *graphics, int x, int y, const pvzstl::string &text, const Color &color, const TRect<int> &clipRect);                                         // 0x30
        void (*DrawStringUnicode)(Font *self, Graphics *graphics, int x, int y, const pvzstl::wstring &text, const Color &color, const TRect<int> &clipRect);                                 // 0x34
        void (*StringToGlyphs)(Font *self, const pvzstl::wstring &text, std::vector<Glyph> &glyphs);                                                                                          // 0x38
        void (*DrawGlyphs)(Font *self, Graphics *graphics, int x, int y, std::vector<Glyph> &glyphs, unsigned int start, unsigned int count, const Color &color, const TRect<int> &clipRect); // 0x3C
        void (*GlyphExtentsVector)(Font *self, const std::vector<Glyph> &glyphs, int &extents);                                                                                               // 0x40
        void (*GlyphExtentsList)(Font *self, const int &glyphsList, int &extents);                                                                                                            // 0x44
        void (*StringExtents)(Font *self, const pvzstl::string &text, int &extents);                                                                                                          // 0x48
        void (*StringExtentsUnicode)(Font *self, const pvzstl::wstring &text, int &extents);                                                                                                  // 0x4C
        bool (*IsComposited)(Font *self);                                                                                                                                                     // 0x50
    };

    void **vTable;
    int mAscent;
    int mAscentPadding;
    int mHeight;
    int mLineSpacingOffset;

    inline int CharWidthKern(int theChar, int thePrevChar) {
        return reinterpret_cast<int (*)(Font *, int, int)>(Sexy_Font_CharWidthKernAddr)(this, theChar, thePrevChar);
    }
    inline int StringWidth(const pvzstl::string &text) {
        // 函数符号固定返回0，虚函数vTable[8]调用才能生效
        return reinterpret_cast<int (*)(Font *, const pvzstl::string &)>(Sexy_Font_StringWidthAddr)(this, text);
    }
    const FontVTable *GetVTable() const {
        return (FontVTable *)vTable;
    }

    int GetHeight() {
        return mHeight;
    }
    int GetAscent() {
        return mAscent;
    }

protected:
    Font() = default;
    ~Font() = default;
};

class ImageFont : public Font {
public:
    struct ImageFontVTable {
        void (*completeDestructor)(ImageFont *self);                                                                                                               // 0x00
        void (*deletingDestructor)(ImageFont *self);                                                                                                               // 0x04
        int (*GetAscent)(Font *self);                                                                                                                              // 0x08
        int (*GetAscentPadding)(Font *self);                                                                                                                       // 0x0C
        int (*GetDescent)(Font *self);                                                                                                                             // 0x10
        int (*GetHeight)(Font *self);                                                                                                                              // 0x14
        int (*GetLineSpacingOffset)(Font *self);                                                                                                                   // 0x18
        int (*GetLineSpacing)(Font *self);                                                                                                                         // 0x1C
        int (*StringWidth)(ImageFont *self, const pvzstl::string &text);                                                                                           // 0x20
        int (*StringWidthUnicode)(ImageFont *self, const pvzstl::wstring &text);                                                                                   // 0x24
        int (*CharWidth)(ImageFont *self, int character);                                                                                                          // 0x28
        int (*CharWidthKern)(ImageFont *self, int character, int previousCharacter);                                                                               // 0x2C
        void (*DrawString)(ImageFont *self, Graphics *graphics, int x, int y, const pvzstl::string &text, const Color &color, const TRect<int> &clipRect);         // 0x30
        void (*DrawStringUnicode)(ImageFont *self, Graphics *graphics, int x, int y, const pvzstl::wstring &text, const Color &color, const TRect<int> &clipRect); // 0x34
        void (*StringToGlyphs)(ImageFont *self, const pvzstl::wstring &text, std::vector<Glyph> &glyphs);                                                          // 0x38
        void (*DrawGlyphs)(
            ImageFont *self, Graphics *graphics, int x, int y, std::vector<Glyph> &glyphs, unsigned int startIndex, unsigned int glyphCount, const Color &color, const TRect<int> &clipRect); // 0x3C
        void (*GlyphExtentsVector)(Font *self, const std::vector<Glyph> &glyphs, int &extents);                                                                                               // 0x40
        void (*GlyphExtentsList)(ImageFont *self, const int &glyphs, int &extents);                                                                                                           // 0x44
        void (*StringExtents)(Font *self, const pvzstl::string &text, int &extents);                                                                                                          // 0x48
        void (*StringExtentsUnicode)(Font *self, const pvzstl::wstring &text, int &extents);                                                                                                  // 0x4C
        bool (*IsComposited)(ImageFont *self);                                                                                                                                                // 0x50
        ImageFont *(*Duplicate)(ImageFont *self);                                                                                                                                             // 0x54
        void (*GenerateActiveFontLayers)(ImageFont *self);                                                                                                                                    // 0x58
        void (*DrawStringEx)(ImageFont *self, Graphics *graphics, int x, int y, const pvzstl::string &text, const Color &color, const TRect<int> *clipRect, int *drawnRects, int *width);     // 0x5C
        void (*DrawStringExUnicode)(ImageFont *self,
                                    Graphics *graphics,
                                    int x,
                                    int y,
                                    const pvzstl::wstring &text,
                                    const Color &color,
                                    const TRect<int> *clipRect,
                                    int *drawnRects,
                                    int *width);                                                              // 0x60
        int (*MappedCharWidthKern)(ImageFont *self, int character, int previousCharacter);                    // 0x64
        void (*SetPointSize)(ImageFont *self, int pointSize);                                                 // 0x68
        int (*GetPointSize)(ImageFont *self);                                                                 // 0x6C
        void (*SetScale)(ImageFont *self, double scale);                                                      // 0x70
        int (*GetDefaultPointSize)(ImageFont *self);                                                          // 0x74
        void (*AddTagUnicode)(ImageFont *self, const pvzstl::wstring &tag);                                   // 0x78
        void (*RemoveTagUnicode)(ImageFont *self, const pvzstl::wstring &tag);                                // 0x7C
        bool (*HasTagUnicode)(ImageFont *self, const pvzstl::wstring &tag);                                   // 0x80
        void (*AddTag)(ImageFont *self, const pvzstl::string &tag);                                           // 0x84
        void (*RemoveTag)(ImageFont *self, const pvzstl::string &tag);                                        // 0x88
        bool (*HasTag)(ImageFont *self, const pvzstl::string &tag);                                           // 0x8C
        pvzstl::wstring *(*GetDefine)(pvzstl::wstring *result, ImageFont *self, const pvzstl::wstring &name); // 0x90
        void (*Prepare)(ImageFont *self);                                                                     // 0x94
    };

    const ImageFontVTable *GetVTable() const {
        return (ImageFontVTable *)vTable;
    }

    ImageFont() = delete;
    ~ImageFont() = delete;
};

class FreeTypeFont : public Font {
    [[maybe_unused]] int unk[3];

public:
    FreeTypeFont(SexyAppBase *theApp, pvzstl::string const &theFile, int size, bool bold, bool italic, bool underline) {
        reinterpret_cast<void (*)(FreeTypeFont *, SexyAppBase *, pvzstl::string const &, int, bool, bool, bool)>(Sexy_FreeTypeFont_FreeTypeFontAddr)(
            this, theApp, theFile, size, bold, italic, underline);
    }
    ~FreeTypeFont() {
        reinterpret_cast<void (*)(FreeTypeFont *)>(Sexy_FreeTypeFont__destructorAddr)(this);
    }
};

class SysFont : public Font {
public:
    SysFont() = delete;
    ~SysFont() = delete;
};

} // namespace Sexy

#endif // PVZ_SEXYAPPFRAMEWORK_GRAPHICS_FONT_H
