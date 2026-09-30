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

#ifndef PVZ_LAWN_GAMEPAD_CONTROLS_H
#define PVZ_LAWN_GAMEPAD_CONTROLS_H

#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/SexyAppFramework/Misc/GamepadButtons.h"
#include "PvZ/SexyAppFramework/Misc/KeyCodes.h"

#include "BaseGamepadControls.h"

class Zombie;
class Plant;

class GamepadControls : public BaseGamepadControls {
public:
    struct GamepadControlsVTable {
        void (*OnButtonDown)(GamepadControls *self, int button, int playerIndex, unsigned int flags); // 0x00
        void (*OnButtonUp)(GamepadControls *self, int button, int playerIndex, unsigned int flags);   // 0x04
        void (*OnGameAxisMove)(GamepadControls *self, int axis, int value, int playerIndex);          // 0x08
        bool (*OnKeyDown)(GamepadControls *self, Sexy::KeyCode keyCode, unsigned int flags);          // 0x0C
        bool (*OnKeyUp)(GamepadControls *self, Sexy::KeyCode keyCode, unsigned int flags);            // 0x10
        void (*Update)(GamepadControls *self, float deltaTime);                                       // 0x14
        bool (*BeginDraw)(GamepadControls *self, Sexy::Graphics *graphics);                           // 0x18
        void (*Draw)(GamepadControls *self, Sexy::Graphics *graphics);                                // 0x1C
        void (*EndDraw)(GamepadControls *self, Sexy::Graphics *graphics);                             // 0x20
        void (*MakeParentGraphicsFrame)(GamepadControls *self, Sexy::Graphics *graphics);             // 0x24
        void (*EnterState)(BaseGamepadControls *self, BaseGamepadControls::MovementState state);      // 0x28
        void (*ExitState)(BaseGamepadControls *self, BaseGamepadControls::MovementState state);       // 0x2C
        void (*UpdateStates)(GamepadControls *self, float deltaTime);                                 // 0x30
        void (*GotoState)(BaseGamepadControls *self, BaseGamepadControls::MovementState state);       // 0x34
        Sexy::FPoint (*GetSnapToGridPos)(BaseGamepadControls *self);                                  // 0x38
        Sexy::Point (*GetSnapToGridXY)(BaseGamepadControls *self);                                    // 0x3C
        void (*completeDestructor)(GamepadControls *self);                                            // 0x40
        void (*deletingDestructor)(GamepadControls *self);                                            // 0x44
    };

public:
    float mCursorLabelLiftOffset;         // 43
    ParticleSystemID mSelectorParticleID; // 44
    int mSelectedSeedIndex;               // 45
    SeedType mSelectedSeedType;           // 46
    bool mIsZombie;                       // 188
    bool mCanPickUp;                      // 189
    int mSelectedUpgradableType;          // 48
    int mCobCannonPlantIndexInList;       // 49
    bool mIsCobCannonSelected;            // 200
    float mCanPickUpFade;                 // 0x0CC, 原 mUpdateAdd_a2_Or_Minus_2xa2
    ReanimationID mPreviewReanimID1;      // 52
    ReanimationID mCobCannonReanimID;     // 53
    ReanimationID mPreviewReanimID3;      // 54
    int mCobCannonAnimCounter;            // 55
    ReanimationID mPreviewReanimID4;      // 56
    SeedType mPreviewingSeedType;         // 57
    Sexy::Image *mPreviewImage;           // 58
    ZombieID mButterZombieID;             // 59
    int mDigIndicatorStartCounter;        // 60
    bool mIsShowingDigIndicator;          // 244
    bool mIsInShopSeedBank;               // 245
    int mSelectedShopSeedIndex;           // 62
    int unknown252;                       // 63 (offset 252)
    int unknown256;                       // 64 (offset 256)
    int unknown260;                       // 65 (offset 260)
    // 大小66个整数

    SeedBank *GetSeedBank() {
        return reinterpret_cast<SeedBank *(*)(GamepadControls *)>(GamepadControls_GetSeedBankAddr)(this);
    }
    void OnButtonUp(Sexy::GamepadButton theButton, int theGamepadIndex, unsigned int a4) {
        reinterpret_cast<void (*)(GamepadControls *, Sexy::GamepadButton, int, unsigned int)>(GamepadControls_OnButtonUpAddr)(this, theButton, theGamepadIndex, a4);
    }
    // theGamepadIndex 根据手柄决定是0还是1
    // a4 恒定为0
    bool OnKeyDown(Sexy::KeyCode theKey, unsigned int a3) {
        return reinterpret_cast<bool (*)(GamepadControls *, int, unsigned int)>(GamepadControls_OnKeyDownAddr)(this, theKey, a3);
    }
    void UpdateSeedSelect(float dt) {
        reinterpret_cast<void *(*)(GamepadControls *, float dt)>(GamepadControls_UpdateSeedSelectAddr)(this, dt);
    }
    // the special item bar (the engine's L2/R2): into it, out of it, its items
    void UpSelectedSeed() {
        reinterpret_cast<void (*)(GamepadControls *)>(GamepadControls_UpSelectedSeedAddr)(this);
    }
    void DownSelectedSeed() {
        reinterpret_cast<void (*)(GamepadControls *)>(GamepadControls_DownSelectedSeedAddr)(this);
    }
    void IncShopSeed() {
        reinterpret_cast<void (*)(GamepadControls *)>(GamepadControls_IncShopSeedAddr)(this);
    }
    void DecShopSeed() {
        reinterpret_cast<void (*)(GamepadControls *)>(GamepadControls_DecShopSeedAddr)(this);
    }
    const GamepadControlsVTable *GetVTable() const {
        return (GamepadControlsVTable *)Sexy::GamepadListener::vTable;
    }
    // 确定 13 1096
    // 返回 27 1096
    // 左 37 1096
    // 上 38 1096
    // 右 39 1096
    // 下 40 1096
    // 铲子 49 1112
    // 锤子 50 1112

    void ButtonDownFireCobcannonTest();
    void InvalidatePreviewReanim();
    void Draw(Sexy::Graphics *g);
    void Update(float a2);
    void UpdateStates(float dt);
    void DrawPreview(Sexy::Graphics *g);
    void UpdatePreviewReanim();
    void OnButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int unk);
    bool ShopBarButton(Sexy::GamepadButton theButton); // Switch port: -, Y, ZL/ZR on the lawn
    void SyncShopBar();                                // Switch port: the item bar only while - has it open
    void pickUpCobCannon(Plant *cobCannon);

protected:
    GamepadControls() = default;
    ~GamepadControls() = default;

    friend void InitHookFunction();

    void _constructor(Board *theBoard, int thePlayerIndex1, int thePlayerIndex2);
    void _destructor() {
        reinterpret_cast<void (*)(GamepadControls *)>(GamepadControls___destructorAddr)(this);
    }
};

class GamepadControls_ : public GamepadControls {
public:
    GamepadControls_(Board *theBoard, int thePlayerIndex1, int thePlayerIndex2) {
        _constructor(theBoard, thePlayerIndex1, thePlayerIndex2);
    }
    ~GamepadControls_() {
        _destructor();
    };
};

class ZenGardenControls : public GamepadControls {
public:
    GameObjectType mObjectType; // 66
    // 大小67个整数

    ZenGardenControls(Board *theBoard, int thePlayerIndex1, int thePlayerIndex2) {
        reinterpret_cast<void (*)(ZenGardenControls *, Board *, int, int)>(ZenGardenControls_ZenGardenControlsAddr)(this, theBoard, thePlayerIndex1, thePlayerIndex2);
    }
    ~ZenGardenControls() {
        _destructor();
    };

    void Update(float a2);

protected:
    void _destructor() {
        reinterpret_cast<void (*)(ZenGardenControls *)>(ZenGardenControls__destructorAddr)(this);
    }
};

class TreeOfWisdomControls : public GamepadControls {
public:
    int mUnknown264; // 66
    int mUnknown268; // 67
    int mUnknown272; // 68
    // 大小69个整数

    TreeOfWisdomControls(Board *theBoard, int thePlayerIndex1, int thePlayerIndex2) {
        reinterpret_cast<void (*)(TreeOfWisdomControls *, Board *, int, int)>(TreeOfWisdomControls_TreeOfWisdomControlsAddr)(this, theBoard, thePlayerIndex1, thePlayerIndex2);
    }
    ~TreeOfWisdomControls() {
        _destructor();
    };

protected:
    void _destructor() {
        reinterpret_cast<void (*)(TreeOfWisdomControls *)>(TreeOfWisdomControls__destructorAddr)(this);
    }
};

/***************************************************************************************************************/


inline void (*old_GamepadControls_Draw)(GamepadControls *gamePad, Sexy::Graphics *graphics);

inline void (*old_GamepadControls_Update)(GamepadControls *gamepadControls, float a2);

inline void (*old_GamepadControls_GamepadControls)(GamepadControls *gamePad, Board *board, int a3, int a4);

inline void (*old_GamepadControls_ButtonDownFireCobcannonTest)(GamepadControls *gamepadControls);

inline void (*old_GamepadControls_UpdatePreviewReanim)(GamepadControls *gamePad);

inline void (*old_GamepadControls_DrawPreview)(GamepadControls *gamePad, Sexy::Graphics *graphics);

// Switch port: a player has the item bar open (LawnApp::CanShopLevel)
bool GamepadShopBarOpen();

inline void (*old_GamepadControls_OnButtonDown)(GamepadControls *, Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int unk);

inline void (*old_ZenGardenControls_Update)(ZenGardenControls *a1, float a2);

// FilterEffect GetFilterEffectTypeBySeedType(SeedType mSeedType);

#endif // PVZ_LAWN_GAMEPAD_CONTROLS_H
