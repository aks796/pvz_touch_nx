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

#include "PvZ/Lawn/Widget/ModMenuDialog.h"

#include "Homura/MemberUtils.h"
#include "PvZ/Cheat.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/HelpOptionsDialog.h"
#include "PvZ/ModFeature.h"
#include "PvZ/SexyAppFramework/Graphics/Color.h"
#include "PvZ/SexyAppFramework/Graphics/Font.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Widget/WidgetManager.h"
#include "PvZ/Symbols.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <vector>

using namespace Sexy;

namespace {
// The menu's rows: its entries, parsed once from the list the Java menu shows
// ("[num_][CollapseAdd_]Type_Label[_option,option...]").
enum class RowKind { Section, Toggle, Once, Spinner, Value, Caption };

struct MenuRow {
    RowKind kind;
    int feat;    // the entry's number (ApplyModFeature); -1: none
    int section; // the index of its Section row; -1 for a Section
    std::string label;
    std::vector<std::string> options; // a Spinner's
    Color color;                      // a Caption's
};

constexpr int kRowHeight = 40;     // a section's rows
constexpr int kSectionGapY = 16;   // between the section buttons
constexpr int kSectionGapX = 24;

// Where things go; set for the dialog's frame (ModMenuDialog's constructor).
struct Layout {
    int width, height;      // the frame: whole tiles
    int listX, listWidth;   // the rows
    int listY, rows;
    int footerY;            // the hint line's baseline
    int centerX;            // the middle of the dark part
} L;
constexpr int kRepeatDelay = 35; // updates (100 a second) before a held direction repeats
constexpr int kRepeatEvery = 7;
constexpr int kFastAfter = 150; // a held InputValue then steps by 10
constexpr int kFlashTicks = 120;
constexpr int kValueMax = 99999;

enum { DIR_UP = 1, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

std::vector<MenuRow> gRows;
std::map<int, int> gValues; // Toggle/Spinner/Value entries: feat -> state
std::set<int> gChosen;      // the entries set in this menu (saved; config.ini sets the others)
bool gDirty;                // gChosen changed since the last save
int gOpen = -1;             // the open section's row; -1: the sections themselves
int gSectionAt;             // the section button selected (an index into gSections)
int gCursor;                // the selected row of the open section
std::vector<int> gSections; // the Section rows with something to set, in order

using StoneButtonFunc = void (*)(Graphics *, int, int, int, int, bool, bool, const pvzstl::string &, bool);
using UpdateFunc = void (*)(Widget *);
using DrawFunc = void (*)(Widget *, Graphics *);
UpdateFunc gBaseUpdate;
DrawFunc gBaseDraw;

// Strips the entry's HTML (keeping a <font color> for captions) and turns the
// full-width colon into ':'.
std::string CleanText(std::string_view s, Color *color) {
    std::string out;
    for (size_t i = 0; i < s.size();) {
        if (s[i] == '<') {
            const size_t end = s.find('>', i);
            if (end == std::string_view::npos) {
                break;
            }
            const std::string_view tag = s.substr(i, end + 1 - i);
            if (color != nullptr && tag.find("yellow") != std::string_view::npos) {
                *color = Color(255, 230, 90);
            } else if (color != nullptr && tag.find("green") != std::string_view::npos) {
                *color = Color(140, 230, 120);
            }
            i = end + 1;
        } else if (s.substr(i, 3) == "\xEF\xBC\x9A") {
            out += ':';
            i += 3;
        } else {
            out += s[i++];
        }
    }
    const size_t first = out.find_first_not_of(' ');
    if (first == std::string::npos) {
        return {};
    }
    return out.substr(first, out.find_last_not_of(' ') + 1 - first);
}

void BuildRows() {
    if (!gRows.empty()) {
        return;
    }
    int section = -1;
    for (const char *entry : cheat::lang::en_US::featureList) {
        std::string_view e(entry);
        int feat = -1;
        const size_t numEnd = e.find('_');
        if (numEnd != std::string_view::npos && numEnd > 0 && e.substr(0, numEnd).find_first_not_of("-0123456789") == std::string_view::npos) {
            feat = std::atoi(std::string(e.substr(0, numEnd)).c_str());
            e.remove_prefix(numEnd + 1);
        }
        if (e.starts_with("Collapse_")) {
            section = static_cast<int>(gRows.size());
            gRows.push_back({RowKind::Section, -1, -1, CleanText(e.substr(9), nullptr), {}, Color()});
            continue;
        }
        if (e.starts_with("CollapseAdd_")) {
            e.remove_prefix(12);
        }
        const size_t typeEnd = e.find('_');
        if (typeEnd == std::string_view::npos) {
            continue;
        }
        const std::string_view type = e.substr(0, typeEnd);
        std::string_view rest = e.substr(typeEnd + 1);
        MenuRow row{RowKind::Caption, feat, section, {}, {}, Color(225, 225, 225)};
        if (type == "Toggle" || type == "CheckBox") {
            row.kind = RowKind::Toggle;
        } else if (type == "OnceCheckBox" || type == "Button") {
            row.kind = RowKind::Once;
        } else if (type == "Spinner") {
            row.kind = RowKind::Spinner;
        } else if (type == "InputValue") {
            row.kind = RowKind::Value;
        } else if (type != "RichTextView") {
            continue; // FormationCopy, InputText: formation codes through the clipboard, Android's
        }
        if (row.kind == RowKind::Spinner) {
            const size_t labelEnd = rest.find('_');
            if (labelEnd == std::string_view::npos) {
                continue;
            }
            std::string_view options = rest.substr(labelEnd + 1);
            rest = rest.substr(0, labelEnd);
            for (;;) {
                const size_t comma = options.find(',');
                row.options.push_back(CleanText(options.substr(0, comma), nullptr));
                if (comma == std::string_view::npos) {
                    break;
                }
                options.remove_prefix(comma + 1);
            }
        }
        row.label = CleanText(rest, &row.color);
        if (feat == 125 || row.label.find("Export/Import") != std::string::npos) {
            continue; // deploys the pasted formation code: nothing to paste here
        }
        if (row.kind != RowKind::Caption && feat < 0) {
            continue;
        }
        gRows.push_back(std::move(row));
    }
    for (int i = 0; i < static_cast<int>(gRows.size()); ++i) {
        if (gRows[i].kind != RowKind::Section) {
            continue;
        }
        const bool any = std::any_of(gRows.begin(), gRows.end(), [i](const MenuRow &r) { return r.section == i && r.kind != RowKind::Caption; });
        if (any) {
            gSections.push_back(i);
        }
    }
}

int DefaultValue(int feat) {
    return feat == 85 ? 1 : 0; // targetWavesToJump starts at 1
}

int ValueOf(const MenuRow &row) {
    if (row.feat == 5) {
        return requestPause ? 1 : 0; // the advanced pause: also (+) and replays
    }
    const auto it = gValues.find(row.feat);
    return it != gValues.end() ? it->second : DefaultValue(row.feat);
}

void Apply(const MenuRow &row, int value) {
    switch (row.kind) {
        case RowKind::Toggle:
            ApplyModFeature(row.feat, 0, value != 0, {});
            break;
        case RowKind::Spinner:
        case RowKind::Value:
            ApplyModFeature(row.feat, value, false, {});
            break;
        case RowKind::Once:
            ApplyModFeature(row.feat, 0, true, {});
            return;
        default:
            return;
    }
    gValues[row.feat] = value;
    gChosen.insert(row.feat);
    gDirty = true;
}

std::string SettingsPath() {
    const char *dir = std::getenv("ANDROID_FILES_DIR");
    if (dir == nullptr || *dir == '\0') {
        dir = std::getenv("HOME");
    }
    return dir == nullptr || *dir == '\0' ? std::string() : std::string(dir) + "/modmenu.txt";
}

void SaveSettings() {
    if (!gDirty) {
        return;
    }
    const std::string path = SettingsPath();
    FILE *file = path.empty() ? nullptr : std::fopen(path.c_str(), "w");
    if (file == nullptr) {
        return;
    }
    std::fputs("# Help & Options > Mod Menu: the settings chosen there (entry=value)\n", file);
    for (const int feat : gChosen) {
        if (feat != 5) { // the advanced pause is not kept
            std::fprintf(file, "%d=%d\n", feat, gValues[feat]);
        }
    }
    std::fclose(file);
    gDirty = false;
}

bool InLevel(LawnApp *app) {
    return app->mBoard != nullptr && !isMainMenu;
}

// An action that works outside a level too: the level jump dialog.
bool NeedsLevel(const MenuRow &row) {
    return row.kind == RowKind::Once && row.feat != 81;
}

bool IsVisible(int i) {
    return gOpen >= 0 && gRows[i].section == gOpen;
}

bool IsSelectable(int i) {
    return gRows[i].kind != RowKind::Caption && IsVisible(i);
}

std::vector<int> VisibleRows() {
    std::vector<int> rows;
    for (int i = 0; i < static_cast<int>(gRows.size()); ++i) {
        if (IsVisible(i)) {
            rows.push_back(i);
        }
    }
    return rows;
}

int DirOf(KeyCode key) {
    switch (key) {
        case KeyCode::KEYCODE_UP:
        case KeyCode::KEYCODE_GAMEPAD_UP:
        case KeyCode::KEYCODE_GAMEPAD_DPAD_UP:
            return DIR_UP;
        case KeyCode::KEYCODE_DOWN:
        case KeyCode::KEYCODE_GAMEPAD_DOWN:
        case KeyCode::KEYCODE_GAMEPAD_DPAD_DOWN:
            return DIR_DOWN;
        case KeyCode::KEYCODE_LEFT:
        case KeyCode::KEYCODE_GAMEPAD_LEFT:
        case KeyCode::KEYCODE_GAMEPAD_DPAD_LEFT:
            return DIR_LEFT;
        case KeyCode::KEYCODE_RIGHT:
        case KeyCode::KEYCODE_GAMEPAD_RIGHT:
        case KeyCode::KEYCODE_GAMEPAD_DPAD_RIGHT:
            return DIR_RIGHT;
        default:
            return 0;
    }
}

int Width(Font *font, const std::string &text) { // the font's own StringWidth (Font::StringWidth is the base's)
    return font->GetVTable()->StringWidth(font, pvzstl::string(text.c_str()));
}

// text cut to maxWidth with "..." (whole UTF-8 characters).
std::string Fit(Font *font, std::string text, int maxWidth) {
    if (Width(font, text) <= maxWidth) {
        return text;
    }
    while (!text.empty() && Width(font, text + "...") > maxWidth) {
        while (text.size() > 1 && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80) {
            text.pop_back();
        }
        text.pop_back();
    }
    return text + "...";
}

void DrawText(Graphics *g, Font *font, const std::string &text, int x, int y, const Color &color) {
    g->SetFont(font);
    g->SetColor(color);
    g->DrawString(pvzstl::string(text.c_str()), x, y);
}

// The selected thing glows, brighter and dimmer in turn (as the Zombatar
// screen's): lit from within, with a soft halo.
void DrawGlow(Graphics *g, const Rect &r, int theTick) {
    constexpr int kBreath = 130; // updates (100 a second) a glow takes
    const float pulse = 0.5f - 0.5f * std::cos(float(theTick % kBreath) * 6.2831853f / kBreath);
    g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
    for (int k = 1; k <= 5; ++k) {
        g->SetColor(Color(255, 226, 120, int((16.0f + 28.0f * pulse) * float(6 - k) / 5.0f)));
        g->DrawRect(Rect(r.mX - k, r.mY - k, r.mWidth + 2 * k - 1, r.mHeight + 2 * k - 1));
    }
    g->SetColor(Color(255, 236, 170, int(22.0f + 40.0f * pulse)));
    g->FillRect(r);
    g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// The section buttons: two columns of the game's stone buttons (as Help &
// Options' own), row by row.
Rect SectionRect(int k) {
    const int w = (L.listWidth + 16 - kSectionGapX) / 2;
    const int h = IMAGE_BUTTON_LEFT->mHeight;
    return Rect(L.listX - 8 + (k % 2) * (w + kSectionGapX), L.listY + (k / 2) * (h + kSectionGapY), w, h);
}
} // namespace

void ModMenuDialog::Show(LawnApp *theApp) {
    if (theApp == nullptr || theApp->GetDialog(static_cast<Dialogs>(DIALOG_ID)) != nullptr) {
        return;
    }
    ModMenuDialog *dialog = new ModMenuDialog(theApp);
    theApp->AddDialog(static_cast<Dialogs>(DIALOG_ID), dialog);
    theApp->mWidgetManager->SetFocus(dialog);
}

void ModMenuDialog::Remember(int theFeat, int theValue, bool theBoolean) {
    BuildRows();
    for (const MenuRow &row : gRows) {
        if (row.feat == theFeat && (row.kind == RowKind::Toggle || row.kind == RowKind::Spinner || row.kind == RowKind::Value)) {
            gValues[theFeat] = row.kind == RowKind::Toggle ? (theBoolean ? 1 : 0) : theValue;
            return;
        }
    }
}

void ModMenuDialog::LoadSettings() {
    BuildRows();
    const std::string path = SettingsPath();
    FILE *file = path.empty() ? nullptr : std::fopen(path.c_str(), "r");
    if (file == nullptr) {
        return;
    }
    char line[64];
    while (std::fgets(line, sizeof(line), file) != nullptr) {
        int feat, value;
        if (line[0] == '#' || std::sscanf(line, "%d=%d", &feat, &value) != 2 || feat == 5) {
            continue;
        }
        for (const MenuRow &row : gRows) {
            if (row.feat != feat || (row.kind != RowKind::Toggle && row.kind != RowKind::Spinner && row.kind != RowKind::Value)) {
                continue;
            }
            if (row.kind == RowKind::Spinner) {
                value = std::clamp(value, 0, static_cast<int>(row.options.size()) - 1);
            } else if (row.kind == RowKind::Value) {
                value = std::clamp(value, 0, kValueMax);
            }
            Apply(row, value);
            break;
        }
    }
    std::fclose(file);
    gDirty = false;
}

ModMenuDialog::ModMenuDialog(LawnApp *theApp) {
    LawnDialog::_constructor(theApp, nullptr, DIALOG_ID, true, "Mod Menu", "", "", 0);
    mDrawStandardBack = true;

    // LawnDialog's table is 133 entries (Widget's 122, then Dialog's).
    static void *sVTable[133];
    static std::once_flag sVTableInit;
    std::call_once(sVTableInit, [this] {
        std::memcpy(sVTable, this->Sexy::Widget::vTable, sizeof(sVTable));
        gBaseUpdate = reinterpret_cast<UpdateFunc>(sVTable[31]);
        gBaseDraw = reinterpret_cast<DrawFunc>(sVTable[36]);
        sVTable[0] = (void *)homura::ExtractMemFuncPtr(&ModMenuDialog::_destructor);
        sVTable[1] = (void *)homura::ExtractMemFuncPtr(&ModMenuDialog::_destructor2);
        sVTable[31] = (void *)homura::ExtractMemFuncPtr(&ModMenuDialog::Update);
        sVTable[36] = (void *)homura::ExtractMemFuncPtr(&ModMenuDialog::Draw);
        sVTable[71] = (void *)homura::ExtractMemFuncPtr(&ModMenuDialog::KeyDown);
        sVTable[72] = (void *)homura::ExtractMemFuncPtr(&ModMenuDialog::KeyUp);
        sVTable[77] = (void *)homura::ExtractMemFuncPtr(&ModMenuDialog::MouseDown);
        sVTable[80] = (void *)homura::ExtractMemFuncPtr(&ModMenuDialog::MouseUp);
        sVTable[81] = (void *)homura::ExtractMemFuncPtr(&ModMenuDialog::MouseDrag);
    });
    this->Sexy::Widget::vTable = sVTable;

    // The frame is drawn in whole tiles (LawnDialog::Draw): the size is made of
    // them, and the rows go inside its dark middle. The 1.1.5 tiles: sides
    // 107 / 93 / 120 wide, the top 97 tall (the dark starts 34 down), the
    // bottom 112 (its stone ledge 107), a middle row 54; the dark runs from 37
    // into the left tile to 46 into the right one.
    const int leftW = IMAGE_DIALOG_TOPLEFT->mWidth, midW = IMAGE_DIALOG_TOPMIDDLE->mWidth, rightW = IMAGE_DIALOG_TOPRIGHT->mWidth;
    const int topH = IMAGE_DIALOG_TOPLEFT->mHeight, midH = IMAGE_DIALOG_CENTERLEFT->mHeight, bottomH = IMAGE_DIALOG_BOTTOMLEFT->mHeight;
    constexpr int kHeaderH = 45; // LawnDialog's header strip above the frame
    L.width = leftW + rightW + std::max(1, (800 - leftW - rightW) / midW) * midW;
    L.height = kHeaderH + topH + bottomH + std::max(1, (650 - kHeaderH - topH - bottomH) / midH) * midH;
    const int innerLeft = leftW * 37 / 107, innerRight = L.width - rightW * 74 / 120;
    const int innerTop = kHeaderH + topH * 34 / 97, innerBottom = L.height - bottomH * 107 / 114;
    L.listX = innerLeft + 22;
    L.listWidth = innerRight - 30 - L.listX; // room at the right for the scroll marks
    L.listY = innerTop + 46;                 // under the title
    L.footerY = innerBottom - 10;
    L.centerX = (innerLeft + innerRight) / 2;
    L.rows = std::max(4, (L.footerY - 30 - L.listY) / kRowHeight);

    const Rect &screen = LawnApp::FULLSCREEN_RECT;
    LawnDialog::Resize(screen.mX + (screen.mWidth - L.width) / 2, screen.mY + (screen.mHeight - L.height) / 2, L.width, L.height);

    BuildRows();
    gOpen = -1; // it opens on the sections
    gSectionAt = std::clamp(gSectionAt, 0, std::max(0, static_cast<int>(gSections.size()) - 1));
}

ModMenuDialog::~ModMenuDialog() {
    _destructor();
}

void ModMenuDialog::_destructor() {
    LawnDialog::_destructor();
}

void ModMenuDialog::_destructor2() {
    delete this;
}

void ModMenuDialog::OpenSection(int theAt) {
    if (gSections.empty()) {
        return;
    }
    gSectionAt = (theAt + static_cast<int>(gSections.size())) % static_cast<int>(gSections.size());
    gOpen = gSections[gSectionAt];
    *mDialogHeader = pvzstl::string(gRows[gOpen].label.c_str());
    mTop = 0;
    mFlashRow = -1;
    gCursor = gOpen;
    Step(DIR_DOWN, false); // its first row
    mTop = 0;
}

void ModMenuDialog::CloseSection() {
    gOpen = -1;
    *mDialogHeader = pvzstl::string("Mod Menu");
    mFlashRow = -1;
}

void ModMenuDialog::MoveTo(int theRow) {
    gCursor = theRow;
    const std::vector<int> rows = VisibleRows();
    const int at = static_cast<int>(std::find(rows.begin(), rows.end(), theRow) - rows.begin());
    if (at < mTop) {
        mTop = at;
    } else if (at >= mTop + L.rows) {
        mTop = at - L.rows + 1;
    }
    if (at == mTop && at > 0 && gRows[rows[at - 1]].kind == RowKind::Caption) {
        mTop = at - 1; // keep the caption over the rows it names
    }
    mTop = std::clamp(mTop, 0, std::max(0, static_cast<int>(rows.size()) - L.rows));
}

void ModMenuDialog::Step(int theDir, bool theRepeat) {
    if (gOpen < 0) { // the section buttons: two to a row
        const int n = static_cast<int>(gSections.size());
        if (n == 0) {
            return;
        }
        int at = gSectionAt;
        switch (theDir) {
            case DIR_LEFT:
                at = at % 2 == 1 ? at - 1 : at;
                break;
            case DIR_RIGHT:
                at = at % 2 == 0 && at + 1 < n ? at + 1 : at;
                break;
            case DIR_UP:
                at = at >= 2 ? at - 2 : (theRepeat ? at : ((n - 1) / 2) * 2 + at % 2);
                break;
            case DIR_DOWN:
                at = at + 2 < n ? at + 2 : (theRepeat ? at : at % 2);
                break;
            default:
                break;
        }
        gSectionAt = std::clamp(at, 0, n - 1);
        return;
    }
    if (theDir == DIR_LEFT || theDir == DIR_RIGHT) {
        Change(gCursor, theDir == DIR_RIGHT ? 1 : -1, theRepeat);
        return;
    }
    const int n = static_cast<int>(gRows.size());
    const int delta = theDir == DIR_DOWN ? 1 : -1;
    for (int i = gCursor + delta; i >= 0 && i < n; i += delta) {
        if (IsSelectable(i)) {
            MoveTo(i);
            return;
        }
    }
    if (theRepeat) {
        return; // a held key stops at the end
    }
    for (int i = delta > 0 ? 0 : n - 1; i >= 0 && i < n; i += delta) {
        if (IsSelectable(i)) {
            MoveTo(i);
            return;
        }
    }
}

void ModMenuDialog::Change(int theRow, int theDelta, bool theRepeat) {
    const MenuRow &row = gRows[theRow];
    switch (row.kind) {
        case RowKind::Toggle:
            if ((ValueOf(row) != 0) != (theDelta > 0)) {
                Apply(row, theDelta > 0 ? 1 : 0);
            }
            break;
        case RowKind::Spinner: {
            const int n = static_cast<int>(row.options.size());
            int value = ValueOf(row) + theDelta;
            value = theRepeat ? std::clamp(value, 0, n - 1) : (value + n) % n;
            if (value != ValueOf(row)) {
                Apply(row, value);
            }
            break;
        }
        case RowKind::Value: {
            const int step = theRepeat && mHeldTicks > kFastAfter ? 10 : 1;
            const int value = std::clamp(ValueOf(row) + theDelta * step, 0, kValueMax);
            if (value != ValueOf(row)) {
                Apply(row, value);
            }
            break;
        }
        default:
            break;
    }
}

void ModMenuDialog::Activate(int theRow) {
    const MenuRow &row = gRows[theRow];
    switch (row.kind) {
        case RowKind::Toggle:
            Apply(row, ValueOf(row) != 0 ? 0 : 1);
            break;
        case RowKind::Spinner:
        case RowKind::Value:
            Change(theRow, 1, false);
            break;
        case RowKind::Once:
            if (NeedsLevel(row) && !InLevel(mApp)) {
                break;
            }
            if (row.feat == 41 || row.feat == 81) {
                // they open the game's cheat dialogs: once the menus are gone
                Close(true);
                Apply(row, 1);
                break;
            }
            Apply(row, 1);
            mFlashRow = theRow;
            mFlashTicks = kFlashTicks;
            break;
        default:
            break;
    }
}

void ModMenuDialog::Close(bool theAlsoHelpOptions) {
    if (mClosing) {
        return;
    }
    mClosing = true;
    SaveSettings();
    LawnApp *app = mApp;
    app->KillDialog(static_cast<Dialogs>(DIALOG_ID)); // deleted later (SafeDeleteWidget)
    // The game hands the focus back to the board only: Help & Options, under
    // this, takes it again here.
    if (theAlsoHelpOptions) {
        LeaveMenus(app);
    } else if (Dialog *help = app->GetDialog(Dialogs::DIALOG_HELPOPTIONS)) {
        app->mWidgetManager->SetFocus(help);
    }
}

void ModMenuDialog::LeaveMenus(LawnApp *theApp) {
    if (Dialog *help = theApp->GetDialog(Dialogs::DIALOG_HELPOPTIONS)) {
        old_HelpOptionsDialog_ButtonDepress(static_cast<HelpOptionsDialog *>(help), 4); // its Back
    }
    if (theApp->GetDialog(Dialogs::DIALOG_NEWOPTIONS) != nullptr) {
        theApp->KillNewOptionsDialog(); // the pause menu (xbox_pause_menu) Help came from
    }
}

void ModMenuDialog::Update() {
    gBaseUpdate(this);
    ++mTick;
    if (mFlashTicks > 0 && --mFlashTicks == 0) {
        mFlashRow = -1;
    }
    if (mHeldDir != 0 && !mClosing) {
        ++mHeldTicks;
        if (mHeldTicks >= kRepeatDelay && (mHeldTicks - kRepeatDelay) % kRepeatEvery == 0) {
            Step(mHeldDir, true);
        }
    }
    MarkDirty();
}

bool ModMenuDialog::KeyDown(KeyCode theKey) {
    if (mClosing) {
        return true;
    }
    switch (theKey) {
        case KeyCode::KEYCODE_ESCAPE:
        case KeyCode::KEYCODE_GAMEPAD_B:
            if (gOpen >= 0) {
                CloseSection(); // back to the sections
            } else {
                Close(false);
            }
            return true;
        case KeyCode::KEYCODE_RETURN:
        case KeyCode::KEYCODE_SPACE:
        case KeyCode::KEYCODE_GAMEPAD_A:
            if (gOpen < 0) {
                OpenSection(gSectionAt);
            } else {
                Activate(gCursor);
            }
            return true;
        case KeyCode::KEYCODE_GAMEPAD_TL:
        case KeyCode::KEYCODE_GAMEPAD_TR: // the previous / next section
            if (gOpen >= 0) {
                OpenSection(gSectionAt + (theKey == KeyCode::KEYCODE_GAMEPAD_TR ? 1 : -1));
            }
            return true;
        default:
            break;
    }
    const int dir = DirOf(theKey);
    if (dir != 0) {
        mHeldDir = dir;
        mHeldTicks = 0;
        Step(dir, false);
    }
    return true;
}

bool ModMenuDialog::KeyUp(KeyCode theKey) {
    if (DirOf(theKey) == mHeldDir) {
        mHeldDir = 0;
    }
    return true;
}

int ModMenuDialog::RowAt(int x, int y) const {
    if (x < L.listX - 8 || x >= L.listX + L.listWidth + 8 || y < L.listY || y >= L.listY + L.rows * kRowHeight) {
        return -1;
    }
    const std::vector<int> rows = VisibleRows();
    const int at = mTop + (y - L.listY) / kRowHeight;
    return at < static_cast<int>(rows.size()) ? rows[at] : -1;
}

void ModMenuDialog::MouseDown(int x, int y, int theBtnNum, int theClickCount) {
    (void)theBtnNum;
    (void)theClickCount;
    mTouching = true;
    mDragged = false;
    mDownX = x;
    mDownY = y;
    mDragY = y;
}

void ModMenuDialog::MouseDrag(int x, int y) {
    if (!mTouching) {
        return;
    }
    if (std::abs(y - mDownY) > 12 || std::abs(x - mDownX) > 12) {
        mDragged = true;
    }
    if (gOpen < 0) {
        return;
    }
    const int rows = static_cast<int>(VisibleRows().size());
    while (y - mDragY >= kRowHeight && mTop > 0) { // the finger pulls the list down
        --mTop;
        mDragY += kRowHeight;
    }
    while (mDragY - y >= kRowHeight && mTop < rows - L.rows) {
        ++mTop;
        mDragY -= kRowHeight;
    }
}

void ModMenuDialog::MouseUp(int x, int y, int theBtnNum, int theClickCount) {
    (void)theBtnNum;
    (void)theClickCount;
    if (!mTouching || mClosing) {
        return;
    }
    mTouching = false;
    if (mDragged) {
        return;
    }
    if (y >= L.footerY - 24 && y < L.footerY + 12) { // the footer: "B: Back"
        if (gOpen >= 0) {
            CloseSection();
        } else {
            Close(false);
        }
        return;
    }
    if (gOpen < 0) {
        for (int k = 0; k < static_cast<int>(gSections.size()); ++k) {
            if (SectionRect(k).Contains(x, y)) {
                OpenSection(k);
                return;
            }
        }
        return;
    }
    const int row = RowAt(x, y);
    if (row < 0 || !IsSelectable(row)) {
        return;
    }
    MoveTo(row);
    const RowKind kind = gRows[row].kind;
    if ((kind == RowKind::Spinner || kind == RowKind::Value) && x < L.listX + L.listWidth / 2) {
        return; // the label: selects; the arrows at the right change it
    }
    if ((kind == RowKind::Spinner || kind == RowKind::Value) && x < L.listX + L.listWidth - 110) {
        Change(row, -1, false); // the left half of the value
        return;
    }
    Activate(row);
}

void ModMenuDialog::Draw(Graphics *g) {
    gBaseDraw(this, g);

    Font *font = FONT_DWARVENTODCRAFT24;
    Font *small = FONT_DWARVENTODCRAFT18;
    const Color text(236, 240, 228);
    const Color highlight(255, 255, 153);
    const Color on(130, 240, 110);
    const Color off(175, 175, 175);
    const Color hint(214, 214, 204);

    if (gOpen < 0) { // the sections
        const auto drawStone = reinterpret_cast<StoneButtonFunc>(DrawStoneButtonAddr);
        for (int k = 0; k < static_cast<int>(gSections.size()); ++k) {
            const Rect r = SectionRect(k);
            const bool selected = k == gSectionAt;
            if (drawStone != nullptr) {
                drawStone(g, r.mX, r.mY, r.mWidth, r.mHeight, false, selected, pvzstl::string(gRows[gSections[k]].label.c_str()), false);
            } else {
                DrawText(g, font, gRows[gSections[k]].label, r.mX + 12, r.mY + r.mHeight - 14, selected ? highlight : text);
            }
            if (selected) {
                DrawGlow(g, Rect(r.mX + 6, r.mY + 4, r.mWidth - 12, r.mHeight - 10), mTick);
            }
        }
        const std::string footer = "A: Open        B: Close";
        DrawText(g, small, footer, L.centerX - Width(small, footer) / 2, L.footerY, hint);
        return;
    }

    const std::vector<int> rows = VisibleRows();
    mTop = std::clamp(mTop, 0, std::max(0, static_cast<int>(rows.size()) - L.rows));
    for (int k = 0; k < L.rows && mTop + k < static_cast<int>(rows.size()); ++k) {
        const int i = rows[mTop + k];
        const MenuRow &row = gRows[i];
        const int y = L.listY + k * kRowHeight;
        const int baseline = y + kRowHeight - 11;
        const bool selected = i == gCursor;
        const Rect band(L.listX - 8, y + 1, L.listWidth + 16, kRowHeight - 2);
        if ((mTop + k) % 2 == 1) { // every other row a shade darker, to follow across
            g->SetColor(Color(0, 0, 0, 46));
            g->FillRect(band);
        }
        if (selected) {
            DrawGlow(g, band, mTick);
        }
        if (row.kind == RowKind::Caption) {
            DrawText(g, small, Fit(small, row.label, L.listWidth - 16), L.listX + 4, baseline, row.color);
            continue;
        }

        std::string value;
        Color valueColor = selected ? highlight : text;
        switch (row.kind) {
            case RowKind::Toggle:
                value = ValueOf(row) != 0 ? "On" : "Off";
                valueColor = ValueOf(row) != 0 ? on : off;
                break;
            case RowKind::Spinner:
                value = "<  " + row.options[std::clamp(ValueOf(row), 0, static_cast<int>(row.options.size()) - 1)] + "  >";
                break;
            case RowKind::Value:
                value = "<  " + std::to_string(ValueOf(row)) + "  >";
                break;
            case RowKind::Once:
                if (i == mFlashRow) {
                    value = "Done!";
                    valueColor = on;
                } else if (NeedsLevel(row) && !InLevel(mApp)) {
                    value = "(in a level)";
                    valueColor = off;
                } else if (selected) {
                    value = "A: Go";
                }
                break;
            default:
                break;
        }
        const int labelX = L.listX + 8;
        const int right = L.listX + L.listWidth;
        const int labelWidth = Width(font, row.label);
        int valueRoom = right - labelX - labelWidth - 24;
        if (valueRoom < 200 && !value.empty()) {
            valueRoom = std::max(200, (right - labelX) / 2);
        }
        value = Fit(font, value, valueRoom);
        const int valueWidth = value.empty() ? 0 : Width(font, value);
        DrawText(g, font, Fit(font, row.label, right - labelX - valueWidth - 24), labelX, baseline, selected ? highlight : text);
        if (!value.empty()) {
            DrawText(g, font, value, right - valueWidth, baseline, valueColor);
        }
    }

    const int total = static_cast<int>(rows.size());
    if (total > L.rows) { // a scroll bar: where the rows on show are in the section
        const Rect track(L.listX + L.listWidth + 14, L.listY + 4, 8, L.rows * kRowHeight - 8);
        const int thumbH = std::max(24, track.mHeight * L.rows / total);
        const int thumbY = track.mY + (track.mHeight - thumbH) * mTop / std::max(1, total - L.rows);
        g->SetColor(Color(0, 0, 0, 90));
        g->FillRect(track);
        g->SetColor(Color(255, 236, 150, 210));
        g->FillRect(Rect(track.mX + 1, thumbY, track.mWidth - 2, thumbH));
        g->SetColor(Color(255, 250, 215, 240));
        g->FillRect(Rect(track.mX + 2, thumbY + 2, 2, thumbH - 4)); // a glint down its side
    }
    const std::string footer = "A: Choose    Left/Right: Change    L/R: Sections    B: Back";
    DrawText(g, small, Fit(small, footer, L.listWidth + 16), L.centerX - std::min(Width(small, footer), L.listWidth + 16) / 2, L.footerY, hint);
}
