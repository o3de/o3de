/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>

#include <QIconEngine>

namespace EMStudio
{
    //! Create QIcons with a single solid QColor
    class SolidColorIconEngine : public QIconEngine
    {
    public:
        explicit SolidColorIconEngine(const QColor& color);
        explicit SolidColorIconEngine(const AZ::Color& color);
        ~SolidColorIconEngine() = default;

        QColor color() const;
        void setColor(const QColor& color);
        void setColor(const AZ::Color& color);

        void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;
        QIconEngine* clone() const override;

    private:
        QColor m_color;
    };
} // namespace EMStudio
