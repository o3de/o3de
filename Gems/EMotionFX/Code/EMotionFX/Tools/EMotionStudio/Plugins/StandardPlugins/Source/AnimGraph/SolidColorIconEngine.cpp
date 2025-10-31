/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QPainter>

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/SolidColorIconEngine.h>

namespace EMStudio
{
    SolidColorIconEngine::SolidColorIconEngine(const QColor& color)
        : QIconEngine()
    {
        setColor(color);
    }

    SolidColorIconEngine::SolidColorIconEngine(const AZ::Color& color)
        : QIconEngine()
    {
        setColor(color);
    }

    QColor SolidColorIconEngine::color() const
    {
        return m_color;
    }

    void SolidColorIconEngine::setColor(const QColor& color)
    {
        m_color = color;
    }

    void SolidColorIconEngine::setColor(const AZ::Color& color)
    {
        m_color = QColor::fromRgb(color.GetR8(), color.GetG8(), color.GetB8(), color.GetA8());
    }

    void SolidColorIconEngine::paint(
        QPainter* painter, const QRect& rect, [[maybe_unused]] QIcon::Mode mode, [[maybe_unused]] QIcon::State state)
    {
        painter->setBrush(m_color);
        painter->drawRect(rect);
    }

    QIconEngine* SolidColorIconEngine::clone() const
    {
        return new SolidColorIconEngine(m_color);
    }
} // namespace EMStudio
