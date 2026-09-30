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

#ifndef PVZ_SEXYAPPFRAMEWORK_SEXY_APP_BASE_H
#define PVZ_SEXYAPPFRAMEWORK_SEXY_APP_BASE_H

#include "PvZ/MagicNumbers.h"
#include "PvZ/STL/list.h"
#include "PvZ/STL/map.h"
#include "PvZ/STL/set.h"
#include "PvZ/STL/string.h"
#include "PvZ/SexyAppFramework/Misc/CritSect.h"
#include "PvZ/SexyAppFramework/Misc/Ratio.h"
#include "PvZ/SexyAppFramework/Misc/ResourceManager.h"
#include "PvZ/SexyAppFramework/Thread.h"
#include "PvZ/SexyAppFramework/Widget/DialogListener.h"
#include "PvZ/SexyAppFramework/Widget/InputConnectListener.h"
#include "PvZ/SexyAppFramework/Widget/WidgetManager.h"
#include "PvZ/Symbols.h"

#include "Buffer.h"
#include "Graphics/Color.h"
#include "Graphics/MemoryImage.h"
#include "Misc/Common.h"
#include "Misc/Rect.h"
#include "Sound/AudiereSoundManager.h"
#include "Widget/ButtonListener.h"

void InitHookFunction();

namespace Sexy {

struct Event;

class Dialog;
class Gamepad;
class MusicInterface;

class SexyAppBase : public Sexy::ButtonListener, public Sexy::DialogListener, public Sexy::InputConnectListener {
public:
    struct SexyAppBaseVTable {
        void (*completeDestructor)(SexyAppBase *self);                               // 0x000
        void (*deletingDestructor)(SexyAppBase *self);                               // 0x004
        void (*ButtonPress)(Sexy::ButtonListener *self, int id);                     // 0x008
        void (*ButtonPressWithCount)(Sexy::ButtonListener *self, int id, int count); // 0x00C
        void (*ButtonDepress)(Sexy::ButtonListener *self, int id);                   // 0x010
        void (*ButtonDownTick)(Sexy::ButtonListener *self, int id);                  // 0x014
        void (*ButtonMouseEnter)(Sexy::ButtonListener *self, int id);                // 0x018
        void (*ButtonMouseLeave)(Sexy::ButtonListener *self, int id);                // 0x01C
        void (*ButtonMouseMove)(Sexy::ButtonListener *self, int id, int x, int y);   // 0x020
        void (*MakeWindow)(SexyAppBase *self);                                       // 0x024
        void (*InitCursors)(SexyAppBase *self);                                      // 0x028
        void (*EnforceCursor)(SexyAppBase *self);                                    // 0x02C
        void (*ClearKeysDown)(SexyAppBase *self);                                    // 0x030
        void (*UpdateFrames)(SexyAppBase *self);                                     // 0x034
        void (*ReInitImages)(SexyAppBase *self);                                     // 0x038
        void (*DeleteNativeImageData)(SexyAppBase *self);                            // 0x03C
        void (*DeleteExtraImageData)(SexyAppBase *self);                             // 0x040
        void (*Delete3DImageData)(SexyAppBase *self);                                // 0x044
        void (*LoadingThreadCompleted)(SexyAppBase *self);                           // 0x048
        void (*Evict3DImageData)(SexyAppBase *self, std::uint32_t amount);           // 0x04C
        void (*StartSensor)(SexyAppBase *self, int sensorType);                      // 0x050
        void (*StopSensor)(SexyAppBase *self, int sensorType);                       // 0x054
        void (*PauseApp)(SexyAppBase *self);                                         // 0x058
        void (*ResumeApp)(SexyAppBase *self);                                        // 0x05C
        int *(*CreateMusicInterface)(SexyAppBase *self, void *context);              // 0x060,Sexy::MusicInterface*
        void (*InitHook)(SexyAppBase *self);                                         // 0x064
        void (*ShutdownHook)(SexyAppBase *self);                                     // 0x068
        void (*PreTerminate)(SexyAppBase *self);                                     // 0x06C
        void (*LoadingThreadProc)(SexyAppBase *self);                                // 0x070
        void (*WriteToRegistry)(SexyAppBase *self);                                  // 0x074
        void (*ReadFromRegistry)(SexyAppBase *self);                                 // 0x078
        Sexy::Dialog *(*NewDialog)(SexyAppBase *self, int dialogId, bool modal, const pvzstl::string &header, const pvzstl::string &lines, const pvzstl::string &footer, int buttonMode); // 0x07C
        void (*PreDisplayHook)(SexyAppBase *self);                                                                                                                                        // 0x080
        bool (*IsUIOrientationAllowed)(SexyAppBase *self, int orientation);                                                                                                               // 0x084
        void (*UIOrientationChanged)(SexyAppBase *self, int orientation);                                                                                                                 // 0x088
        int (*GetUIOrientation)(SexyAppBase *self);                                                                                                                                       // 0x08C
        void (*OnFullVersionChange)(SexyAppBase *self);                                                                                                                                   // 0x090
        void (*LowMemoryWarning)(SexyAppBase *self);                                                                                                                                      // 0x094
        void (*AppEnteredBackground)(SexyAppBase *self);                                                                                                                                  // 0x098
        void (*AppEnteredForeground)(SexyAppBase *self);                                                                                                                                  // 0x09C
        void (*BeginPopup)(SexyAppBase *self);                                                                                                                                            // 0x0A0
        void (*EndPopup)(SexyAppBase *self);                                                                                                                                              // 0x0A4
        int (*MsgBox)(SexyAppBase *self, const pvzstl::string &text, const pvzstl::string &title, int flags);                                                                             // 0x0A8
        int (*MsgBoxIntString)(SexyAppBase *self, const std::basic_string<int> &text, const std::basic_string<int> &title, int flags);                                                    // 0x0AC
        void (*Popup)(SexyAppBase *self, const pvzstl::string &text);                                                                                                                     // 0x0B0
        void (*PopupIntString)(SexyAppBase *self, const std::basic_string<int> &text);                                                                                                    // 0x0B4
        void (*LogScreenSaverError)(SexyAppBase *self, const pvzstl::string &error);                                                                                                      // 0x0B8
        void (*SafeDeleteWidget)(SexyAppBase *self, Sexy::Widget *widget);                                                                                                                // 0x0BC
        void (*URLOpenFailed)(SexyAppBase *self, const pvzstl::string &url);                                                                                                              // 0x0C0
        void (*URLOpenSucceeded)(SexyAppBase *self, const pvzstl::string &url);                                                                                                           // 0x0C4
        bool (*OpenURL)(SexyAppBase *self, const pvzstl::string &url, bool shutdownOnOpen);                                                                                               // 0x0C8
        void (*OpenRegisterPageWithParameters)(SexyAppBase *self, int *parameters);                                                                                                       // 0x0CC
        void (*OpenRegisterPage)(SexyAppBase *self);                                                                                                                                      // 0x0D0
        pvzstl::string (*GetProductVersion)(SexyAppBase *self, const pvzstl::string &path);                                                                                               // 0x0D4
        void (*SEHOccured)(SexyAppBase *self);                                                                                                                                            // 0x0D8
        pvzstl::string (*GetGameSEHInfo)(SexyAppBase *self);                                                                                                                              // 0x0DC
        void (*GetSEHWebParams)(SexyAppBase *self, int *parameters);                                                                                                                      // 0x0E0
        void (*Shutdown)(SexyAppBase *self);                                                                                                                                              // 0x0E4
        void (*Quit)(SexyAppBase *self);                                                                                                                                                  // 0x0E8
        pvzstl::string (*FormatHelpString)(SexyAppBase *self);                                                                                                                            // 0x0EC
        void (*AddParameterEntry)(SexyAppBase *self, int *entry);                                                                                                                         // 0x0F0
        void (*AddParameterEntries)(SexyAppBase *self, int *entries);                                                                                                                     // 0x0F4
        void (*DoParseCmdLine)(SexyAppBase *self);                                                                                                                                        // 0x0F8
        void (*ParseCmdLineVector)(SexyAppBase *self, const std::vector<pvzstl::string> &arguments);                                                                                      // 0x0FC
        void (*ParseCmdLineString)(SexyAppBase *self, const pvzstl::string &commandLine);                                                                                                 // 0x100
        void (*HandleCmdLineParam)(SexyAppBase *self, const pvzstl::string &name, const pvzstl::string &value);                                                                           // 0x104
        void (*HandleNotifyGameMessage)(SexyAppBase *self, int type, int parameter);                                                                                                      // 0x108
        void (*HandleGameAlreadyRunning)(SexyAppBase *self);                                                                                                                              // 0x10C
        void (*Startup)(SexyAppBase *self);                                                                                                                                               // 0x110
        void (*Start)(SexyAppBase *self);                                                                                                                                                 // 0x114
        void (*Terminate)(SexyAppBase *self);                                                                                                                                             // 0x118
        void (*Init)(SexyAppBase *self);                                                                                                                                                  // 0x11C
        void (*Cleanup)(SexyAppBase *self);                                                                                                                                               // 0x120
        void (*PreDDInterfaceInitHook)(SexyAppBase *self);                                                                                                                                // 0x124
        void (*PostDDInterfaceInitHook)(SexyAppBase *self);                                                                                                                               // 0x128
        bool (*ChangeDirHook)(SexyAppBase *self, const char *intendedPath);                                                                                                               // 0x12C
        void (*PlaySample)(SexyAppBase *self, int soundId);                                                                                                                               // 0x130
        void (*PlaySampleWithPan)(SexyAppBase *self, int soundId, int pan);                                                                                                               // 0x134
        void (*PlaySampleSingle)(SexyAppBase *self, int soundId);                                                                                                                         // 0x138
        void (*PlaySampleSingleWithPan)(SexyAppBase *self, int soundId, int pan);                                                                                                         // 0x13C
        double (*GetMasterVolume)(SexyAppBase *self);                                                                                                                                     // 0x140
        double (*GetMusicVolume)(SexyAppBase *self);                                                                                                                                      // 0x144
        double (*GetSfxVolume)(SexyAppBase *self);                                                                                                                                        // 0x148
        bool (*IsMuted)(SexyAppBase *self);                                                                                                                                               // 0x14C
        void (*SetMasterVolume)(SexyAppBase *self, double volume);                                                                                                                        // 0x150
        void (*SetMusicVolume)(SexyAppBase *self, double volume);                                                                                                                         // 0x154
        void (*SetSfxVolume)(SexyAppBase *self, double volume);                                                                                                                           // 0x158
        void (*Mute)(SexyAppBase *self, bool autoMute);                                                                                                                                   // 0x15C
        void (*Unmute)(SexyAppBase *self, bool autoMute);                                                                                                                                 // 0x160
        double (*GetLoadingThreadProgress)(SexyAppBase *self);                                                                                                                            // 0x164
        Sexy::Image *(*GetTexImage)(SexyAppBase *self, const pvzstl::string &fileName, bool commitBits);                                                                                  // 0x168
        Sexy::Image *(*GetImage)(SexyAppBase *self, const pvzstl::string &fileName, bool commitBits);                                                                                     // 0x16C
        Sexy::Image *(*GetImageWithAttrs)(SexyAppBase *self, const pvzstl::string &fileName, const pvzstl::string &variant, const pvzstl::string &attributes, bool commitBits);           // 0x170
        bool (*ReloadImage)(SexyAppBase *self, Sexy::Image *image);                                                                                                                       // 0x174
        int *(*GetSharedImage)(SexyAppBase *self, const pvzstl::string &fileName, const pvzstl::string &variant, bool *isNew, bool commitBits, bool allowTriReps);                        // 0x178
        int *(*GetSharedImageWithAttrs)(
            SexyAppBase *self, const pvzstl::string &fileName, const pvzstl::string &variant, const pvzstl::string &attributes, const pvzstl::string &alphaFile, bool *isNew);           // 0x17C
        Sexy::Image *(*GetImageForInput)(SexyAppBase *self, const pvzstl::string &fileName, int width, int height, const pvzstl::string &variant);                                       // 0x180
        void (*RemoveImageForInput)(SexyAppBase *self, const pvzstl::string &fileName);                                                                                                  // 0x184
        void (*SwitchScreenMode)(SexyAppBase *self);                                                                                                                                     // 0x188
        void (*SwitchScreenModeWindowed)(SexyAppBase *self, bool windowed);                                                                                                              // 0x18C
        void (*SwitchScreenModeFull)(SexyAppBase *self, bool windowed, bool use3D, bool force);                                                                                          // 0x190
        void (*SetAlphaDisabled)(SexyAppBase *self, bool disabled);                                                                                                                      // 0x194
        Sexy::Dialog *(*DoDialog)(SexyAppBase *self, int dialogId, bool modal, const pvzstl::string &header, const pvzstl::string &lines, const pvzstl::string &footer, int buttonMode); // 0x198
        Sexy::Dialog *(*GetDialog)(SexyAppBase *self, int dialogId);                                                                                                                     // 0x19C
        void (*AddDialogWithId)(SexyAppBase *self, int dialogId, Sexy::Dialog *dialog);                                                                                                  // 0x1A0
        void (*AddDialog)(SexyAppBase *self, Sexy::Dialog *dialog);                                                                                                                      // 0x1A4
        bool (*KillDialogFull)(SexyAppBase *self, int dialogId, bool removeWidget, bool deleteWidget);                                                                                   // 0x1A8
        bool (*KillDialogById)(SexyAppBase *self, int dialogId);                                                                                                                         // 0x1AC
        bool (*KillDialogByPointer)(SexyAppBase *self, Sexy::Dialog *dialog);                                                                                                            // 0x1B0
        int (*GetDialogCount)(SexyAppBase *self);                                                                                                                                        // 0x1B4
        void (*ModalOpen)(SexyAppBase *self);                                                                                                                                            // 0x1B8
        void (*ModalClose)(SexyAppBase *self);                                                                                                                                           // 0x1BC
        void (*DialogButtonPress)(SexyAppBase *self, int dialogId, int buttonId);                                                                                                        // 0x1C0
        void (*DialogButtonDepress)(SexyAppBase *self, int dialogId, int buttonId);                                                                                                      // 0x1C4
        void (*GotFocus)(SexyAppBase *self);                                                                                                                                             // 0x1C8
        void (*LostFocus)(SexyAppBase *self);                                                                                                                                            // 0x1CC
        bool (*IsAltKeyUsed)(SexyAppBase *self, int keyCode);                                                                                                                            // 0x1D0
        bool (*KeyDown)(SexyAppBase *self, int keyCode);                                                                                                                                 // 0x1D4
        bool (*DebugKeyDown)(SexyAppBase *self, int keyCode);                                                                                                                            // 0x1D8
        bool (*DebugKeyDownAsync)(SexyAppBase *self, int keyCode, bool controlDown, bool altDown);                                                                                       // 0x1DC
        void (*CloseRequestAsync)(SexyAppBase *self);                                                                                                                                    // 0x1E0
        void (*Done3dTesting)(SexyAppBase *self);                                                                                                                                        // 0x1E4
        pvzstl::string (*NotifyCrashHook)(SexyAppBase *self);                                                                                                                            // 0x1E8
        bool (*CheckSignature)(SexyAppBase *self, const Sexy::Buffer &buffer, const pvzstl::string &fileName);                                                                           // 0x1EC
        void (*PreDrawScreen)(SexyAppBase *self);                                                                                                                                        // 0x1F0
        bool (*DrawDirtyStuff)(SexyAppBase *self);                                                                                                                                       // 0x1F4
        void (*Redraw)(SexyAppBase *self, Sexy::TRect<int> *clipRect);                                                                                                                   // 0x1F8
        void (*InitPropertiesHook)(SexyAppBase *self);                                                                                                                                   // 0x1FC
        bool (*UpdateAppStep)(SexyAppBase *self, bool *updated);                                                                                                                         // 0x200
        bool (*UpdateApp)(SexyAppBase *self);                                                                                                                                            // 0x204
        void (*PopulateMessages)(SexyAppBase *self);                                                                                                                                     // 0x208
        bool (*ProcessMessages)(SexyAppBase *self, std::vector<Sexy::Event> &events);                                                                                                    // 0x20C
        bool (*ProcessMessage)(SexyAppBase *self, Sexy::Event &event);                                                                                                                   // 0x210
        void (*CleanupHook)(SexyAppBase *self);                                                                                                                                          // 0x214
        void (*DrawOneFrame)(SexyAppBase *self);                                                                                                                                         // 0x218
        bool (*AppCanRestore)(SexyAppBase *self);                                                                                                                                        // 0x21C
        void (*InputStatusChanged)(SexyAppBase *self, int *status);                                                                                                                      // 0x220
        void (*InitPreLoadWidget)(SexyAppBase *self);                                                                                                                                    // 0x224
        Sexy::Widget *(*CreatePreLoadWidget)(SexyAppBase *self);                                                                                                                         // 0x228
        void (*FinishPreLoadWidget)(SexyAppBase *self);                                                                                                                                  // 0x22C
        void (*PostPreLoad)(SexyAppBase *self);                                                                                                                                          // 0x230
        void (*PreLoadResources)(SexyAppBase *self);                                                                                                                                     // 0x234
        void (*AuthFinished)(SexyAppBase *self, bool success);                                                                                                                           // 0x238
        void (*StartAuth)(SexyAppBase *self);                                                                                                                                            // 0x23C
        void (*GetMemoryInfo)(SexyAppBase *self, int &memoryInfo);                                                                                                                       // 0x240
        bool (*FrameNeedsSwapScreenImage)(SexyAppBase *self);                                                                                                                            // 0x244
        bool (*TakeScreenshot)(SexyAppBase *self, Sexy::MemoryImage &image);                                                                                                             // 0x248
    };

public:
    int mRandSeed; // 0x00C, dword 3

    homura::Storage<pvzstl::string> mCompanyName;           // 0x010
    homura::Storage<pvzstl::string> mFullCompanyName;       // 0x014
    homura::Storage<pvzstl::string> mProdName;              // 0x018
    homura::Storage<pvzstl::string> mTitle;                 // 0x01C
    homura::Storage<pvzstl::string> mRegKey;                // 0x020
    homura::Storage<pvzstl::string> mChangeDirTo;           // 0x024
    homura::Storage<pvzstl::string> mResourceManifestPath;  // 0x028
    homura::Storage<pvzstl::string> mDefaultPropertiesPath; // 0x02C

    int mRelaxUpdateBacklogCount; // 0x030, dword 12
    int mPreferredX;              // 0x034, dword 13
    int mPreferredY;              // 0x038, dword 14
    int mPreferredX2;             // 0x03C, tentative
    int mPreferredY2;             // 0x040, tentative
    int unk_044;                  // 0x044
    int unk_048;                  // 0x048

    int mWidth;              // 0x04C, dword 19
    int mHeight;             // 0x050, dword 20
    int mFullscreenBits;     // 0x054, dword 21
    double mMusicVolume;     // 0x058
    double mSfxVolume;       // 0x060
    double mDemoMusicVolume; // 0x068
    double mDemoSfxVolume;   // 0x070

    bool mNoSoundNeeded;         // 0x078
    bool mWantFMod;              // 0x079
    bool mCmdLineParsed;         // 0x07A
    bool mSkipSignatureChecks;   // 0x07B
    bool mStandardWordWrap;      // 0x07C
    bool mAllowExtendedChars;    // 0x07D
    bool mOnlyAllowOneCopyToRun; // 0x07E
    bool unk_07F;                // 0x07F

    void *mMutex;                                 // 0x080
    homura::Storage<CritSect> mCritSect;          // 0x084
    homura::Storage<CritSect> mSecondaryCritSect; // 0x088, Android addition
    int mNotifyGameMessage;                       // 0x08C, tentative name
    bool mBetaValidate;                           // 0x090
    unsigned char mAdd8BitMaxTable[512];          // 0x091, values 0..255 then 0xFF

    WidgetManager *mWidgetManager; // 0x294, dword 165

    homura::Storage<pvzstl::map<int, Sexy::Dialog *>> mDialogMap; // 0x298
    homura::Storage<pvzstl::list<void *>> mDialogList;            // 0x2B0
    unsigned int mPrimaryThreadId;                                // 0x2B8
    Sexy::Thread mPrimaryThread;                                  // 0x2BC, Android addition

    bool unkAppState_2C4;          // 0x2C4, likely mSEHOccured from PC member order
    bool mShutDown;                // 0x2C5, confirmed by external xref
    bool unkAppStateFlags_2C6[10]; // 0x2C6~0x2CF; first byte is likely mExitToTop
    unsigned int mTimeLoaded;      // 0x2D0
    void *mPlatformWindow1;        // 0x2D4
    void *mPlatformWindow2;        // 0x2D8
    bool mIsScreenSaver;           // 0x2DC = false
    bool mAllowMonitorPowersave;   // 0x2DD = true
    bool mNoDefer;                 // 0x2DE = false
    bool mFullScreenPageFlip;      // 0x2DF = true

    bool mTabletPC; // 0x2E0 = false，高置信

    void *mPlatformDriverOrQueue; // 0x2E4 = nullptr，准确类型未知

    void *mFileDriver;                               // 0x2E8
    void *mAppDriver;                                // 0x2EC
    void *mDDInterface;                              // 0x2F0, high-confidence slot
    void *mAudioDriver;                              // 0x2F4
    void *mResStreamsDriver;                         // 0x2F8
    void *mHttpDriver;                               // 0x2FC
    bool mAlphaDisabled;                             // 0x300
    MusicInterface *mMusicInterface;                 // 0x304
    bool mReadFromRegistry;                          // 0x308
    homura::Storage<pvzstl::string> mRegisterLink;   // 0x30C
    homura::Storage<pvzstl::string> mProductVersion; // 0x310
    void *mCursorImages[13];                         // 0x314, dword 197~209
    int unkCursorPlatform[26];                       // 0x348~0x3AF

    bool mLawnMouseMode;     // 0x3B0
    bool mIsOpeningURL;      // 0x3B1
    bool mShutdownOnURLOpen; // 0x3B2, tentative

    homura::Storage<pvzstl::string> mOpeningURL; // 0x3B4
    unsigned int mOpeningURLTime;                // 0x3B8
    unsigned int mLastTimerTime;                 // 0x3BC
    unsigned int mLastBigDelayTime;              // 0x3C0
    double mUnmutedMusicVolume;                  // 0x3C8
    double mUnmutedSfxVolume;                    // 0x3D0
    int mMuteCount;                              // 0x3D8
    int mAutoMuteCount;                          // 0x3DC
    bool mDemoMute;                              // 0x3E0
    bool mMuteOnLostFocus;                       // 0x3E1

    homura::Storage<pvzstl::set<Sexy::MemoryImage *>> mMemoryImageSet;                                       // 0x3E4
    homura::Storage<CritSect> mMemoryImageCritSect;                                                          // 0x3FC, Android addition
    homura::Storage<pvzstl::set<void *>> mPIEffectSet;                                                       // 0x400, std::set<PIEffect*>
    homura::Storage<pvzstl::set<void *>> mPopAnimSet;                                                        // 0x418, std::set<PopAnim*>
    homura::Storage<pvzstl::map<std::pair<pvzstl::string, pvzstl::string>, void *>> mSharedImageMap;         // 0x430, map<pair<string,string>, SharedImage>
    bool mCleanupSharedImages;                                                                               // 0x448
    homura::Storage<pvzstl::map<pvzstl::string, pvzstl::map<pvzstl::string, Sexy::Image *>>> mImageGroupMap; // 0x44C
    homura::Storage<pvzstl::map<int, pvzstl::string>> mImageIdStringMap;                                     // 0x464

    int mNonDrawCount;               // 0x47C
    int mFrameTime;                  // 0x480
    bool mIsDrawing;                 // 0x484
    bool mLastDrawWasEmpty;          // 0x485
    bool mHasPendingDraw;            // 0x486
    double mPendingUpdatesAcc;       // 0x488
    double mUpdateFTimeAcc;          // 0x490
    unsigned int mLastTimeCheck;     // 0x498
    unsigned int mLastTime;          // 0x49C
    unsigned int mLastUserInputTick; // 0x4A0
    int mSleepCount;                 // 0x4A4
    int mDrawCount;                  // 0x4A8
    int mUpdateCount;                // 0x4AC
    int mUpdateAppState;             // 0x4B0
    int mUpdateAppDepth;             // 0x4B4
    int unk_4B8;                     // 0x4B8
    int mUpdateFrameTime;            // 0x4BC, initialized to 10; name tentative
    double mUpdateMultiplier;        // 0x4C0
    bool mPaused;                    // 0x4C8
    int mFastForwardToUpdateNum;     // 0x4CC
    bool mFastForwardToMarker;       // 0x4D0
    bool mFastForwardStep;           // 0x4D1
    unsigned int mLastDrawTick;      // 0x4D4
    unsigned int mNextDrawTick;      // 0x4D8
    int mStepMode;                   // 0x4DC
    int mCursorNum;                  // 0x4E0
    int unkCursorState_4E4;          // 0x4E4

    AudiereSoundManager *mSoundManager; // 0x4E8, dword 314

    void *mHandCursor;                                     // 0x4EC
    void *mDraggingCursor;                                 // 0x4F0
    int unkCursorPlatform_4F4[7];                          // 0x4F4~0x50F
    int mMouseX;                                           // 0x510, remapped current mouse X
    int mMouseY;                                           // 0x514, remapped current mouse Y
    homura::Storage<pvzstl::list<void *>> mSafeDeleteList; // 0x518, list<WidgetSafeDeleteInfo>

    bool mMouseIn;       // 0x520
    bool mRunning;       // 0x521
    bool mActive;        // 0x522
    bool mMinimized;     // 0x523
    bool mPhysMinimized; // 0x524
    bool mIsDisabled;    // 0x525
    bool mHasFocus;      // 0x526
    bool unk_527;

    int mDrawTime;              // 0x528
    unsigned int mFPSStartTick; // 0x52C
    int mFPSFlipCount;          // 0x530
    int mFPSDirtyCount;         // 0x534
    int mFPSTime;               // 0x538
    int mFPSCount;              // 0x53C
    bool mShowFPS;              // 0x540
    int mShowFPSMode;           // 0x544
    int mScreenBltTime;         // 0x548
    int unkPerf_54C[8];         // 0x54C ~ 0x56B

    bool mAutoStartLoadingThread; // 0x56C
    bool mLoadingThreadStarted;   // 0x56D
    bool mLoadingThreadCompleted; // 0x56E
    bool mLoaded;                 // 0x56F

    bool mYieldMainThread;   // 0x570
    bool mLoadingFailed;     // 0x571
    bool unkLoadingFlag_572; // 0x572
    bool unkLoadingFlag_573; // 0x573

    Sexy::Thread mLoadingThread; // 0x574

    bool mCursorThreadRunning;     // 0x57C
    bool mSysCursor;               // 0x57D
    bool mCustomCursorsEnabled;    // 0x57E
    bool mCustomCursorDirty;       // 0x57F
    bool mLastShutdownWasGraceful; // 0x580
    bool mIsWideWindow;            // 0x581
    bool unkLoadingFlag_582;       // 0x582
    bool unkLoadingFlag_583;       // 0x583
    bool unkLoadingFlag_584;       // 0x584
    bool unkLoadingFlag_585;       // 0x585
    bool unkLoadingFlag_586;       // 0x586
    bool unkLoadingFlag_587;       // 0x587

    int mNumLoadingThreadTasks;       // 0x588, dword 354
    int mCompletedLoadingThreadTasks; // 0x58C, dword 355

    bool mRecordingDemoBuffer;                             // 0x590
    bool mPlayingDemoBuffer;                               // 0x591
    bool mManualShutdown;                                  // 0x592
    homura::Storage<pvzstl::string> mDemoPrefix;           // 0x594
    homura::Storage<pvzstl::string> mDemoFileName;         // 0x598
    char mDemoBuffer[0x1C];                                // 0x59C
    int mDemoLength;                                       // 0x5B8
    int mLastDemoMouseX;                                   // 0x5BC
    int mLastDemoMouseY;                                   // 0x5C0
    int mLastDemoUpdateCnt;                                // 0x5C4
    bool mDemoNeedsCommand;                                // 0x5C8
    bool mDemoIsShortCmd;                                  // 0x5C9
    int mDemoCmdNum;                                       // 0x5CC
    int mDemoCmdOrder;                                     // 0x5D0
    int mDemoCmdBitPos;                                    // 0x5D4
    bool mDemoLoadingComplete;                             // 0x5D8
    int mCurHandleNum;                                     // 0x5DC
    homura::Storage<pvzstl::list<void *>> mDemoMarkerList; // 0x5E0

    bool mDebugKeysEnabled;     // 0x5E8
    bool mEnableMaximizeButton; // 0x5E9
    bool mCtrlDown;             // 0x5EA
    bool mAltDown;              // 0x5EB
    bool unk_5EC[4];            // 0x5EC

    int mSyncRefreshRate;                   // 0x5F0
    bool mVSyncUpdates;                     // 0x5F4
    bool mSoftVSyncWait;                    // 0x5F5, default true
    bool mVSyncBroken;                      // 0x5F6
    bool unkVSync_5F7;                      // 0x5F7
    int mVSyncBrokenCount;                  // 0x5F8
    unsigned int mVSyncBrokenTestStartTick; // 0x5FC
    unsigned int mVSyncBrokenTestUpdates;   // 0x600
    bool mWaitForVSync;                     // 0x604
    bool mUserChanged3DSetting;             // 0x605
    bool mAutoEnable3D;                     // 0x606
    bool mTest3D;                           // 0x607
    bool mIs3DAccelerated;                  // 0x608
    bool unk3D_609;                         // 0x609

    unsigned int mMinVidMemory3D;         // 0x60C, unkMem5[0] = 6
    unsigned int mRecommendedVidMemory3D; // 0x610, unkMem5[1] = 14

    bool mWidescreenAware;   // 0x614, unkMem5[2].byte0
    bool unkDisplayFlag_615; // 0x615, initialized true
    bool unkDisplayFlag_616; // 0x616
    bool unkDisplayFlag_617; // 0x617
    Rect mScreenBounds;      // 0x618, unkMem5[3]~[6]

    bool mEnableWindowAspect;    // 0x628, unkMem5[7].byte0
    Ratio mWindowAspect;         // 0x62C, 4:3
    Ratio mPresentationAspect;   // 0x634, 4:3, Android addition
    Ratio mWideScreenAspect;     // 0x63C, 16:10, Android addition
    int unk_644;                 // 0x644
    int mScreenWidth;            // 0x648 = 800
    int mScreenHeight;           // 0x64C = 600
    bool unkDisplayFlags_650[4]; // 0x650

    homura::Storage<pvzstl::map<pvzstl::string, pvzstl::string>> mStringProperties;                    // 0x654, map<string, SexyString>
    homura::Storage<pvzstl::map<pvzstl::string, bool>> mBoolProperties;                                // 0x66C
    homura::Storage<pvzstl::map<pvzstl::string, int>> mIntProperties;                                  // 0x684
    homura::Storage<pvzstl::map<pvzstl::string, double>> mDoubleProperties;                            // 0x69C
    homura::Storage<pvzstl::map<pvzstl::string, std::vector<pvzstl::string>>> mStringVectorProperties; // 0x6B4, map<string, vector<string>>
    ResourceManager *mResourceManager;                                                                 // 0x6CC, unkMem5[48]

    char mPopLoc[0x30];                                                // 0x6D0, unkMem5[49]~[60]
    int *mAuthManager;                                                 // 0x700, unkMem5[61]
    int unkAuth_704[3];                                                // 0x704, unkMem5[62]~[64]
    int *mInputManager;                                                // 0x710, unkMem5[65]
    int *mInputConnectManager;                                         // 0x714, unkMem5[66]
    homura::Storage<pvzstl::string> unkString_718;                     // 0x718
    homura::Storage<std::vector<pvzstl::string>> mUnknownStringVector; // 0x71C, vector<string>
    void *mUnknownHeap_728;                                            // 0x728, separately deleted
    int unk_72C;                                                       // 0x72C
    int unk_730;                                                       // 0x730
    float unkFloat_734;                                                // 0x734 = 100.0f
    float unkFloat_738;                                                // 0x738 = 100.0f
    int unkInt_73C;                                                    // 0x73C = 100
    int mMaxFPS;                                                       // 0x740, SEXY_MAX_FPS
    int unk_744[2];                                                    // 0x744
    bool unkFlag_74C;                                                  // 0x74C = true
    homura::Storage<pvzstl::string> unkString_750;                     // 0x750

    // Exact 0x34-byte footprint observed around AddParameterEntries().
    // Only mSelectedIndex's constructor value (-1) is known; the remaining
    // semantics are intentionally left generic.
    struct ParameterStorage {
        int mField00;
        int mField04;
        int mField08;
        int mField0C;
        int mSelectedIndex;
        unsigned char mData[0x20];
    } mParameterStorage; // 0x754

    void *mParameterEntriesHeap; // 0x788, deleted in destructor
    int unk_78C;                 // 0x78C
    int unk_790;                 // 0x790
    bool unkFlags_794[4];        // 0x794
    int unk_798;                 // 0x798
    bool mThreadedPreload;       // 0x79C, default true
    bool unkPreload_79D;         // 0x79D
    bool mSafeReload;            // 0x79E, SEXY_SAFE_RELOAD
    bool unkPreload_79F;         // 0x79F

    Dialog *GetDialog(Dialogs theDialogId) { // vTable + 4 * 103
        return reinterpret_cast<Dialog *(*)(SexyAppBase *, Dialogs)>(Sexy_SexyAppBase_GetDialogAddr)(this, theDialogId);
    }
    Image *GetImage(const pvzstl::string &theFileName, bool commitBits = true) {
        return reinterpret_cast<Image *(*)(SexyAppBase *, const pvzstl::string &, bool)>(Sexy_SexyAppBase_GetImageAddr)(this, theFileName, commitBits);
    }
    bool RegistryReadString(const pvzstl::string &theValueName, pvzstl::string *theString) {
        return reinterpret_cast<bool (*)(SexyAppBase *, const pvzstl::string &, pvzstl::string *)>(Sexy_SexyAppBase_RegistryReadStringAddr)(this, theValueName, theString);
    }
    Image *CopyImage(Image *theImage) {
        return reinterpret_cast<Image *(*)(SexyAppBase *, Image *)>(Sexy_SexyAppBase_CopyImageAddr)(this, theImage);
    }
    Image *CopyImage(Image *theImage, const Rect &theRect) {
        return reinterpret_cast<Image *(*)(SexyAppBase *, Image *, const Rect &)>(Sexy_SexyAppBase_CopyImage2Addr)(this, theImage, theRect);
    }
    void AddDialog(Dialogs id, Dialog *theDialog) { // vTable + 4 * 104
        reinterpret_cast<void (*)(SexyAppBase *, Dialogs, Dialog *)>(Sexy_SexyAppBase_AddDialog2Addr)(this, id, theDialog);
    }
    void AddDialog(Dialog *theDialog) {
        reinterpret_cast<void (*)(SexyAppBase *, Dialog *)>(Sexy_SexyAppBase_AddDialogAddr)(this, theDialog);
    }
    int GetDialogCount() { // vTable + 4 * 109
        return reinterpret_cast<int (*)(SexyAppBase *)>(Sexy_SexyAppBase_GetDialogCountAddr)(this);
    }
    void DoParseCmdLine() { // vTable + 4 * 62
        reinterpret_cast<void (*)(SexyAppBase *)>(Sexy_SexyAppBase_DoParseCmdLineAddr)(this);
    }
    int GetInteger(const pvzstl::string &theName, int defValue) {
        return reinterpret_cast<bool (*)(SexyAppBase *, const pvzstl::string &, int)>(Sexy_SexyAppBase_GetIntegerAddr)(this, theName, defValue);
    }
    int SetCursor(int theCursorNum) {
        return reinterpret_cast<int (*)(SexyAppBase *, int)>(Sexy_SexyAppBase_SetCursorAddr)(this, theCursorNum);
    }
    void LoadResourceManifest() {
        reinterpret_cast<void (*)(SexyAppBase *)>(Sexy_SexyAppBase_LoadResourceManifestAddr)(this);
    }
    void StartLoadingThread() {
        reinterpret_cast<void (*)(SexyAppBase *)>(Sexy_SexyAppBase_StartLoadingThreadAddr)(this);
    }
    double GetLoadingThreadProgress() {
        return reinterpret_cast<double (*)(SexyAppBase *)>(Sexy_SexyAppBase_GetLoadingThreadProgressAddr)(this);
    }
    bool WriteBufferToFile(const pvzstl::string &theFileName, Sexy::Buffer *theBuffer) {
        return reinterpret_cast<bool (*)(SexyAppBase *, const pvzstl::string &, Sexy::Buffer *)>(Sexy_SexyAppBase_WriteBufferToFileAddr)(this, theFileName, theBuffer);
    }
    bool ReadBufferFromFile(const pvzstl::string &theFileName, Sexy::Buffer *theBuffer, bool dontWriteToDemo = false) {
        return reinterpret_cast<bool (*)(SexyAppBase *, const pvzstl::string &, Sexy::Buffer *, bool)>(Sexy_SexyAppBase_ReadBufferFromFileAddr)(this, theFileName, theBuffer, dontWriteToDemo);
    } // UNICODE

    const SexyAppBaseVTable *GetVTable() const {
        return (SexyAppBaseVTable *)Sexy::ButtonListener::vTable;
    }

    bool UpdateApp();
    bool EraseFile(const pvzstl::string &theFileName);

protected:
    SexyAppBase() = default;
    ~SexyAppBase() = default;

    friend void ::InitHookFunction();

    void _constructor();
};

} // namespace Sexy

inline void (*old_Sexy_SexyAppBase_SexyAppBase)(Sexy::SexyAppBase *appBase);

inline bool (*old_Sexy_SexyAppBase_UpdateApp)(Sexy::SexyAppBase *appBase);

#endif // PVZ_SEXYAPPFRAMEWORK_SEXY_APP_BASE_H
