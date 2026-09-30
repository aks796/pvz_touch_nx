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

#ifndef PVZ_LAWN_BASE_GAMEPAD_CONTROLS_H
#define PVZ_LAWN_BASE_GAMEPAD_CONTROLS_H

#include "Homura/MemberUtils.h"
#include "Homura/TypeUtils.h"
#include "PvZ/Lawn/Board/GameObject.h"
#include "PvZ/SexyAppFramework/Misc/KeyCodes.h"
#include "PvZ/SexyAppFramework/Misc/Point.h"

struct SnapToGridPosition {
    float mX;
    float mY;
};

namespace Sexy {
class GamepadListener {
public:
    void **vTable;
};
} // namespace Sexy

class SeedBank;

class BaseGamepadControls : public Sexy::GamepadListener, public GameObject {
public:
    enum MovementState {
        MOVEMENT_STATE_NONE = 0,
        // 默认光标状态
        MOVEMENT_STATE_NORMAL = 1,
        // 暂未确认：进入时清左摇杆速度，退出时清两个字段
        MOVEMENT_STATE_UNKNOWN_2 = 2,
        // 按住 X 给僵尸上黄油
        MOVEMENT_STATE_BUTTER_HELD = 3,
        // 松开 X 后进入的黄油释放/恢复状态
        MOVEMENT_STATE_BUTTER_RELEASED = 4,
        // 方向键移动一格时的瞬时中间状态
        MOVEMENT_STATE_GRID_STEP = 5,
        // 在SeedBank中左右选取种子的状态
        MOVEMENT_STATE_SELECT_SEED = 6,
        // 选中种子后的种植光标状态
        MOVEMENT_STATE_PLANT_CURSOR = 7,
        // 挖掘蓄按状态，退出时根据 mDigIndicatorPercentage 决定是否真正挖掘
        MOVEMENT_STATE_DIG_HOLD = 8,
        MOVEMENT_STATE_UNKNOWN_9 = 9,
        // 长按挖掘前的等待/提示状态
        MOVEMENT_STATE_DIG_INDICATOR_WAIT = 10,
    };
    struct BaseGamepadControlsVTable {
        void (*OnButtonDown)(BaseGamepadControls *self, int button, int playerIndex, unsigned int flags); // 0x00
        void (*UnknownPureVirtual1)(BaseGamepadControls *self);                                           // 0x04
        void (*UnknownPureVirtual2)(BaseGamepadControls *self);                                           // 0x08
        bool (*OnKeyDown)(BaseGamepadControls *self, Sexy::KeyCode keyCode, unsigned int flags);          // 0x0C
        bool (*OnKeyUp)(BaseGamepadControls *self, Sexy::KeyCode keyCode, unsigned int flags);            // 0x10
        void (*Update)(BaseGamepadControls *self, float deltaTime);                                       // 0x14
        bool (*BeginDraw)(BaseGamepadControls *self, Sexy::Graphics *graphics);                           // 0x18
        void (*Draw)(BaseGamepadControls *self, Sexy::Graphics *graphics);                                // 0x1C
        void (*EndDraw)(BaseGamepadControls *self, Sexy::Graphics *graphics);                             // 0x20
        void (*MakeParentGraphicsFrame)(BaseGamepadControls *self, Sexy::Graphics *graphics);             // 0x24
        void (*EnterState)(BaseGamepadControls *self, MovementState state);                               // 0x28
        void (*ExitState)(BaseGamepadControls *self, MovementState state);                                // 0x2C
        void (*UpdateStates)(BaseGamepadControls *self, float deltaTime);                                 // 0x30
        void (*GotoState)(BaseGamepadControls *self, MovementState state);                                // 0x34
        SnapToGridPosition (*GetSnapToGridPos)(BaseGamepadControls *self);                                // 0x38
        Sexy::Point (*GetSnapToGridXY)(BaseGamepadControls *self);                                        // 0x3C
        void (*completeDestructor)(BaseGamepadControls *self);                                            // 0x40
        void (*deletingDestructor)(BaseGamepadControls *self);                                            // 0x44
    };

public:
    Board *mBoard;                   // 14
    bool mDrawCursorFrame;           // 0x03C
    float mCursorHighlightAnimPhase; // 16
    int mGamepadFrameCounter;        // 17
    float mGamepadAccLeftX;
    float mGamepadAccLeftY;
    float mGridCenterPositionX;    // 20
    float mGridCenterPositionY;    // 21
    float mRightPositionX;         // 22
    float mRightPositionY;         // 23
    MovementState mGamepadState;   // 24
    float mMovementStateTimer;     // 0x064, 原 unk1
    float mDigIndicatorPercentage; // 26 , 每按下一次铲除键就加2.3
    float mCursorPositionX;        // 27
    float mCursorPositionY;        // 28
    float mGamepadVelocityLeftX;   // 29
    float mGamepadVelocityLeftY;   // 30
    float mGamepadVelocityRightX;  // 31
    float mGamepadVelocityRightY;  // 32
    float mInputLeftStickX;        // 0x084, 原 unk2[0]
    float mInputLeftStickY;        // 0x088, 原 unk2[1]
    float mInputRightStickX;       // 0x08C, 原 unk2[2]
    float mInputRightStickY;       // 0x090, 原 unk2[3]
    int mPlayerIndex;              // 37，只是个index，GamepadControls[0]->mPlayerIndex == 0，GamepadControls[1]->mPlayerIndex == 1。用于找到对应index的CursorObject和CursorPreview
    int mGamepadIndex;             // 38，与mPlayerIndex完全相等，没有区别
    float mCursorPositionYJitter;  // 39
    float mCursorJitterAnimPhase;  // 40
    int mButterCursorMoveCounter;  // 0x0A4, 原 unkMems[0]
    int mDerivedSyncBlockPadding;  // 0x0A8, 原 unkMems[1]，暂未发现语义访问
    // 大小43个整数

    void Update(float dt) {
        reinterpret_cast<void (*)(BaseGamepadControls *, float)>(BaseGamepadControls_UpdateAddr)(this, dt);
    }
    void GotoState(int state) {
        reinterpret_cast<void (*)(BaseGamepadControls *, int)>(BaseGamepadControls_GotoStateAddr)(this, state);
    }
    void UpdateStates(float dt) {
        reinterpret_cast<void (*)(BaseGamepadControls *, float)>(BaseGamepadControls_UpdateStatesAddr)(this, dt);
    }
    SnapToGridPosition GetSnapToGridPos() {
        return reinterpret_cast<SnapToGridPosition (*)(BaseGamepadControls *)>(BaseGamepadControls_GetSnapToGridPosAddr)(this);
        //        return homura::CallVirtualFunc<BaseGamepadControls, 14, SnapToGridPosition>(this);
    }
    const BaseGamepadControlsVTable *GetVTable() const {
        return (BaseGamepadControlsVTable *)Sexy::GamepadListener::vTable;
    }
    void GetGamepadVelocity(float *horizontal, float *vertical);

protected:
    BaseGamepadControls() = default;
    ~BaseGamepadControls() = default;
};

#endif // PVZ_LAWN_BASE_GAMEPAD_CONTROLS_H
