/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        CGImageRef cgImage = CGWindowListCreateImageFromArray(bounds, windows, kCGWindowImageNominalResolution);
        CFRelease(windows);

        QImage result = QtMac::fromCGImageRef(cgImage).toImage();
        CGImageRelease(cgImage);

        return result;
    }

} // namespace AzQtComponents

#include "Utilities/moc_ScreenGrabber.cpp"