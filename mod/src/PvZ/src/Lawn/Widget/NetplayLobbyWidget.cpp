/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 */

#include "PvZ/Lawn/SwitchPad.h"
#include "PvZ/Lawn/Widget/NetplayLobbyWidget.h"

#include "Homura/MemberUtils.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/WaitForSecondPlayerDialog.h"
#include "PvZ/NetPlay.h"
#include "PvZ/SexyAppFramework/Graphics/Font.h"
#include "PvZ/SexyAppFramework/Widget/ScrollWidget.h"
#include "PvZ/TodLib/Common/TodCommon.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <cstring>
#include <algorithm>
#include <mutex>
#include <vector>

using namespace Sexy;

namespace {
constexpr int kLeftPanelX = 40;
constexpr int kPanelYOffset = 15;
constexpr int kLeftPanelY = 120 + kPanelYOffset;
constexpr int kLeftPanelWidth = 330;
constexpr int kPanelHeight = 490;
constexpr int kRightPanelX = 390;
constexpr int kRightPanelY = 120 + kPanelYOffset;
constexpr int kRightPanelWidth = 850;
constexpr int kSelectedServerLatencyX = kRightPanelX + 20;
constexpr int kSelectedServerLatencyY = 170 + kPanelYOffset;
constexpr int kServerRowY = 230 + kPanelYOffset;
constexpr int kServerRowHeight = 55;
constexpr int kRoomScrollX = 410;
constexpr int kRoomScrollY = 193 + kPanelYOffset;
constexpr int kRoomScrollWidth = 810;
constexpr int kRoomScrollHeight = 330;
constexpr int kRoomCardWidth = 390;
constexpr int kRoomCardHeight = 100;
constexpr int kRoomCardGap = 15;
constexpr int kMaxSpectatorNamesShown = 6;
constexpr int kLobbyStatusY = 630;
constexpr int kRoomExitedStatusY = 640;
constexpr int kRoomActionY = 545;
constexpr int kRoomActionWidth = 190;
constexpr int kRoomActionHeight = 45;
constexpr int kRoomActionGap = 15;

void DrawPanel(Graphics *g, const Rect &rect) {
    g->SetColor(Color(66, 36, 20, 210));
    g->FillRect(rect);
    g->SetColor(Color(210, 164, 91, 230));
    g->DrawRect(rect);
}

pvzstl::string BuildRoomTag(const ServerRoomItem &room) {
    if (room.protocolVersion != 0 && room.protocolVersion != NETPLAY_VERSION) {
        return NetplayRoomVersionTag(room.protocolVersion); // Switch port: with the room's build
    }
    if (room.full && room.spectateAllowed) {
        return TodStringTranslate(room.gaming ? "[SPECTATE_QUEUE_AVAILABLE]" : "[SPECTATE_AVAILABLE]");
    }
    if (room.gaming) {
        return TodStringTranslate("[TAG_GAMING]");
    }
    if (room.full) {
        return TodStringTranslate("[TAG_FULL]");
    }
    return TodStringTranslate(room.hostProbeDone ? "[P2P_READY]" : "[P2P_NOT_READY]");
}

pvzstl::string BuildSpectatorsText(const WaitForSecondPlayerDialog *dialog) {
    pvzstl::string text = StrFormat(TodStringTranslate("[SPECTATORS_NUM]").c_str(), dialog->mServerSpectatorCount);
    if (dialog->mServerSpectatorCount <= 0) {
        text += "-";
        return text;
    }

    for (int i = 0; i < dialog->mServerSpectatorCount && i < kMaxSpectatorNamesShown; ++i) {
        if (i > 0) {
            text += ", ";
        }
        text += dialog->mServerSpectatorNames[i][0] != '\0' ? dialog->mServerSpectatorNames[i] : "-";
    }
    if (dialog->mServerSpectatorCount > kMaxSpectatorNamesShown) {
        text += ", ...";
    }
    return text;
}

bool IsRoomActive(const WaitForSecondPlayerDialog *dialog) {
    return dialog != nullptr && (dialog->mIsCreatingRoom || dialog->mIsJoiningRoom || dialog->mServerHosting || dialog->mServerJoined || dialog->mServerSpectating);
}

void DrawActiveRoomInfo(Graphics *g, const WaitForSecondPlayerDialog *dialog) {
    const int centerX = kRightPanelX + kRightPanelWidth / 2;
    if (dialog->mUIMode == UIMode::MODE2_WIFI) {
        const char *title = dialog->mIsCreatingRoom ? "[ROOM_CREATED_FMT]" : "[JOINING_MANUAL]";
        const char *playerName = (dialog->mApp != nullptr && dialog->mApp->mPlayerInfo != nullptr && dialog->mApp->mPlayerInfo->mName != nullptr) ? dialog->mApp->mPlayerInfo->mName : "-";
        const pvzstl::string text = dialog->mIsCreatingRoom ? StrFormat(TodStringTranslate(title).c_str(), playerName) : TodStringTranslate(title);
        TodDrawString(g, text, centerX, 300, FONT_DWARVENTODCRAFT18, Color(255, 238, 175), DS_ALIGN_CENTER);

        const pvzstl::string detail = dialog->mIsCreatingRoom
            ? (IsRemoteServer() ? StrFormat(TodStringTranslate("[OTHER_JOINED_FMT]").c_str(), gSecondPlayerName) : TodStringTranslate("[WAIT_OTHER_JOIN]"))
            : (IsRemoteClient() ? StrFormat(TodStringTranslate("[JOINED_MANUAL_FMT]").c_str(), gSecondPlayerName) : TodStringTranslate("[JOINING_MANUAL]"));
        TodDrawString(g, detail, centerX, 355, FONT_HOUSEOFTERROR20, Color(235, 220, 185), DS_ALIGN_CENTER);
        return;
    }

    const char *roleKey = dialog->mServerHosting ? "[HOST]" : (dialog->mServerSpectating ? "[SPECTATING]" : "[CLIENT]");
    const char *localName = (dialog->mApp != nullptr && dialog->mApp->mPlayerInfo != nullptr && dialog->mApp->mPlayerInfo->mName != nullptr) ? dialog->mApp->mPlayerInfo->mName : "-";
    const char *hostName = dialog->mServerHosting ? localName : (gServerHostName[0] != '\0' ? gServerHostName : "-");
    pvzstl::string guestName = "-";
    if (dialog->mServerHosting) {
        guestName = dialog->mServerHostHasGuest ? (gSecondPlayerName[0] != '\0' ? gSecondPlayerName : "-") : TodStringTranslate("[WAIT_OTHER_JOIN]");
    } else if (dialog->mServerJoined) {
        guestName = localName;
    } else if (gSecondPlayerName[0] != '\0') {
        guestName = gSecondPlayerName;
    }

    TodDrawString(g, roleKey, centerX, 245, FONT_DWARVENTODCRAFT24, Color(255, 238, 175), DS_ALIGN_CENTER);
    TodDrawString(g, StrFormat("%s: %s", TodStringTranslate("[HOST]").c_str(), hostName), centerX, 285, FONT_HOUSEOFTERROR20, Color(235, 220, 185), DS_ALIGN_CENTER);
    TodDrawString(g, StrFormat("%s: %s", TodStringTranslate("[CLIENT]").c_str(), guestName.c_str()), centerX, 325, FONT_HOUSEOFTERROR20, Color(235, 220, 185), DS_ALIGN_CENTER);

    if (dialog->mServerHostSpectateAllowed || dialog->mServerJoinedSpectateAllowed || dialog->mServerSpectating || dialog->mServerSpectatorCount > 0) {
        TodDrawStringWrapped(g, BuildSpectatorsText(dialog), Rect(kRightPanelX + 145, 345, 560, 65), FONT_HOUSEOFTERROR16, Color(235, 220, 185), DS_ALIGN_CENTER, false);
    }
}
} // namespace

class NetplayRoomListWidget : public Widget {
public:
    NetplayLobbyWidget *mOwner;

    explicit NetplayRoomListWidget(NetplayLobbyWidget *owner) {
        Widget::_constructor();
        static void *sVTable[122];
        static std::once_flag sVTableInit;
        std::call_once(sVTableInit, [this] {
            std::memcpy(sVTable, vTable, sizeof(sVTable));
            sVTable[0] = (void *)homura::ExtractMemFuncPtr(&NetplayRoomListWidget::_destructor);
            sVTable[1] = (void *)homura::ExtractMemFuncPtr(&NetplayRoomListWidget::_destructor2);
            sVTable[36] = (void *)homura::ExtractMemFuncPtr(&NetplayRoomListWidget::Draw);
            sVTable[78] = (void *)homura::ExtractMemFuncPtr(&NetplayRoomListWidget::MouseDown);
        });
        vTable = sVTable;
        mOwner = owner;
    }

    ~NetplayRoomListWidget() {
        Widget::_destructor();
    }

    void Draw(Graphics *g) {
        if (mOwner == nullptr || mOwner->mDialog == nullptr) {
            return;
        }
        WaitForSecondPlayerDialog *dialog = mOwner->mDialog;
        const int count = mOwner->GetRoomCount();

        if (count <= 0) {
            if (IsRoomActive(dialog)) {
                return;
            }
            const char *message = dialog->mUIMode == UIMode::MODE3_SERVER ? "[SERVER_NO_ROOMS_TIP]" : "[NO_AVAILABLE_ROOMS]";
            TodDrawString(g, message, kRoomScrollWidth / 2, 150, FONT_HOUSEOFTERROR20, Color(255, 230, 170), DS_ALIGN_CENTER);
            return;
        }

        for (int i = 0; i < count; ++i) {
            const int column = i % 2;
            const int row = i / 2;
            const int x = column * (kRoomCardWidth + kRoomCardGap);
            const int y = row * (kRoomCardHeight + kRoomCardGap);
            const int selected = dialog->mUIMode == UIMode::MODE2_WIFI ? dialog->mSelectedServerIndex : dialog->mSelectedRoomIndex_Server;
            const bool isSelected = i == selected;

            g->SetColor(isSelected ? Color(104, 82, 35, 245) : Color(92, 58, 37, 230));
            g->FillRect(Rect(x, y, kRoomCardWidth, kRoomCardHeight));
            g->SetColor(isSelected ? Color(255, 224, 92) : Color(151, 111, 72));
            g->DrawRect(Rect(x, y, kRoomCardWidth, kRoomCardHeight));

            if (dialog->mUIMode == UIMode::MODE2_WIFI) {
                const auto &room = gServers[i];
                TodDrawString(g, room.name, x + kRoomCardWidth / 2, y + 38, FONT_DWARVENTODCRAFT18, Color(255, 244, 195), DS_ALIGN_CENTER);
                TodDrawString(g, StrFormat("%s:%d", room.ip, room.tcpPort), x + kRoomCardWidth / 2, y + 72, FONT_HOUSEOFTERROR16, Color(208, 208, 208), DS_ALIGN_CENTER);
            } else {
                const ServerRoomItem &room = dialog->mServerRooms[i];
                TodDrawString(g, room.name, x + kRoomCardWidth / 2, y + 38, FONT_DWARVENTODCRAFT18, Color(255, 244, 195), DS_ALIGN_CENTER);
                TodDrawString(g, BuildRoomTag(room), x + kRoomCardWidth / 2, y + 72, FONT_HOUSEOFTERROR16, Color(190, 225, 190), DS_ALIGN_CENTER);
            }
        }
    }

    void MouseDown(int x, int y, int theClickCount) {
        (void)theClickCount;
        if (mOwner == nullptr) {
            return;
        }
        const int column = x / (kRoomCardWidth + kRoomCardGap);
        const int row = y / (kRoomCardHeight + kRoomCardGap);
        if (column < 0 || column > 1 || x - column * (kRoomCardWidth + kRoomCardGap) >= kRoomCardWidth || y - row * (kRoomCardHeight + kRoomCardGap) >= kRoomCardHeight) {
            return;
        }
        mOwner->SelectRoom(row * 2 + column);
    }

protected:
    void _destructor() {
        Widget::_destructor();
    }
    void _destructor2() {
        delete this;
    }
};

NetplayLobbyWidget::NetplayLobbyWidget(WaitForSecondPlayerDialog *dialog) {
    Widget::_constructor();
    static void *sVTable[122];
    static std::once_flag sVTableInit;
    std::call_once(sVTableInit, [this] {
        std::memcpy(sVTable, vTable, sizeof(sVTable));
        sVTable[0] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::_destructor);
        sVTable[1] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::_destructor2);
        sVTable[29] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::AddedToManager);
        sVTable[30] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::RemovedFromManager);
        sVTable[36] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::Draw);
        sVTable[78] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::MouseDown);
    });
    vTable = sVTable;

    mDialog = dialog;
    Resize(LawnApp::FULLSCREEN_RECT.mX, LawnApp::FULLSCREEN_RECT.mY, LawnApp::FULLSCREEN_RECT.mWidth, LawnApp::FULLSCREEN_RECT.mHeight);
    mClip = true;
    mSelectedServerListIndex = 0;
    mZombieBackground = Rand(2);

    mRoomScrollWidget = new ScrollWidget();
    mRoomScrollWidget->Resize(kRoomScrollX, kRoomScrollY, kRoomScrollWidth, kRoomScrollHeight);
    mRoomScrollWidget->SetScrollMode(ScrollWidget::SCROLL_VERTICAL);
    mRoomScrollWidget->EnableBounce(false);
    mRoomListWidget = new NetplayRoomListWidget(this);
    mRoomListWidget->Resize(0, 0, kRoomScrollWidth, kRoomScrollHeight);

    mAddServerButton = MakeButton(NetplayLobbyWidget_AddServer, dialog, this, "[CONNECT_CUSTOM_SERVER]");
    mAddServerButton->Resize(60, 545 + kPanelYOffset, 290, 50);
    mCreateRoomButton = MakeButton(WaitForSecondPlayerDialog::WaitForSecondPlayerDialog_Right, dialog, this, "[CREATE_ROOM_BUTTON]");
    mCreateRoomButton->Resize(410, 545 + kPanelYOffset, 390, 50);
    mJoinRoomButton = MakeButton(WaitForSecondPlayerDialog::WaitForSecondPlayerDialog_Left, dialog, this, "[JOIN_ROOM_BUTTON]");
    mJoinRoomButton->Resize(830, 545 + kPanelYOffset, 390, 50);
    mReplayManageButton = MakeButton(NetplayLobbyWidget_ReplayManage, dialog, this, "[REPLAY_MANAGE]");
    mReplayManageButton->Resize(40, 650, 230, 50);
    mLocalBattleButton = MakeButton(NetplayLobbyWidget_LocalBattle, dialog, this, "[PLAY_OFFLINE]");
    mLocalBattleButton->Resize(525, 650, 230, 50);
    mBackButton = MakeButton(NetplayLobbyWidget_Back, dialog, this, "[BACK]");
    mBackButton->Resize(1010, 650, 230, 50);
    mPrimaryActionButton = MakeButton(NetplayLobbyWidget_PrimaryAction, dialog, this, "[START_GAME]");
    mPrimaryActionButton->Resize(410, 465 + kPanelYOffset, 390, 48);
    mRoomOptionButton = MakeButton(NetplayLobbyWidget_RoomOption, dialog, this, "[ENABLE_SPECTATE]");
    mRoomOptionButton->Resize(830, 465 + kPanelYOffset, 390, 48);
    mPrimaryActionButton->SetVisible(false);
    mRoomOptionButton->SetVisible(false);

    TodLoadResources("DelayLoad_Almanac");
}

NetplayLobbyWidget::~NetplayLobbyWidget() {
    _destructor();
}

void NetplayLobbyWidget::_destructor() {
    delete mRoomOptionButton;
    delete mPrimaryActionButton;
    delete mBackButton;
    delete mLocalBattleButton;
    delete mReplayManageButton;
    delete mJoinRoomButton;
    delete mCreateRoomButton;
    delete mAddServerButton;
    delete mRoomListWidget;
    delete mRoomScrollWidget;
    Widget::_destructor();
}

void NetplayLobbyWidget::_destructor2() {
    delete this;
}

void NetplayLobbyWidget::AddedToManager(WidgetManager *theWidgetManager) {
    WidgetContainer::AddedToManager(theWidgetManager);
    AddWidget(mRoomScrollWidget);
    mRoomScrollWidget->AddWidget(mRoomListWidget);
    AddWidget(mAddServerButton);
    AddWidget(mCreateRoomButton);
    AddWidget(mJoinRoomButton);
    AddWidget(mReplayManageButton);
    AddWidget(mLocalBattleButton);
    AddWidget(mBackButton);
    AddWidget(mPrimaryActionButton);
    AddWidget(mRoomOptionButton);
}

void NetplayLobbyWidget::RemovedFromManager(WidgetManager *theWidgetManager) {
    WidgetContainer::RemovedFromManager(theWidgetManager);
    mRoomScrollWidget->RemoveWidget(mRoomListWidget);
    RemoveWidget(mRoomScrollWidget);
    RemoveWidget(mAddServerButton);
    RemoveWidget(mCreateRoomButton);
    RemoveWidget(mJoinRoomButton);
    RemoveWidget(mReplayManageButton);
    RemoveWidget(mLocalBattleButton);
    RemoveWidget(mBackButton);
    RemoveWidget(mPrimaryActionButton);
    RemoveWidget(mRoomOptionButton);
}

int NetplayLobbyWidget::GetRoomCount() const {
    if (mDialog == nullptr || mDialog->mIsCreatingRoom || mDialog->mIsJoiningRoom || mDialog->mServerHosting || mDialog->mServerJoined || mDialog->mServerSpectating) {
        return 0;
    }
    if (mDialog->mUIMode == UIMode::MODE2_WIFI) {
        return std::max(0, gScannedServerCount);
    }
    if (mDialog->mUIMode == UIMode::MODE3_SERVER && mDialog->mServerConnected) {
        return std::max(0, mDialog->mServerRoomCount);
    }
    return 0;
}

void NetplayLobbyWidget::RefreshControls() {
    if (mDialog == nullptr) {
        return;
    }
    mDialog->RefreshButtons();
    mJoinRoomButton->SetLabel(*mDialog->mLeftButton->mLabel);
    mJoinRoomButton->mDisabled = mDialog->mLeftButton->mDisabled;
    mCreateRoomButton->SetLabel(*mDialog->mRightButton->mLabel);
    mCreateRoomButton->mDisabled = mDialog->mRightButton->mDisabled;

    const bool lanRoomActive = mDialog->mIsCreatingRoom || mDialog->mIsJoiningRoom;
    const bool serverRoomActive = mDialog->mServerHosting || mDialog->mServerJoined || mDialog->mServerSpectating;
    const bool roomActive = lanRoomActive || serverRoomActive;
    const bool serverIdleDisconnected = mDialog->mUIMode == UIMode::MODE3_SERVER && !mDialog->mServerConnected && !mDialog->mServerConnecting && !serverRoomActive;
    if (serverIdleDisconnected) {
        mCreateRoomButton->SetLabel("[CREATE_ROOM_BUTTON]");
        mCreateRoomButton->mDisabled = true;
    }

    // These two controls are fixed parts of the lobby layout. Context-sensitive
    // room actions are presented separately inside the room panel below.
    mAddServerButton->SetLabel("[CONNECT_CUSTOM_SERVER]");
    mAddServerButton->mDisabled = lanRoomActive || serverRoomActive || mDialog->mServerConnecting;
    mReplayManageButton->SetLabel("[REPLAY_MANAGE]");
    mReplayManageButton->mDisabled = lanRoomActive || serverRoomActive || mDialog->mServerConnected || mDialog->mServerConnecting;
    mLocalBattleButton->SetLabel("[PLAY_OFFLINE]");
    mLocalBattleButton->mDisabled = roomActive || mDialog->mServerConnecting;

    const bool showPrimaryAction = mDialog->mIsCreatingRoom || serverRoomActive;
    mPrimaryActionButton->SetVisible(showPrimaryAction);
    if (showPrimaryAction) {
        mPrimaryActionButton->SetLabel(*mDialog->mLawnYesButton->mLabel);
        mPrimaryActionButton->mDisabled = mDialog->mLawnYesButton->mDisabled;
    }

    const bool showRoomOption = mDialog->mUIMode == UIMode::MODE3_SERVER && mDialog->mServerHosting;
    mRoomOptionButton->SetVisible(showRoomOption);
    if (showRoomOption) {
        mRoomOptionButton->SetLabel(*mDialog->mLawnNoButton->mLabel);
        mRoomOptionButton->mDisabled = mDialog->mLawnNoButton->mDisabled;
    }

    if (roomActive) {
        GameButton *roomActions[4];
        int roomActionCount = 0;
        if (showPrimaryAction) {
            roomActions[roomActionCount++] = mPrimaryActionButton;
        }
        if (showRoomOption) {
            roomActions[roomActionCount++] = mRoomOptionButton;
        }
        // Hosts keep "kick guest" before "leave room". Guests and spectators
        // put the role-switch action before "leave room".
        if (mDialog->mServerJoined || mDialog->mServerSpectating) {
            roomActions[roomActionCount++] = mCreateRoomButton;
            roomActions[roomActionCount++] = mJoinRoomButton;
        } else {
            roomActions[roomActionCount++] = mJoinRoomButton;
            roomActions[roomActionCount++] = mCreateRoomButton;
        }

        const int actionsWidth = roomActionCount * kRoomActionWidth + (roomActionCount - 1) * kRoomActionGap;
        const int actionsX = kRoomScrollX + (kRoomScrollWidth - actionsWidth) / 2;
        for (int i = 0; i < roomActionCount; ++i) {
            roomActions[i]->Resize(actionsX + i * (kRoomActionWidth + kRoomActionGap), kRoomActionY, kRoomActionWidth, kRoomActionHeight);
        }
    } else {
        mCreateRoomButton->Resize(410, 545 + kPanelYOffset, 390, 50);
        mJoinRoomButton->Resize(830, 545 + kPanelYOffset, 390, 50);
        mPrimaryActionButton->Resize(410, 465 + kPanelYOffset, 390, 48);
        mRoomOptionButton->Resize(830, 465 + kPanelYOffset, 390, 48);
    }
}

void NetplayLobbyWidget::Draw(Graphics *g) {
    RefreshControls();
    const int roomRows = (GetRoomCount() + 1) / 2;
    const int roomContentHeight = std::max(kRoomScrollHeight, roomRows * kRoomCardHeight + std::max(0, roomRows - 1) * kRoomCardGap);
    if (mRoomListWidget->mHeight != roomContentHeight) {
        mRoomListWidget->Resize(0, 0, kRoomScrollWidth, roomContentHeight);
        // ScrollWidget caches its scroll range. Room lists are refreshed by the
        // networking layer, so explicitly update the cached range when their
        // content height changes.
        mRoomScrollWidget->ClientSizeChanged();
    }
    g->DrawImage(mZombieBackground ? IMAGE_ALMANAC_ZOMBIEBACK : IMAGE_ALMANAC_PLANTBACK, 0, 0);
    g->SetColor(Color(25, 15, 10, 75));
    g->FillRect(Rect(0, 0, mWidth, mHeight));
    DrawPanel(g, Rect(kLeftPanelX, kLeftPanelY, kLeftPanelWidth, kPanelHeight));
    DrawPanel(g, Rect(kRightPanelX, kRightPanelY, kRightPanelWidth, kPanelHeight));

    TodDrawString(g, "[NETPLAY_LOBBY_TITLE]", mWidth / 2, 115, addonFonts.JN_BOBO_HEI36, Color(255, 248, 195), DS_ALIGN_CENTER);
    TodDrawString(g, "[MODE_SERVER_TITLE]", kLeftPanelX + kLeftPanelWidth / 2, 170 + kPanelYOffset, FONT_DWARVENTODCRAFT18, Color(255, 226, 154), DS_ALIGN_CENTER);
    TodDrawString(g, "[AVAILABLE_ROOMS]", kRightPanelX + kRightPanelWidth / 2, 170 + kPanelYOffset, FONT_DWARVENTODCRAFT18, Color(255, 226, 154), DS_ALIGN_CENTER);

    if (IsRoomActive(mDialog)) {
        DrawActiveRoomInfo(g, mDialog);
    }

    const int targetCount = mDialog != nullptr ? mDialog->GetLobbyServerTargetCount() : 0;
    const int itemCount = 1 + targetCount;
    mSelectedServerListIndex = std::clamp(mSelectedServerListIndex, 0, std::max(0, itemCount - 1));
    for (int i = 0; i < itemCount; ++i) {
        const int y = kServerRowY + i * kServerRowHeight;
        const bool selected = i == mSelectedServerListIndex;
        g->SetColor(selected ? Color(95, 105, 45, 245) : Color(82, 58, 42, 225));
        g->FillRect(Rect(kLeftPanelX + 18, y - 30, kLeftPanelWidth - 36, 45));
        g->SetColor(selected ? Color(155, 225, 70) : Color(145, 105, 72));
        g->DrawRect(Rect(kLeftPanelX + 18, y - 30, kLeftPanelWidth - 36, 45));

        pvzstl::string label;
        if (i == 0) {
            label = TodStringTranslate("[WIFI_VS]");
        } else {
            char address[32]{};
            mDialog->GetLobbyServerTargetAddress(i - 1, address, sizeof(address));
            if (i <= 2) {
                label = StrFormat(TodStringTranslate("[OFFICIAL_SERVER_NAME]").c_str(), i, address);
            } else {
                label = StrFormat(TodStringTranslate("[CUSTOM_SERVER_NAME]").c_str(), i - 2, address);
            }
        }
        TodDrawString(g, label, kLeftPanelX + kLeftPanelWidth / 2, y, FONT_HOUSEOFTERROR16, selected ? Color(185, 255, 105) : Color(255, 238, 195), DS_ALIGN_CENTER);
    }

    if (mDialog != nullptr && mSelectedServerListIndex > 0 && mSelectedServerListIndex <= targetCount) {
        // Target probes are stopped and cleared after connecting. From that
        // point the room-list query round trip is the current server latency.
        const int latency = mDialog->mServerConnected ? mDialog->mServerLatencyMs : mDialog->mServerTargetLatencyMs[mSelectedServerListIndex - 1];
        const pvzstl::string latencyText = latency >= 0 ? StrFormat("%dms", latency) : "--ms";
        TodDrawString(g, latencyText, kSelectedServerLatencyX, kSelectedServerLatencyY, FONT_HOUSEOFTERROR16, Color(235, 220, 185), DS_ALIGN_LEFT);
    }

    if (mDialog != nullptr) {
        pvzstl::string status;
        if (mDialog->mUIMode == UIMode::MODE3_SERVER) {
            status = mDialog->mServerStatusText;
        } else if (mDialog->mIsCreatingRoom) {
            status = TodStringTranslate("[WAIT_OTHER_JOIN]");
        } else if (mDialog->mIsJoiningRoom) {
            status = TodStringTranslate("[JOINING_MANUAL]");
        } else {
            status = TodStringTranslate(gScannedServerCount > 0 ? "[AVAILABLE_ROOMS]" : "[SCANNING_ROOMS]");
        }
        const bool roomExited = status == TodStringTranslate("[STATUS_ROOM_EXITED]");
        TodDrawString(g, status, kRightPanelX + kRightPanelWidth / 2, roomExited ? kRoomExitedStatusY : kLobbyStatusY, FONT_HOUSEOFTERROR16, Color(235, 220, 185), DS_ALIGN_CENTER);
    }
    PadDraw(g);
}

// ----------------------------------------------------------------------------
// Controller support (the Switch port). The lobby was made for touch: its
// server list and room cards are hit areas, and nothing handled the keys.
// The D-pad / stick move a highlight between the server rows, the room cards
// on show and the buttons, by where they are; in the rooms, up and down go a
// row at a time and scroll the list. A selects a server, joins the room, or
// presses the button; X creates a room and Y joins the selected one from
// anywhere (their pictures are on those buttons); B leaves, as before (the
// dialog's own KeyDown). The highlight is the thing chosen, not a place, so
// the list scrolling under it cannot move it.
// ----------------------------------------------------------------------------
namespace {

enum LobbySpotKind { SPOT_SERVER, SPOT_ROOM, SPOT_BUTTON };

struct LobbySpot {
    Rect rect;          // in the lobby's coordinates
    LobbySpotKind kind;
    int index;          // the server row / room, or the button's id
};

struct {
    bool active = false;
    LobbySpotKind kind = SPOT_SERVER;
    int index = 0;
    int tick = 0; // frames, for the glow
} gLobbyPad;

int SpotX(const LobbySpot &s) {
    return s.rect.mX + s.rect.mWidth / 2;
}

int SpotY(const LobbySpot &s) {
    return s.rect.mY + s.rect.mHeight / 2;
}

Rect RoomCardRect(int i) { // in the room list's coordinates
    return Rect((i % 2) * (kRoomCardWidth + kRoomCardGap), (i / 2) * (kRoomCardHeight + kRoomCardGap), kRoomCardWidth, kRoomCardHeight);
}

Rect RoomCardOnScreen(NetplayLobbyWidget &w, int i) {
    const Rect c = RoomCardRect(i);
    return Rect(w.mRoomScrollWidget->mX + w.mRoomListWidget->mX + c.mX, w.mRoomScrollWidget->mY + w.mRoomListWidget->mY + c.mY, c.mWidth, c.mHeight);
}

void AddLobbyButton(std::vector<LobbySpot> &v, GameButton *b) {
    if (b != nullptr && b->mVisible && !b->mDisabled && !b->mBtnNoDraw) {
        v.push_back({Rect(b->mX, b->mY, b->mWidth, b->mHeight), SPOT_BUTTON, b->mWidgetId});
    }
}

bool ButtonUsable(const GameButton *b) {
    return b != nullptr && b->mVisible && !b->mDisabled && !b->mBtnNoDraw;
}

// What the highlight can go to: the server rows, the room cards at least half
// on show (the rest are reached through the rooms themselves), the buttons.
void CollectLobbySpots(NetplayLobbyWidget &w, std::vector<LobbySpot> &v) {
    v.clear();
    if (w.mDialog == nullptr) {
        return;
    }
    const int servers = 1 + w.mDialog->GetLobbyServerTargetCount();
    for (int i = 0; i < servers; ++i) {
        v.push_back({Rect(kLeftPanelX + 18, kServerRowY - 30 + i * kServerRowHeight, kLeftPanelWidth - 36, 45), SPOT_SERVER, i});
    }
    const int viewTop = w.mRoomScrollWidget->mY, viewBottom = w.mRoomScrollWidget->mY + w.mRoomScrollWidget->mHeight;
    for (int i = 0; i < w.GetRoomCount(); ++i) {
        const Rect r = RoomCardOnScreen(w, i);
        if (r.mY + r.mHeight / 2 >= viewTop && r.mY + r.mHeight / 2 <= viewBottom) {
            v.push_back({r, SPOT_ROOM, i});
        }
    }
    AddLobbyButton(v, w.mPrimaryActionButton);
    AddLobbyButton(v, w.mRoomOptionButton);
    AddLobbyButton(v, w.mAddServerButton);
    AddLobbyButton(v, w.mCreateRoomButton);
    AddLobbyButton(v, w.mJoinRoomButton);
    AddLobbyButton(v, w.mReplayManageButton);
    AddLobbyButton(v, w.mLocalBattleButton);
    AddLobbyButton(v, w.mBackButton);
}

int FindSpot(const std::vector<LobbySpot> &v, LobbySpotKind kind, int index) {
    for (int i = 0; i < static_cast<int>(v.size()); ++i) {
        if (v[i].kind == kind && v[i].index == index) {
            return i;
        }
    }
    return -1;
}

// The spot the highlight goes to from `from` in a direction: the nearest one
// that way, a spot straight ahead beating one off to the side.
int StepSpot(const std::vector<LobbySpot> &v, const Rect &from, int dx, int dy) {
    const int x = from.mX + from.mWidth / 2, y = from.mY + from.mHeight / 2;
    int best = -1;
    long bestCost = 0;
    for (int i = 0; i < static_cast<int>(v.size()); ++i) {
        const long ox = SpotX(v[i]) - x, oy = SpotY(v[i]) - y;
        const long ahead = dx != 0 ? ox * dx : oy * dy;
        const long aside = dx != 0 ? (oy < 0 ? -oy : oy) : (ox < 0 ? -ox : ox);
        if (ahead <= 8) {
            continue;
        }
        const long cost = ahead + aside * 3;
        if (best < 0 || cost < bestCost) {
            best = i;
            bestCost = cost;
        }
    }
    return best;
}

void DrawButtonPicture(Graphics *g, const GameButton *b, int theCel) {
    if (!ButtonUsable(b) || IMAGE_HELP_BUTTONS_SMALL == nullptr) {
        return;
    }
    const int cel = IMAGE_HELP_BUTTONS_SMALL->GetCelWidth(), h = IMAGE_HELP_BUTTONS_SMALL->GetCelHeight();
    g->DrawImageCel(IMAGE_HELP_BUTTONS_SMALL, b->mX + 14, b->mY + (b->mHeight - h) / 2 - 2, theCel);
    (void)cel;
}

} // namespace

bool NetplayLobbyWidget::PadKeyDown(Sexy::KeyCode theKey) {
    if (!mVisible || mDialog == nullptr) {
        return false;
    }
    // X and Y: create a room, join the one chosen, from anywhere
    if (theKey == KEYCODE_GAMEPAD_X || theKey == KEYCODE_GAMEPAD_Y) {
        GameButton *b = theKey == KEYCODE_GAMEPAD_X ? mCreateRoomButton : mJoinRoomButton;
        if (!ButtonUsable(b)) {
            return false;
        }
        mDialog->ButtonDepress_Thunk(b->mWidgetId); // may close the lobby: nothing after this
        return true;
    }
    int dx = 0, dy = 0;
    switch (theKey) {
        case KEYCODE_LEFT:
        case KEYCODE_GAMEPAD_LEFT:
        case KEYCODE_GAMEPAD_DPAD_LEFT:
            dx = -1;
            break;
        case KEYCODE_RIGHT:
        case KEYCODE_GAMEPAD_RIGHT:
        case KEYCODE_GAMEPAD_DPAD_RIGHT:
            dx = 1;
            break;
        case KEYCODE_UP:
        case KEYCODE_GAMEPAD_UP:
        case KEYCODE_GAMEPAD_DPAD_UP:
            dy = -1;
            break;
        case KEYCODE_DOWN:
        case KEYCODE_GAMEPAD_DOWN:
        case KEYCODE_GAMEPAD_DPAD_DOWN:
            dy = 1;
            break;
        default:
            break;
    }
    const bool press = theKey == KEYCODE_GAMEPAD_A || theKey == KEYCODE_RETURN;
    if (dx == 0 && dy == 0 && !press) {
        return false; // B and the rest: the dialog's
    }
    std::vector<LobbySpot> spots;
    CollectLobbySpots(*this, spots);
    if (spots.empty()) {
        return false;
    }
    const int rooms = GetRoomCount();
    int at = gLobbyPad.active ? FindSpot(spots, gLobbyPad.kind, gLobbyPad.index) : -1;
    if (at < 0 && gLobbyPad.active && gLobbyPad.kind == SPOT_ROOM && gLobbyPad.index < rooms) {
        at = -2; // a room scrolled out of view: still the one chosen
    }
    if (!gLobbyPad.active || at == -1) { // the first press shows the highlight: a room, else Create
        gLobbyPad.active = true;
        const int room = FindSpot(spots, SPOT_ROOM, 0) >= 0 ? 0 : -1;
        if (room >= 0) {
            gLobbyPad.kind = SPOT_ROOM, gLobbyPad.index = room;
            SelectRoom(room);
        } else if (ButtonUsable(mCreateRoomButton)) {
            gLobbyPad.kind = SPOT_BUTTON, gLobbyPad.index = mCreateRoomButton->mWidgetId;
        } else {
            gLobbyPad.kind = spots.front().kind, gLobbyPad.index = spots.front().index;
        }
        return true;
    }
    if (dx != 0 || dy != 0) {
        if (gLobbyPad.kind == SPOT_ROOM) { // the rooms: a row, or a column, at a time
            const int i = gLobbyPad.index;
            const int to = dy != 0 ? i + 2 * dy : ((i % 2 == 0 && dx > 0) || (i % 2 == 1 && dx < 0) ? i + dx : -1);
            if (to >= 0 && to < rooms) {
                gLobbyPad.index = to;
                SelectRoom(to);
                mRoomScrollWidget->ScrollRectIntoView(RoomCardRect(to), false);
                return true;
            }
        }
        const Rect from = at >= 0 ? spots[at].rect : RoomCardOnScreen(*this, gLobbyPad.index);
        const int to = StepSpot(spots, from, dx, dy);
        if (to >= 0) {
            gLobbyPad.kind = spots[to].kind;
            gLobbyPad.index = spots[to].index;
            if (spots[to].kind == SPOT_ROOM) {
                SelectRoom(spots[to].index);
            }
        }
        return true;
    }
    switch (gLobbyPad.kind) {
        case SPOT_SERVER:
            SelectServer(gLobbyPad.index);
            break;
        case SPOT_ROOM: // join the room under the highlight
            SelectRoom(gLobbyPad.index);
            if (ButtonUsable(mJoinRoomButton)) {
                mDialog->ButtonDepress_Thunk(mJoinRoomButton->mWidgetId);
            }
            break;
        default:
            if (at >= 0) {
                mDialog->ButtonDepress_Thunk(gLobbyPad.index); // may close the lobby: nothing after this
            }
            break;
    }
    return true;
}

void NetplayLobbyWidget::PadDraw(Graphics *g) {
    // the buttons' own controller buttons: X Create, Y Join, B Back
    DrawButtonPicture(g, mCreateRoomButton, 2);
    DrawButtonPicture(g, mJoinRoomButton, 3);
    DrawButtonPicture(g, mBackButton, 1);
    if (!gLobbyPad.active) {
        return;
    }
    std::vector<LobbySpot> spots;
    CollectLobbySpots(*this, spots);
    const int i = FindSpot(spots, gLobbyPad.kind, gLobbyPad.index);
    if (i < 0) {
        return; // a room scrolled out of view, or a button gone: nothing lit
    }
    const Rect &r = spots[i].rect;
    switch (spots[i].kind) {
        case SPOT_BUTTON: { // the stone buttons: the main menu's purple glow, the stone's own shape
            // (drawn now, before the buttons: each covers the middle of its own)
            const Sexy::Widget *b = FindWidget(gLobbyPad.index);
            switchpad::DrawStoneGlow(g, r.mX, r.mY, r.mWidth, b != nullptr && b->mIsDown, ++gLobbyPad.tick);
            break;
        }
        case SPOT_SERVER: // a server row: a light line round it (A picks it)
            g->SetColor(Color(255, 236, 150, 220));
            g->DrawRect(Rect(r.mX - 1, r.mY - 1, r.mWidth + 1, r.mHeight + 1));
            g->DrawRect(Rect(r.mX - 2, r.mY - 2, r.mWidth + 3, r.mHeight + 3));
            break;
        default: // a room: already lit as the one selected
            break;
    }
}

void NetplayLobbyWidget::MouseDown(int x, int y, int theClickCount) {
    (void)theClickCount;
    gLobbyPad.active = false; // a finger: the highlight goes until the next button
    const int firstTop = kServerRowY - 30;
    if (x < kLeftPanelX + 18 || x >= kLeftPanelX + kLeftPanelWidth - 18 || y < firstTop) {
        return;
    }
    const int index = (y - firstTop) / kServerRowHeight;
    if (y - (firstTop + index * kServerRowHeight) >= 45) {
        return;
    }
    SelectServer(index);
}

void NetplayLobbyWidget::SelectServer(int listIndex) {
    if (mDialog == nullptr || listIndex < 0 || listIndex > mDialog->GetLobbyServerTargetCount()) {
        return;
    }
    if (listIndex != mSelectedServerListIndex && mDialog->ServerHostRoomLocked()) {
        return;
    }
    if (listIndex == mSelectedServerListIndex && ((listIndex == 0 && mDialog->mUIMode == UIMode::MODE2_WIFI) || (listIndex > 0 && (mDialog->mServerConnected || mDialog->mServerConnecting)))) {
        return;
    }
    mSelectedServerListIndex = listIndex;
    mRoomScrollWidget->ScrollToMin(false);
    mDialog->mApp->PlaySample(SOUND_GRAVEBUTTON);
    if (listIndex == 0) {
        mDialog->SetMode(UIMode::MODE2_WIFI);
    } else {
        mDialog->SetMode(UIMode::MODE3_SERVER);
        mDialog->ConnectLobbyServerTarget(listIndex - 1);
    }
}

void NetplayLobbyWidget::SelectRoom(int roomIndex) {
    if (mDialog == nullptr || roomIndex < 0 || roomIndex >= GetRoomCount()) {
        return;
    }
    if (mDialog->mUIMode == UIMode::MODE2_WIFI) {
        mDialog->mSelectedServerIndex = roomIndex;
    } else {
        mDialog->mSelectedRoomIndex_Server = roomIndex;
    }
    mDialog->mApp->PlaySample(SOUND_GRAVEBUTTON);
}
