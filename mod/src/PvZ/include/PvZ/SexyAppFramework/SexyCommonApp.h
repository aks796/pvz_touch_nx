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

#ifndef PVZ_SEXYAPPFRAMEWORK_SEXYCOMMONAPP_H
#define PVZ_SEXYAPPFRAMEWORK_SEXYCOMMONAPP_H

#include "SexyApp.h"

namespace Sexy {

class SexyCommonApp : public SexyApp {
public:
    void *mConnectionManager; // 0x7A0, unkMemCommonApp[0]

    void *mPlatformIntegration;       // 0x7A4, unkMemCommonApp[1]
    uint32_t mGamePaidQueryStartTick; // 0x7A8, unkMemCommonApp[2]

    void *mPaymentManager;          // 0x7AC, unkMemCommonApp[3]
    void *mTrialTimeCheck;          // 0x7B0, unkMemCommonApp[4]
    void *mAdProvider;              // 0x7B4, unkMemCommonApp[5]
    void *mUpdateRemoteListener;    // 0x7B8, unkMemCommonApp[6]
    Widget *mSoundControllerWidget; // 0x7BC, unkMemCommonApp[7]

    void *mGameCenter;          // 0x7C0, unkMemCommonApp[8]
    void *mShareManager;        // 0x7C4, unkMemCommonApp[9]
    void *mNotificationManager; // 0x7C8, unkMemCommonApp[10]
#if PVZ_VERSION == 111
    void *mAnalyticsManager; // 0x7CC, dword 499, 旧版独有
#endif
    bool mAuthenticated; // 0x7CC, unkMemCommonApp[11].byte0

    int mAuthId;           // 0x7D0, unkMemCommonApp[12]
    int mMarketOrAuthMode; // 0x7D4, unkMemCommonApp[13], ctor = 2

    homura::Storage<pvzstl::string> mServer;       // 0x7D8, unkMemCommonApp[14], -s / -server
    homura::Storage<pvzstl::string> mSPay;         // 0x7DC, unkMemCommonApp[15], -spay
    homura::Storage<pvzstl::string> mUserId;       // 0x7E0, unkMemCommonApp[16], -user / -n / -userid
    homura::Storage<pvzstl::string> mStbId;        // 0x7E4, unkMemCommonApp[17], -stbid
    homura::Storage<pvzstl::string> mStbIdDerived; // 0x7E8, unkMemCommonApp[18], stbid hash/derived string

    homura::Storage<pvzstl::string> mLoginUser;      // 0x7EC, unkMemCommonApp[19], -u
    homura::Storage<pvzstl::string> mLoginPassword;  // 0x7F0, unkMemCommonApp[20], -p
    homura::Storage<pvzstl::string> mLoginFlag;      // 0x7F4, unkMemCommonApp[21], -f
    homura::Storage<pvzstl::string> mAppSessionId;   // 0x7F8, unkMemCommonApp[22], -appsessionid
    homura::Storage<pvzstl::string> mGlobalAreaCode; // 0x7FC, unkMemCommonApp[23], -globalareacode

    bool mUnknownCommonFlag800; // 0x800, unkMemCommonApp[24].byte0

    int mTestAdRecallMode;    // 0x804, unkMemCommonApp[25]
    bool mTestAdRecallResult; // 0x808, unkMemCommonApp[26].byte0
    int mTestAdRecallCounter; // 0x80C, unkMemCommonApp[27]

    homura::Storage<pvzstl::string> mTestAdPayItemId;  // 0x810, unkMemCommonApp[28], default "1"
    int mTestAdPayItemCount;                           // 0x814, unkMemCommonApp[29], default 2
    homura::Storage<pvzstl::string> mTestAdPayChannel; // 0x818, unkMemCommonApp[30], default "alipay"

    homura::Storage<pvzstl::string> mUnknownString81C; // 0x81C, unkMemCommonApp[31]

    bool mTrialCheckStarted;     // 0x820, unkMemCommonApp[32].byte0
    bool mTrialCheckDelayPassed; // 0x821, unkMemCommonApp[32].byte1


public:
    static void getGameInfo(pvzstl::string *theStrings, SexyCommonApp *app) {
        return reinterpret_cast<void (*)(pvzstl::string *, SexyCommonApp *)>(Sexy_SexyCommonApp_getGameInfoAddr)(theStrings, app);
    }

protected:
    SexyCommonApp() = default;
    ~SexyCommonApp() = default;
};
} // namespace Sexy

#endif // PVZ_SEXYAPPFRAMEWORK_SEXYCOMMONAPP_H
