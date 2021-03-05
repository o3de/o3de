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