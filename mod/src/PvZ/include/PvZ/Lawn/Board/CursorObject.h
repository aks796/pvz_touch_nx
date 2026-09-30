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

#ifndef PVZ_LAWN_BOARD_CURSOR_OBJECT_H
#define PVZ_LAWN_BOARD_CURSOR_OBJECT_H

#include "PvZ/Lawn/Board/GameObject.h"
#include "PvZ/Symbols.h"

class Coin;
class Plant;

class CursorObject : public GameObject {
public:
    struct CursorObjectVTable {
        // 0x00 -> CursorObject::~CursorObject()
        void (*completeDestructor)(CursorObject *self);
        // 0x04 -> CursorObject::~CursorObject() + operator delete
        void (*deletingDestructor)(CursorObject *self);
        // 0x08 -> GameObject::BeginDraw(Graphics*)
        bool (*BeginDraw)(GameObject *self, Sexy::Graphics *graphics);
        // 0x0C -> GameObject::EndDraw(Graphics*)
        void (*EndDraw)(GameObject *self, Sexy::Graphics *graphics);
        // 0x10 -> GameObject::MakeParentGraphicsFrame(Graphics*)
        void (*MakeParentGraphicsFrame)(GameObject *self, Sexy::Graphics *graphics);
    };

public:
    int mSelectedIndex;            // 13
    SeedType mType;                // 14
    SeedType mImitaterType;        // 15
    CursorType mCursorType;        // 16
    CoinID mCoinID;                // 17
    PlantID mGlovePlantID;         // 18
    PlantID mDuplicatorPlantID;    // 19
    PlantID mCobCannonPlantID;     // 20
    int mHammerDownCounter;        // 21
    ReanimationID mReanimCursorID; // 22
    // 大小23个整数

    CursorObject() {
        reinterpret_cast<void (*)(CursorObject *)>(CursorObject_CursorObjectAddr)(this);
    }
    ~CursorObject() {
        _destructor();
    };

    void Draw(Sexy::Graphics *g) {
        reinterpret_cast<void (*)(CursorObject *, Sexy::Graphics *)>(CursorObject_DrawAddr)(this, g);
    }
    void Update() {
        reinterpret_cast<void (*)(CursorObject *)>(CursorObject_UpdateAddr)(this);
    }
    const CursorObjectVTable *GetVTable() const {
        return (CursorObjectVTable *)vTable;
    }

    bool BeginDraw(Sexy::Graphics *g);
    void EndDraw(Sexy::Graphics *g);


protected:
    void _destructor() {
        reinterpret_cast<void (*)(CursorObject *)>(CursorObject__destructorAddr)(this);
    }
};

class CursorPreview : public GameObject {
public:
    struct CursorPreviewVTable {
        // 0x00 -> CursorPreview::~CursorPreview()
        void (*completeDestructor)(CursorPreview *self);
        // 0x04 -> CursorPreview::~CursorPreview() + operator delete
        void (*deletingDestructor)(CursorPreview *self);
        // 0x08 -> GameObject::BeginDraw(Graphics*)
        bool (*BeginDraw)(GameObject *self, Sexy::Graphics *graphics);
        // 0x0C -> GameObject::EndDraw(Graphics*)
        void (*EndDraw)(GameObject *self, Sexy::Graphics *graphics);
        // 0x10 -> GameObject::MakeParentGraphicsFrame(Graphics*)
        void (*MakeParentGraphicsFrame)(GameObject *self, Sexy::Graphics *graphics);
    };

public:
    int mGridX;       // 13
    int mGridY;       // 14
    int mPlayerIndex; // 15
    // 大小16个整数

    CursorPreview(int thePlayerIndex) {
        reinterpret_cast<void (*)(CursorPreview *, int)>(CursorPreview_CursorPreviewAddr)(this, thePlayerIndex);
    }
    ~CursorPreview() {
        _destructor();
    };

    void Update() {
        reinterpret_cast<void (*)(CursorPreview *)>(CursorPreview_UpdateAddr)(this);
    }
    void _destructor() {
        reinterpret_cast<void (*)(CursorPreview *)>(CursorPreview__destructorAddr)(this);
    }
    const CursorPreviewVTable *GetVTable() const {
        return (CursorPreviewVTable *)vTable;
    }
};

inline bool (*old_CursorObject_BeginDraw)(CursorObject *cursorObject, Sexy::Graphics *graphics);

inline void (*old_CursorObject_EndDraw)(CursorObject *cursorObject, Sexy::Graphics *graphics);

#endif // PVZ_LAWN_BOARD_CURSOR_OBJECT_H
