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

#ifndef PVZ_LAWN_WIDGET_SCROLL_WIDGET_H
#define PVZ_LAWN_WIDGET_SCROLL_WIDGET_H

#include "PvZ/SexyAppFramework/Misc/Point.h"
#include "PvZ/SexyAppFramework/Widget/Widget.h"

#include <vector>

namespace Sexy {

// class PageControl : public Widget {
// protected:
//     Image* mPartsImage;
//     int mNumberOfPages;
//     int mCurrentPage;

// public:
//     PageControl(Image* partsImage);
//     ~PageControl();
//     void SetNumberOfPages(int count);
//     void SetCurrentPage(int page);
//     int GetCurrentPage();
//     void Draw(Graphics* g); // Is a new function in WP, not an overridden virtual function.
// }

class ScrollWidget : public Widget {
public:
    enum ScrollMode {
        SCROLL_DISABLED,
        SCROLL_HORIZONTAL,
        SCROLL_VERTICAL,
        SCROLL_BOTH,
    };

    enum Colors {
        COLOR_BACKGROUND,
    };

    class Overlay {
    public:
        Image *image;
        FPoint offset;
    };

    static constexpr float SCROLL_TARGET_THRESHOLD_NORM = 0.01f;
    static constexpr float SCROLL_VELOCITY_THRESHOLD_NORM = 0.0001f;
    static constexpr float SCROLL_DEVIATION_DAMPING = 0.5f;
    static constexpr float SCROLL_SPRINGBACK_TENSION = 0.1f;
    static constexpr float SCROLL_VELOCITY_FILTER_WINDOW = 0.1f;
    static constexpr float SCROLL_VELOCITY_DAMPING = 0.975f;
    static constexpr float SCROLL_VELOCITY_DEVIATION_DAMPING = 0.85f;
    static constexpr float SCROLL_DRAG_THRESHOLD = 4.0f;
    static constexpr float SCROLL_PAGE_FLICK_THRESHOLD = 40.0f;
    static constexpr int SCROLL_TAP_DELAY_TICKS = 10;
    static constexpr int SCROLL_INDICATORS_FLASH_TICKS = 100;
    static constexpr float SCROLL_INDICATORS_FADE_IN_RATE = 0.05f;
    static constexpr float SCROLL_INDICATORS_FADE_OUT_RATE = 0.02f;

    bool clientAllowsScroll;
    bool mClientDownDispatched;
    int mDeferredMouseDownMagicCode;
    Widget *mClient;
    Widget *mClientLastDown;
    Image *mIndicatorsImage;
    Image *mBackgroundImage;
    bool mFillBackground;
    std::vector<Overlay> mOverlays;
    bool mDrawOverlays;
    ScrollMode mScrollMode;
    FPoint mScrollTouchReference;
    Insets mScrollInsets;
    FPoint mScrollTarget;
    FPoint mScrollOffset;
    FPoint mScrollOffsetReference;
    FPoint mScrollVelocity;
    bool mBounceEnabled;
    bool mIndicatorsEnabled;
    Insets mIndicatorsInsets;
    int mIndicatorsFlashTimer;
    float mIndicatorsOpacity;
    int mCurrentPageHorizontal;
    int mCurrentPageVertical;
    bool mSeekScrollTarget;
    bool mScrollTracking;
    double mScrollLastTimestamp;
    float mSpringOverride;
    FPoint mScrollMin;
    FPoint mScrollMax;
    FPoint mPageSize;
    ScrollMode mScrollPractical;
    int mPageCountHorizontal;
    int mPageCountVertical;

    ScrollWidget();
    /* virtual */ ~ScrollWidget();

    void Init();
    /* virtual */ void Draw(Graphics *g);
    /* virtual */ void Update();
    void SetScrollMode(ScrollMode mode);
    void SetScrollInsets(const Insets &insets);
    void SetScrollOffset(FPoint offset, bool animated);
    void ScrollToMin(bool animated);
    void ScrollToBottom(bool animated);
    void ScrollToPoint(FPoint point, bool animated);
    void ScrollRectIntoView(const Rect &rect, bool animated);
    void EnableBounce(bool enable);
    void EnablePaging(bool enable);
    void EnableIndicators(Image *indicatorsImage);
    void SetIndicatorsInsets(const Insets &insets);
    void FlashIndicators();
    void SetPageHorizontal(int page, bool animated);
    void SetPageVertical(int page, bool animated);
    void SetPage(int hpage, int vpage, bool animated);
    int GetPageHorizontal() const;
    int GetPageVertical() const;
    void SetBackgroundImage(Image *image);
    void EnableBackgroundFill(bool enable);
    void AddOverlayImage(Image *image, FPoint offset);
    void EnableOverlays(bool enable);
    /* virtual */ void AddWidget(Widget *theWidget);
    /* virtual */ void RemoveWidget(Widget *theWidget);
    /* virtual */ void Resize(int x, int y, int width, int height);
    void ClientSizeChanged();
    /* virtual */ void MouseDown(int x, int y, int theMagicCode);
    /* virtual */ void MouseUp(int x, int y, int theMagicCode);
    /* virtual */ void MouseDrag(int x, int y);
    // /* virtual */ void MouseWheel(int theDelta);

    void TouchBegan(Touch touch, int theMagicCode);
    void TouchMoved(Touch touch);
    void TouchEnded(Touch touch, int theMagicCode);

    void TouchMotion(Touch touch);
    void DoScrollUpdate();
    void DrawHorizontalStretchableImage(Graphics *g, Image *image, const Rect &destRect);
    void DrawVerticalStretchableImage(Graphics *g, Image *image, const Rect &destRect);
    // void DrawProxyWidget(Graphics* g, ProxyWidget* proxyWidget);
    void SnapToPage();
    Widget *GetClientWidgetAt(Touch touch);
    FPoint &GetScrollOffset();
    void SetScrollOffset(float x, float y);
    void CacheDerivedValues();

protected:
    void _destructor();
    void _destructor2();
};

} // namespace Sexy

#endif // PVZ_LAWN_WIDGET_SCROLL_WIDGET_H
