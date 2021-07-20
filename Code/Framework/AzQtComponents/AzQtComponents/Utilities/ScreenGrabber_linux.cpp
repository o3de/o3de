/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QImage>
#include <QPixmap>

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
        QImage empty;
        QImage result = QImage();
        return result;
    }

} // namespace AzQtComponents

#include "Utilities/moc_ScreenGrabber.cpp"
