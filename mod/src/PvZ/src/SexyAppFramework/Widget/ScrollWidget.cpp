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

#include "PvZ/SexyAppFramework/Widget/ScrollWidget.h"
#include "Homura/MemberUtils.h"
#include "PvZ/SexyAppFramework/Widget/WidgetManager.h"

#include <cmath>
#include <cstring>

#include <algorithm>
#include <mutex>

Sexy::ScrollWidget::ScrollWidget() {
    Widget::_constructor();

    static void *sScrollWidget_vTable[122];
    static std::once_flag vtableInitFlag;
    std::call_once(vtableInitFlag, [this] {
        std::memcpy(sScrollWidget_vTable, vTable, sizeof(sScrollWidget_vTable));

        sScrollWidget_vTable[0] = (void *)homura::ExtractMemFuncPtr(&ScrollWidget::_destructor);
        sScrollWidget_vTable[1] = (void *)homura::ExtractMemFuncPtr(&ScrollWidget::_destructor2);
        sScrollWidget_vTable[31] = (void *)homura::ExtractMemFuncPtr(&ScrollWidget::Update);
        sScrollWidget_vTable[36] = (void *)homura::ExtractMemFuncPtr(&ScrollWidget::Draw);
        sScrollWidget_vTable[6] = (void *)homura::ExtractMemFuncPtr(&ScrollWidget::AddWidget);
        sScrollWidget_vTable[7] = (void *)homura::ExtractMemFuncPtr(&ScrollWidget::RemoveWidget);
        sScrollWidget_vTable[50] = (void *)homura::ExtractMemFuncPtr(&ScrollWidget::Resize);
        sScrollWidget_vTable[76] = (void *)homura::ExtractMemFuncPtr(&ScrollWidget::MouseDown);
        sScrollWidget_vTable[79] = (void *)homura::ExtractMemFuncPtr(&ScrollWidget::MouseUp);
        sScrollWidget_vTable[81] = (void *)homura::ExtractMemFuncPtr(&ScrollWidget::MouseDrag);
    });
    vTable = sScrollWidget_vTable;

    Init();
}

Sexy::ScrollWidget::~ScrollWidget() {
    // 不调用自身的 _destructor, 否则会重复析构子对象
    Widget::_destructor();
}

void Sexy::ScrollWidget::_destructor() {
    mOverlays.~vector();
    Widget::_destructor();
}

void Sexy::ScrollWidget::_destructor2() {
    delete this;
}

void Sexy::ScrollWidget::Init() {
    mClient = nullptr;
    mClientDownDispatched = false;
    mDeferredMouseDownMagicCode = 0;
    mClientLastDown = nullptr;
    mIndicatorsImage = nullptr;
    mScrollMode = ScrollMode::SCROLL_VERTICAL;
    mScrollInsets = Insets(0, 0, 0, 0);
    mScrollTracking = false;
    mSeekScrollTarget = false;
    mBounceEnabled = true;
    mIndicatorsEnabled = false;
    mIndicatorsInsets = Insets(0, 0, 0, 0);
    mIndicatorsFlashTimer = 0;
    mIndicatorsOpacity = 0.0f;
    mBackgroundImage = nullptr;
    mFillBackground = false;
    mDrawOverlays = false;
    mScrollOffset = Sexy::FPoint(0.0f, 0.0f);
    mScrollVelocity = Sexy::FPoint(0.0f, 0.0f);
    mClip = true;
}

void Sexy::ScrollWidget::SetScrollMode(ScrollMode mode) {
    mScrollMode = mode;
    CacheDerivedValues();
}

void Sexy::ScrollWidget::SetScrollInsets(const Insets &insets) {
    mScrollInsets = insets;
    CacheDerivedValues();
}

void Sexy::ScrollWidget::SetScrollOffset(Sexy::FPoint offset, bool animated) {
    if (animated) {
        mScrollTarget = offset;
        mSeekScrollTarget = true;
        return;
    }
    mScrollOffset = offset;
    mScrollVelocity = Sexy::FPoint(0.0f, 0.0f);
    if (mClient != nullptr) {
        mClient->Move((int)mScrollOffset.mX, (int)mScrollOffset.mY);
    }
}

void Sexy::ScrollWidget::ScrollToMin(bool animated) {
    SetScrollOffset(Sexy::FPoint(mScrollInsets.mLeft, mScrollInsets.mTop), animated);
}

void Sexy::ScrollWidget::ScrollToBottom(bool animated) {
    SetScrollOffset(Sexy::FPoint(mScrollMin.mX, mScrollMin.mY), animated);
}

void Sexy::ScrollWidget::ScrollToPoint(Sexy::FPoint point, bool animated) {
    if (!mIsDown) {
        SetScrollOffset(Sexy::FPoint(-point.mX, -point.mY), animated);
    }
}

void Sexy::ScrollWidget::ScrollRectIntoView(const Rect &rect, bool animated) {
    if (!mIsDown) {
        float num = rect.mX + rect.mWidth;
        float num2 = rect.mY + rect.mHeight;
        float num3 = std::max(std::min(0.0f, mScrollMin.mX), (float)-rect.mX);
        float num4 = std::max(std::min(0.0f, mScrollMin.mY), (float)-rect.mY);
        float num5 = std::min(mScrollMax.mX, (float)mWidth - num);
        float num6 = std::min(mScrollMax.mY, (float)mHeight - num2);
        SetScrollOffset(Sexy::FPoint(std::min(num5, std::max(num3, mScrollOffset.mX)), std::min(num6, std::max(num4, mScrollOffset.mY))), animated);
    }
}

void Sexy::ScrollWidget::EnableBounce(bool enable) {
    mBounceEnabled = enable;
}

void Sexy::ScrollWidget::EnablePaging(bool enable) {
    // mPagingEnabled = enable;
}

void Sexy::ScrollWidget::EnableIndicators(Image *indicatorsImage) {
    // mIndicatorsImage = indicatorsImage;
    // mIndicatorsEnabled = (nullptr != indicatorsImage);
    // if (mIndicatorsEnabled)
    // {
    //     mIndicatorsProxy = new ProxyWidget(this);
    //     mIndicatorsProxy.mMouseVisible = false;
    //     mIndicatorsProxy.mZOrder = int.MaxValue;
    //     mIndicatorsProxy.Resize(0, 0, mWidth, mHeight);
    //     base.AddWidget(mIndicatorsProxy);
    //     return;
    // }
    // if (!mIndicatorsEnabled && mIndicatorsProxy != nullptr)
    // {
    //     WidgetContainer::RemoveWidget(mIndicatorsProxy);
    //     mIndicatorsProxy.Dispose();
    //     mIndicatorsProxy = nullptr;
    // }
}

void Sexy::ScrollWidget::SetIndicatorsInsets(const Insets &insets) {
    mIndicatorsInsets = insets;
}

void Sexy::ScrollWidget::FlashIndicators() {
    mIndicatorsFlashTimer = SCROLL_INDICATORS_FLASH_TICKS;
}

void Sexy::ScrollWidget::SetPageHorizontal(int page, bool animated) {
    SetPage(page, mCurrentPageVertical, animated);
}

void Sexy::ScrollWidget::SetPageVertical(int page, bool animated) {
    SetPage(mCurrentPageHorizontal, page, animated);
}

void Sexy::ScrollWidget::SetPage(int hpage, int vpage, bool animated) {
    // if (mPagingEnabled)
    // {
    //     mCurrentPageHorizontal = std::max(0, std::min(hpage, mPageCountHorizontal - 1));
    //     mCurrentPageVertical = std::max(0, std::min(vpage, mPageCountVertical - 1));
    //     SetScrollOffset(Sexy::FPoint
    //     (
    //         mScrollInsets.mLeft - mCurrentPageHorizontal * mPageSize.mX,
    //         mScrollInsets.mTop - mCurrentPageVertical * mPageSize.mY
    //     ), animated);
    // }
}

int Sexy::ScrollWidget::GetPageHorizontal() const {
    return mCurrentPageHorizontal;
}

int Sexy::ScrollWidget::GetPageVertical() const {
    return mCurrentPageVertical;
}

void Sexy::ScrollWidget::SetBackgroundImage(Image *image) {
    mBackgroundImage = image;
}

void Sexy::ScrollWidget::EnableBackgroundFill(bool enable) {
    mFillBackground = enable;
}

void Sexy::ScrollWidget::AddOverlayImage(Image *image, Sexy::FPoint offset) {
    mDrawOverlays = true;
    for (auto &overlay : mOverlays) {
        if (overlay.image == image) {
            overlay.offset = offset;
            return;
        }
    }
    Overlay overlay2{
        .image = image,
        .offset = offset,
    };
    mOverlays.push_back(overlay2);
}

void Sexy::ScrollWidget::EnableOverlays(bool enable) {
    mDrawOverlays = enable;
}

void Sexy::ScrollWidget::AddWidget(Widget *theWidget) {
    if (mClient == nullptr) {
        mClient = theWidget;
        Widget *widget = mClient;
        widget->mWidgetFlagsMod.mRemoveFlags = (widget->mWidgetFlagsMod.mRemoveFlags | 16);
        mClient->Move((int)mScrollOffset.mX, (int)mScrollOffset.mY);
        WidgetContainer::AddWidget(mClient);
        CacheDerivedValues();
    }
}

void Sexy::ScrollWidget::RemoveWidget(Widget *theWidget) {
    if (theWidget == mClient) {
        mClient = nullptr;
    }
    WidgetContainer::RemoveWidget(theWidget);
}

void Sexy::ScrollWidget::Resize(int x, int y, int width, int height) {
    Widget::Resize(x, y, width, height);
    // if (mIndicatorsProxy != nullptr)
    // {
    //     mIndicatorsProxy.Resize(0, 0, width, height);
    // }
    CacheDerivedValues();
}

void Sexy::ScrollWidget::ClientSizeChanged() {
    if (mClient != nullptr) {
        CacheDerivedValues();
    }
}

void Sexy::ScrollWidget::MouseDown(int x, int y, int theMagicCode) {
    Touch touch{
        .location{x, y},
        .timestamp = mUpdateCnt / 120.0,
    };
    TouchBegan(touch, theMagicCode);
    //  if (mClient != nullptr)
    //  {
    //      // clientAllowsScroll = mClient->DoScroll(x, y);
    //      // The DoScroll function here is a virtual function in WP,
    //      //     but since it has only one override, we can simply set it to true.
    //      clientAllowsScroll = true;
    //      if (mSeekScrollTarget)
    //      {
    //          if (mListener != nullptr)
    //          {
    //              mListener->ScrollTargetInterrupted(this);
    //          }
    //      }
    //      mScrollTouchReference = Sexy::FPoint(x, y);
    //      mScrollOffsetReference = Sexy::FPoint(mClient->mX, mClient->mY);
    //      mScrollOffset = mScrollOffsetReference;
    //      //mScrollLastTimestamp = touch.timestamp;
    //      mScrollTracking = false;
    //      mSeekScrollTarget = false;
    //      mClientLastDown = GetClientWidgetAt(x, y);
    //      mClientLastDown->mIsDown = true;
    //      mClientLastDown->mIsOver = true;
    //      homura::CallVirtualFunc<Widget, 76, void, int, int, int>(mClientLastDown, x, y, theMagicCode);// MouseDown
    //  }
}

void Sexy::ScrollWidget::MouseUp(int x, int y, int theMagicCode) {
    Touch touch{
        .location{x, y},
        .timestamp = mUpdateCnt / 120.0,
    };
    TouchEnded(touch, theMagicCode);
    // if (mScrollTracking)
    // {
    //     /*TouchMotion(touch);
    //     mScrollTracking = false;
    //     if (mPagingEnabled)
    //     {
    //         SnapToPage();
    //         return;
    //     }*/
    // }
    // else if (mClientLastDown != nullptr)
    // {
    //     Point b = homura::CallVirtualFunc<Widget, 19, Point>(this)/*GetAbsPos*/ - homura::CallVirtualFunc<Widget, 19, Point>(mClientLastDown)/*GetAbsPos*/;
    //     Point a(x, y);
    //     a += b;
    //     //CGMaths.PointTranslate(ref touch.previousLocation, b.mX, b.mY);
    //     homura::CallVirtualFunc<Widget, 79, void, int, int, int>(mClientLastDown, a.mX, a.mY, theMagicCode);// MouseUp
    //     mClientLastDown->mIsDown = false;
    //     mClientLastDown = nullptr;
    // }
}

void Sexy::ScrollWidget::MouseDrag(int x, int y) {
    Touch touch{
        .location{x, y},
        .timestamp = mUpdateCnt / 120.0,
    };
    TouchMoved(touch);
    //  Sexy::FPoint a(x, y);
    //  Sexy::FPoint point = a - mScrollTouchReference;
    //  if (mClient != nullptr)
    //  {
    //      if (clientAllowsScroll)
    //      {
    //          if (!mScrollTracking
    //              && (mScrollPractical & ScrollMode::SCROLL_HORIZONTAL) != ScrollMode::SCROLL_DISABLED
    //              && std::abs(point.mX) > 4.0f)
    //          {
    //              mScrollTracking = true;
    //          }
    //          if (!mScrollTracking
    //              && (mScrollPractical & ScrollMode::SCROLL_VERTICAL) != ScrollMode::SCROLL_DISABLED
    //              && std::abs(point.mY) > 4.0f)
    //          {
    //              mScrollTracking = true;
    //          }
    //      }
    //      if (mScrollTracking && mClientLastDown != nullptr)
    //      {
    //          mClientLastDown->mIsDown = false;
    //          mClientLastDown = nullptr;
    //      }
    //  }
    //  if (mScrollTracking)
    //  {
    //      // Touch touch = new Touch();
    //      // touch.location.mX = x;
    //      // touch.location.mY = y;
    //      TouchMotion(Sexy::FPoint(x, y), (double)mUpdateCnt / 120.0);
    //      return;
    //  }
    //  if (mClientLastDown != nullptr)
    //  {
    //      Point b = homura::CallVirtualFunc<Widget, 19, Point>(this)/*GetAbsPos*/ - homura::CallVirtualFunc<Widget, 19, Point>(mClientLastDown)/*GetAbsPos*/;
    //      Point a = Point(x, y);
    //      Point point2 = a + b;
    //      Point a2(point2.mX + mClientLastDown->mX, point2.mY + mClientLastDown->mY);
    //      bool flag = (homura::CallVirtualFunc<Widget, 116, Rect>(mClientLastDown)/*GetInsetRect*/).Contains(a2);
    //      if (flag && !mClientLastDown->mIsOver)
    //      {
    //          mClientLastDown->mIsOver = true;
    //          homura::CallVirtualFunc<Widget, 73, void>(mClientLastDown);// MouseEnter
    //      }
    //      else if (!flag && mClientLastDown->mIsOver)
    //      {
    //          homura::CallVirtualFunc<Widget, 74, void>(mClientLastDown);// MouseLeave
    //          mClientLastDown->mIsOver = false;
    //      }
    //      //CGMaths.PointTranslate(ref touch.location, b.mX, b.mY);
    //      //CGMathhomuras.PointTranslate(ref touch.previousLocation, b.mX, b.mY);
    //      homura::CallVirtualFunc<Widget, 81, void, int, int>(mClientLastDown, x + b.mX, y + b.mY);// MouseDrag
    //  }
}


void Sexy::ScrollWidget::TouchBegan(Touch touch, int theMagicCode) {
    if (mClient != nullptr) {
        clientAllowsScroll = true;
        if (mSeekScrollTarget) {
            // if (mListener != nullptr)
            // {
            //     mListener->ScrollTargetInterrupted(this);
            // }
        }
        mScrollTouchReference = Sexy::FPoint(touch.location.mX, touch.location.mY);
        mScrollOffsetReference = Sexy::FPoint(mClient->mX, mClient->mY);
        mScrollOffset = mScrollOffsetReference;
        mScrollLastTimestamp = touch.timestamp;
        mScrollTracking = false;
        mSeekScrollTarget = false;
        mClientDownDispatched = false;
        mDeferredMouseDownMagicCode = theMagicCode;
        mClientLastDown = GetClientWidgetAt(touch);
    }
}

void Sexy::ScrollWidget::TouchMoved(Touch touch) {
    Sexy::FPoint cgpoint = Sexy::FPoint(touch.location.mX, touch.location.mY) - mScrollTouchReference;
    if (mClient != nullptr) {
        if (clientAllowsScroll && !mScrollTracking && (std::abs(cgpoint.mX) > SCROLL_DRAG_THRESHOLD || std::abs(cgpoint.mY) > SCROLL_DRAG_THRESHOLD)) {
            mScrollTracking = true;
            if (mClientLastDown != nullptr) {
                mClientLastDown->mIsDown = false;
                mClientLastDown->mIsOver = false;
            }
        }
    }
    if (mScrollTracking) {
        TouchMotion(touch);
        return;
    }
    if (mClientLastDown != nullptr) {
        if (!mClientDownDispatched) {
            mClientLastDown->mIsDown = true;
            mClientLastDown->mIsOver = true;
            homura::CallVirtualFunc<Widget, 76, void, int, int, int>(mClientLastDown, mScrollTouchReference.mX, mScrollTouchReference.mY, mDeferredMouseDownMagicCode); // MouseDown
            mClientDownDispatched = true;
        }
        Point b = homura::CallVirtualFunc<Widget, 19, Point>(this) /*GetAbsPos*/ - homura::CallVirtualFunc<Widget, 19, Point>(mClientLastDown) /*GetAbsPos*/;
        Point a(touch.location.mX, touch.location.mY);
        Point cgpoint2 = a + b;
        Point a2(cgpoint2.mX + mClientLastDown->mX, cgpoint2.mY + mClientLastDown->mY);
        bool flag = (homura::CallVirtualFunc<Widget, 116, Rect>(mClientLastDown) /*GetInsetRect*/).Contains(a2);
        if (flag && !mClientLastDown->mIsOver) {
            mClientLastDown->mIsOver = true;
            homura::CallVirtualFunc<Widget, 73, void>(mClientLastDown); // MouseEnter
        } else if (!flag && mClientLastDown->mIsOver) {
            homura::CallVirtualFunc<Widget, 74, void>(mClientLastDown); // MouseLeave
            mClientLastDown->mIsOver = false;
        }
        touch.location += b;
        homura::CallVirtualFunc<Widget, 81, void, int, int>(mClientLastDown, touch.location.mX, touch.location.mY); // MouseDrag
    }
}

void Sexy::ScrollWidget::TouchEnded(Touch touch, int theMagicCode) {
    if (mScrollTracking) {
        TouchMotion(touch);
        mScrollTracking = false;
        mClientDownDispatched = false;
        mClientLastDown = nullptr;
    } else if (mClientLastDown != nullptr) {
        if (!mClientDownDispatched) {
            mClientLastDown->mIsDown = true;
            mClientLastDown->mIsOver = true;
            homura::CallVirtualFunc<Widget, 76, void, int, int, int>(mClientLastDown, mScrollTouchReference.mX, mScrollTouchReference.mY, mDeferredMouseDownMagicCode); // MouseDown
            mClientDownDispatched = true;
        }
        Point b = homura::CallVirtualFunc<Widget, 19, Point>(this) /*GetAbsPos*/ - homura::CallVirtualFunc<Widget, 19, Point>(mClientLastDown) /*GetAbsPos*/;
        Point a(touch.location.mX, touch.location.mY);
        // a + b;
        touch.location += b;
        homura::CallVirtualFunc<Widget, 79, void, int, int, int>(mClientLastDown, touch.location.mX, touch.location.mY, theMagicCode); // MouseUp
        mClientLastDown->mIsDown = false;
        mClientLastDown->mIsOver = false;
        mClientLastDown = nullptr;
        mClientDownDispatched = false;
    }
}


void Sexy::ScrollWidget::TouchMotion(Touch touch) {
    Sexy::FPoint cgpoint = Sexy::FPoint(touch.location.mX, touch.location.mY) - mScrollTouchReference;
    Sexy::FPoint cgpoint2 = mScrollOffset;
    if ((mScrollPractical & ScrollMode::SCROLL_HORIZONTAL) != ScrollMode::SCROLL_DISABLED) {
        cgpoint2.mX = mScrollOffsetReference.mX + cgpoint.mX;
        float x = mScrollMin.mX;
        float x2 = mScrollMax.mX;
        if (cgpoint2.mX < x) {
            cgpoint2.mX = (mBounceEnabled ? (cgpoint2.mX + 0.5f * (x - cgpoint2.mX)) : x);
            mScrollVelocity.mX = 0.0f;
        } else if (cgpoint2.mX > x2) {
            cgpoint2.mX = (mBounceEnabled ? (cgpoint2.mX + 0.5f * (x2 - cgpoint2.mX)) : x2);
            mScrollVelocity.mX = 0.0f;
        } else {
            float num = cgpoint2.mX - mScrollOffset.mX;
            double num2 = touch.timestamp - mScrollLastTimestamp;
            if (num2 > 0.0) {
                double num3 = num / num2;
                double num4 = std::min(1.0, num2 / 0.10000000149011612);
                mScrollVelocity.mX = (float)(num4 * num3 + (1.0 - num4) * mScrollVelocity.mX);
            }
        }
    }
    if ((mScrollPractical & ScrollMode::SCROLL_VERTICAL) != ScrollMode::SCROLL_DISABLED) {
        cgpoint2.mY = mScrollOffsetReference.mY + cgpoint.mY;
        float y = mScrollMin.mY;
        float y2 = mScrollMax.mY;
        if (cgpoint2.mY < y) {
            cgpoint2.mY = (mBounceEnabled ? (cgpoint2.mY + 0.5f * (y - cgpoint2.mY)) : y);
            mScrollVelocity.mY = 0.0f;
        } else if (cgpoint2.mY > y2) {
            cgpoint2.mY = (mBounceEnabled ? (cgpoint2.mY + 0.5f * (y2 - cgpoint2.mY)) : y2);
            mScrollVelocity.mY = 0.0f;
        } else {
            float num5 = cgpoint2.mY - mScrollOffset.mY;
            double num6 = touch.timestamp - mScrollLastTimestamp;
            if (num6 > 0.0) {
                double num7 = num5 / num6;
                double num8 = std::min(1.0, num6 / 0.10000000149011612);
                mScrollVelocity.mY = (float)(num8 * num7 + (1.0 - num8) * mScrollVelocity.mY);
            }
        }
    }
    mScrollOffset = cgpoint2;
    mScrollLastTimestamp = touch.timestamp;
    mClient->Move((int)mScrollOffset.mX, (int)mScrollOffset.mY);
}

// void Sexy::ScrollWidget::MouseWheel(int theDelta)
// {
//     if ((mScrollPractical & ScrollMode::SCROLL_VERTICAL) != ScrollMode::SCROLL_DISABLED)
//     {
//         //mScrollOffset.mY += theDelta;
//         mScrollVelocity.mY += theDelta * 1.4f;
//     }
//     else if ((mScrollPractical & ScrollMode::SCROLL_HORIZONTAL) != ScrollMode::SCROLL_DISABLED)
//     {
//         //mScrollOffset.mX += theDelta;
//         mScrollVelocity.mX += theDelta * 1.4f;
//     }
//     //mScrollOffset = point2;
//     //mScrollLastTimestamp = touch.timestamp;
//     mClient->Move((int)mScrollOffset.mX, (int)mScrollOffset.mY);
//     //oldTouch = touch.location;
//     //oldTouchTime = touch.timestamp;
// }

void Sexy::ScrollWidget::Update() {
    mUpdateCnt++;
    DoScrollUpdate();
}

static float CGVectorNorm(Sexy::FPoint v) {
    return v.mX * v.mX + v.mY * v.mY;
}

static Sexy::FPoint CGPointAddScaled(Sexy::FPoint augend, Sexy::FPoint addend, float factor) {
    return {
        augend.mX + addend.mX * factor,
        augend.mY + addend.mY * factor,
    };
}

void Sexy::ScrollWidget::DoScrollUpdate() {
    if (mVisible && !mDisabled) {
        if (mIsDown) {
            mIndicatorsFlashTimer = SCROLL_INDICATORS_FLASH_TICKS;
        } else {
            float num = std::min(0.0f, mScrollMin.mX);
            float num2 = std::min(0.0f, mScrollMin.mY);
            float num3 = mScrollMax.mX;
            float num4 = mScrollMax.mY;
            if (mSeekScrollTarget) {
                float num5 = CGVectorNorm(mScrollTarget - mScrollOffset);
                if (num5 < 0.01f) {
                    mScrollOffset = mScrollTarget;
                    mSeekScrollTarget = false;
                    // if (mListener != nullptr)
                    // {
                    //     mListener->ScrollTargetReached(this);
                    // }
                } else {
                    num3 = (num = mScrollTarget.mX);
                    num4 = (num2 = mScrollTarget.mY);
                }
            }
            float num6 = CGVectorNorm(mScrollVelocity);
            if (num6 < 0.0001f) {
                mScrollVelocity = Sexy::FPoint(0.0f, 0.0f);
            } else {
                bool flag = mScrollOffset.mX < num || mScrollOffset.mX >= num3;
                bool flag2 = mScrollOffset.mY < num2 || mScrollOffset.mY >= num4;
                Sexy::FPoint multiplier = Sexy::FPoint();
                multiplier.mX = (flag ? 0.85f : 0.975f);
                multiplier.mY = (flag2 ? 0.85f : 0.975f);
                mScrollOffset = CGPointAddScaled(mScrollOffset, mScrollVelocity, 0.01f);
                mScrollVelocity = mScrollVelocity * multiplier;
            }
            if (mScrollOffset.mX < num) {
                if (mBounceEnabled || mSeekScrollTarget) {
                    float num7 = (mSpringOverride == 0.0f) ? 0.1f : mSpringOverride;
                    mScrollOffset.mX = mScrollOffset.mX + num7 * (num - mScrollOffset.mX);
                } else {
                    mScrollOffset.mX = num;
                    mScrollVelocity.mX = 0.0f;
                }
            } else if (mScrollOffset.mX > num3) {
                if (mBounceEnabled || mSeekScrollTarget) {
                    float num8 = (mSpringOverride == 0.0f) ? 0.1f : mSpringOverride;
                    mScrollOffset.mX = mScrollOffset.mX + num8 * (num3 - mScrollOffset.mX);
                } else {
                    mScrollOffset.mX = num3;
                    mScrollVelocity.mX = 0.0f;
                }
            }
            if (mScrollOffset.mY < num2) {
                if (mBounceEnabled || mSeekScrollTarget) {
                    float num9 = (mSpringOverride == 0.0f) ? 0.1f : mSpringOverride;
                    mScrollOffset.mY = mScrollOffset.mY + num9 * (num2 - mScrollOffset.mY);
                } else {
                    mScrollOffset.mY = num2;
                    mScrollVelocity.mY = 0.0f;
                }
            } else if (mScrollOffset.mY > num4) {
                if (mBounceEnabled || mSeekScrollTarget) {
                    float num10 = (mSpringOverride == 0.0f) ? 0.1f : mSpringOverride;
                    mScrollOffset.mY = mScrollOffset.mY + num10 * (num4 - mScrollOffset.mY);
                } else {
                    mScrollOffset.mY = num4;
                    mScrollVelocity.mY = 0.0f;
                }
            }
            if (mClient != nullptr) {
                mClient->Move((int)mScrollOffset.mX, (int)mScrollOffset.mY);
            }
            if (mIndicatorsFlashTimer > 0) {
                mIndicatorsFlashTimer--;
            }
        }
        if (mIndicatorsFlashTimer > 0 && mIndicatorsOpacity < 1.0f) {
            mIndicatorsOpacity = std::min(1.0f, mIndicatorsOpacity + SCROLL_INDICATORS_FADE_IN_RATE);
            return;
        }
        if (mIndicatorsFlashTimer == 0 && mIndicatorsOpacity > 0.0f) {
            mIndicatorsOpacity = std::max(0.0f, mIndicatorsOpacity - SCROLL_INDICATORS_FADE_OUT_RATE);
        }
    }
}

void Sexy::ScrollWidget::DrawHorizontalStretchableImage(Graphics *g, Image *image, const Rect &destRect) {
    int width = image->GetWidth();
    int height = image->GetHeight();
    Rect theSrcRect(0, 0, (width - 1) / 2, height);
    Rect theSrcRect2(theSrcRect.mWidth, 0, 1, height);
    Rect theSrcRect3(theSrcRect2.mX + theSrcRect2.mWidth, 0, width - theSrcRect.mWidth - theSrcRect2.mWidth, height);
    int theY = destRect.mY + (destRect.mHeight - height) / 2;
    Rect theDestRect(destRect.mX + theSrcRect.mWidth, theY, destRect.mWidth - theSrcRect.mWidth - theSrcRect3.mWidth, height);
    g->DrawImage(image, destRect.mX, theY, theSrcRect);
    g->DrawImage(image, theDestRect, theSrcRect2);
    g->DrawImage(image, destRect.mX + destRect.mWidth - theSrcRect3.mWidth, theY, theSrcRect3);
}

void Sexy::ScrollWidget::DrawVerticalStretchableImage(Graphics *g, Image *image, const Rect &destRect) {
    int width = image->GetWidth();
    int height = image->GetHeight();
    Rect theSrcRect(0, 0, width, (height - 1) / 2);
    Rect theSrcRect2(0, theSrcRect.mHeight, width, 1);
    Rect theSrcRect3(0, theSrcRect2.mY + theSrcRect2.mHeight, width, height - theSrcRect.mHeight - theSrcRect2.mHeight);
    int theX = destRect.mX + (destRect.mWidth - width) / 2;
    Rect theDestRect(theX, destRect.mY + theSrcRect.mHeight, width, destRect.mHeight - theSrcRect.mHeight - theSrcRect3.mHeight);
    g->DrawImage(image, theX, destRect.mY, theSrcRect);
    g->DrawImage(image, theDestRect, theSrcRect2);
    g->DrawImage(image, theX, destRect.mY + destRect.mHeight - theSrcRect3.mHeight, theSrcRect3);
}

void Sexy::ScrollWidget::Draw(Graphics *g) {
    if (mBackgroundImage) {
        g->DrawImage(mBackgroundImage, 0, 0);
        return;
    }
    if (mFillBackground) {
        Color aColor = homura::CallVirtualFunc<Widget, 46, Color, int>(this, 0);
        g->SetColor(aColor);
        Rect aRect(0, 0, mWidth, mHeight);
        g->FillRect(aRect);
    }
}

// void Sexy::ScrollWidget::DrawProxyWidget(Graphics* g, ProxyWidget* proxyWidget)
// {
//     Color color = new Color(255, 255, 255, (int)(255f * mIndicatorsOpacity));
//     if (color.A != 0)
//     {
//         int width = mIndicatorsImage->GetWidth();
//         int height = mIndicatorsImage->GetHeight();
//         Insets insets = mIndicatorsInsets;
//         g->SetColor(color);
//         g->SetColorizeImages(true);
//         if ((mScrollPractical & ScrollMode::SCROLL_HORIZONTAL) != ScrollMode::SCROLL_DISABLED)
//         {
//             float num = mWidth / (float)mClient->Width();
//             int num2 = mWidth - insets.mLeft - insets.mRight - (((mScrollMode & ScrollMode::SCROLL_VERTICAL) != ScrollMode::SCROLL_DISABLED) ? width : 0);
//             int num3 = (int)(num2 * num);
//             int num4 = num2 - num3;
//             float num5 = std::min(0, mWidth - mClient->mWidth - mScrollInsets.mRight);
//             float num6 = mScrollInsets.mLeft;
//             float num7 = 1f - (mScrollOffset.mX - num5) / (num6 - num5);
//             int num8 = (int)(num4 * num7);
//             int num9 = num8 + num3;
//             num8 = std::min(std::max(0, num8), num2 - width);
//             num9 = std::min(std::max(width, num9), num2);
//             Rect destRect = default(Rect);
//             destRect.mX = insets.mLeft + num8;
//             destRect.mY = mHeight - insets.mBottom - height;
//             destRect.mWidth = num9 - num8;
//             destRect.mHeight = height;
//             ScrollWidget->DrawHorizontalStretchableImage(g, mIndicatorsImage, destRect);
//         }
//         if ((mScrollPractical & ScrollMode::SCROLL_VERTICAL) != ScrollMode::SCROLL_DISABLED)
//         {
//             float num10 = mHeight / (float)mClient->Height();
//             int num11 = mHeight - insets.mTop - insets.mBottom - (((mScrollMode & ScrollMode::SCROLL_HORIZONTAL) != ScrollMode::SCROLL_DISABLED) ? height : 0);
//             int num12 = (int)(num11 * num10);
//             int num13 = num11 - num12;
//             float num14 = std::min(0, mHeight - mClient->mHeight - mScrollInsets.mBottom);
//             float num15 = mScrollInsets.mTop;
//             float num16 = 1f - (mScrollOffset.mY - num14) / (num15 - num14);
//             int num17 = (int)(num13 * num16);
//             int num18 = num17 + num12;
//             num17 = std::min(std::max(0, num17), num11 - height);
//             num18 = std::min(std::max(height, num18), num11);
//             Rect destRect2 = default(Rect);
//             destRect2.mX = mWidth - insets.mRight - width;
//             destRect2.mY = insets.mTop + num17;
//             destRect2.mWidth = width;
//             destRect2.mHeight = num18 - num17;
//             ScrollWidget->DrawVerticalStretchableImage(g, mIndicatorsImage, destRect2);
//         }
//     }
//     if (mDrawOverlays)
//     {
//         g->SetColorizeImages(false);
//         foreach (ScrollWidget->Overlay overlay in mOverlays)
//         {
//             g->DrawImage(overlay->image, overlay->offset.mX, overlay->offset.mY);
//         }
//     }
// }

void Sexy::ScrollWidget::SnapToPage() {
    Sexy::FPoint point = Sexy::FPoint(mScrollInsets.mLeft + mPageSize.mX / 2.0f, mScrollInsets.mTop + mPageSize.mY / 2.0f) - mScrollOffset;
    int num = (int)std::floor(point.mX / mPageSize.mX);
    int num2 = (int)std::floor(point.mY / mPageSize.mY);
    num = std::max(0, std::min(num, mPageCountHorizontal - 1));
    num2 = std::max(0, std::min(num2, mPageCountVertical - 1));
    Sexy::FPoint point2 = Sexy::FPoint();
    point2.mX = mScrollInsets.mLeft - num * mPageSize.mX;
    point2.mY = mScrollInsets.mTop - num2 * mPageSize.mY;
    if (mScrollVelocity.mX > 40.0f && point2.mX < mScrollOffset.mX) {
        num--;
    } else if (mScrollVelocity.mX < -40.0f && point2.mX > mScrollOffset.mX) {
        num++;
    }
    if (mScrollVelocity.mY > 40.0f && point2.mY < mScrollOffset.mY) {
        num2--;
    } else if (mScrollVelocity.mY < -40.0f && point2.mY > mScrollOffset.mY) {
        num2++;
    }
    SetPage(num, num2, true);
}

Sexy::Widget *Sexy::ScrollWidget::GetClientWidgetAt(Touch touch) {
    int num = (int)touch.location.mX - mClient->mX;
    int num2 = (int)touch.location.mY - mClient->mY;
    int theFlags = 16 | mWidgetManager->GetWidgetFlags();
    Widget *widgetAtHelper = nullptr;
    int num3 = 0;
    int num4 = 0;
    if (mClientLastDown != nullptr) {
        Point absPos = homura::CallVirtualFunc<Widget, 19, Point>(mClient) /*GetAbsPos*/;
        Point absPos2 = homura::CallVirtualFunc<Widget, 19, Point>(mClientLastDown) /*GetAbsPos*/;
        widgetAtHelper = mClientLastDown;
        num3 = (int)(touch.location.mX + absPos.mX - absPos2.mX);
        num4 = (int)(touch.location.mY + absPos.mY - absPos2.mY);
    } else {
        Widget *widget = mClient;
        widget->mWidgetFlagsMod.mRemoveFlags = (widget->mWidgetFlagsMod.mRemoveFlags & -17);
        bool flag = false;
        widgetAtHelper = mClient->GetWidgetAtHelper(num, num2, theFlags, &flag, &num3, &num4);
        Widget *widget2 = mClient;
        widget2->mWidgetFlagsMod.mRemoveFlags = (widget2->mWidgetFlagsMod.mRemoveFlags | 16);
    }
    if (widgetAtHelper == nullptr || widgetAtHelper->mDisabled) {
        num3 = num;
        num4 = num2;
        widgetAtHelper = mClient;
    }
    touch.location.mX = num3;
    touch.location.mY = num4;
    return widgetAtHelper;
}

Sexy::FPoint &Sexy::ScrollWidget::GetScrollOffset() {
    return mScrollOffset;
}

void Sexy::ScrollWidget::SetScrollOffset(float x, float y) {
    mScrollOffset.mX = x;
    mScrollOffset.mY = y;
}

void Sexy::ScrollWidget::CacheDerivedValues() {
    if (mClient != nullptr) {
        mScrollMin.mX = mWidth - mClient->mWidth - mScrollInsets.mRight;
        mScrollMin.mY = mHeight - mClient->mHeight - mScrollInsets.mBottom;
        mScrollMax.mX = mScrollInsets.mLeft;
        mScrollMax.mY = mScrollInsets.mTop;
        int num = ((mScrollMin.mX < mScrollMax.mX) ? 1 : 0) | ((mScrollMin.mY < mScrollMax.mY) ? 2 : 0);
        mScrollPractical = ScrollMode(mScrollMode & num);
    } else {
        mScrollMin.mX = (mScrollMax.mX = (mScrollMin.mY = (mScrollMax.mY = 0.0f)));
        mScrollPractical = ScrollMode::SCROLL_DISABLED;
    }
}
