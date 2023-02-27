/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <QWidget>
#include <QPixmap>

namespace AzToolsFramework
{
    //////////////////////////////////////////////////////////////////////////
    // Custom widget to render a QPixmap, centered, with its aspect ratio
    // maintained and always fitting within the confines of the widget's
    // bounding box. You can use this instead of a QLabel if that's the
    // functionality you are looking for.
    class AspectRatioAwarePixmapWidget : public QWidget
    {
    public:
        AZ_CLASS_ALLOCATOR(AspectRatioAwarePixmapWidget, AZ::SystemAllocator);

        explicit AspectRatioAwarePixmapWidget(QWidget* parent = 0);

        const QPixmap* pixmap() const { return &m_pixmap; }
        void setPixmap(const QPixmap& p);

        int heightForWidth(int w) const override;
        bool hasHeightForWidth() const override;


    protected:
        void paintEvent(QPaintEvent* e) override;

        QPixmap m_pixmap;
        QPixmap m_scaledPixmap;
    };
}

