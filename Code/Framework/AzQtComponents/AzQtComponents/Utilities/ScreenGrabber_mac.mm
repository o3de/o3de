/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreFoundation/CoreFoundation.h>
#include <AppKit/AppKit.h>

#include <QImage>
#include <QPixmap>
#include <QtMac>

#include <AzQtComponents/Components/Widgets/Eyedropper.h>
#include <AzQtComponents/Utilities/ScreenGrabber.h>

namespace AzQtComponents
{

    class ScreenGrabber::Internal
    {};

    ScreenGrabber::ScreenGrabber(const QSize size, Eyedropper* parent /* = nullptr */)
        : QObject(parent)
        , m_size(size)
        , m_owner(parent)
    {
    }

    ScreenGrabber::~ScreenGrabber()
    {
    }

    QImage ScreenGrabber::grab(const QPoint& point) const
    {
        WId magnifier = m_owner->effectiveWinId();

        NSView* view = reinterpret_cast<NSView*>(magnifier);
        CGWindowID windowId = (CGWindowID)[[view window] windowNumber];

        CFArrayRef allWindows = CGWindowListCreate(kCGWindowListOptionAll | kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
        CFMutableArrayRef windows = CFArrayCreateMutableCopy(nullptr, 0, allWindows);
        for (int i = 0; i < CFArrayGetCount(windows); i++)
        {
            CGWindowID window = static_cast<CGWindowID>(reinterpret_cast<uint64_t>(CFArrayGetValueAtIndex(windows, i)));
            if (window == windowId)
            {
                CFArrayRemoveValueAtIndex(windows, i);
                break;
            }
        }
        CFRelease(allWindows);

        // The part of the screen we want to scale up
        QRect region({}, m_size);
        region.moveCenter(point);
        CGRect bounds = CGRectMake(region.x(), region.y(), region.width(), region.height());

        //TODO - Add proper support for macOS 14.0+.
        //Try looking into SCScreenshotManager (https://developer.apple.com/documentation/screencapturekit/scscreenshotmanager?language=objc)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        CGImageRef cgImage = CGWindowListCreateImageFromArray(bounds, windows, kCGWindowImageNominalResolution);
#pragma clang diagnostic pop

        CFRelease(windows);

        QImage result = QtMac::fromCGImageRef(cgImage).toImage();
        CGImageRelease(cgImage);

        return result;
    }

} // namespace AzQtComponents

#include "Utilities/moc_ScreenGrabber.cpp"
