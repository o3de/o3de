/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorLabel.h>
#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorHexEdit.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Swatch.h>
#include <QHBoxLayout>
#include <QEvent>

namespace AzQtComponents
{
    ColorLabel::ColorLabel(QWidget* parent)
        : ColorLabel(AZ::Colors::Red, parent)
    {
    }

    ColorLabel::ColorLabel(const AZ::Color& color, QWidget* parent)
        : QWidget(parent)
        , m_swatch(new Swatch(this))
        , m_hexEdit(new ColorHexEdit(this))
    {
        QHBoxLayout* boxLayout = new QHBoxLayout(this);

        boxLayout->setAlignment(Qt::AlignLeft);
        boxLayout->setContentsMargins(0, 0, 0, 0);
        boxLayout->setSpacing(0);

        boxLayout->addWidget(m_swatch);
        boxLayout->addWidget(m_hexEdit);

        m_swatch->installEventFilter(this);

        connect(m_hexEdit, &ColorHexEdit::valueChangeEnded, this, &ColorLabel::onHexEditColorChanged);
        connect(m_swatch, &Swatch::colorChanged, this, &ColorLabel::setColor);

        m_hexEdit->setEditAlpha(false);
        m_hexEdit->setLabelVisible(false);

        setColor(color);
    }

    ColorLabel::~ColorLabel()
    {
    }

    void ColorLabel::setTextInputVisible(bool visible)
    {
        m_hexEdit->setVisible(visible);
    }

    void ColorLabel::setColor(const AZ::Color& color)
    {
        const bool doEmit = m_color != color;
        m_color = color;

        updateSwatchColor();
        updateHexColor();

        if (doEmit)
        {
            emit colorChanged(m_color);
        }
    }

    bool ColorLabel::eventFilter(QObject* object, QEvent* event)
    {
        if (event->type() == QEvent::MouseButtonRelease)
        {
            setColor(AzQtComponents::ColorPicker::getColor(AzQtComponents::ColorPicker::Configuration::RGB, m_color, QStringLiteral("Color Picker RGB"), QString(), QStringList(), this));
            return true;
        }

        return QObject::eventFilter(object, event);
    }

    void ColorLabel::onHexEditColorChanged()
    {
        const AZ::Color color(static_cast<float>(m_hexEdit->red()), static_cast<float>(m_hexEdit->green()), static_cast<float>(m_hexEdit->blue()), 1.0f);
        setColor(color);
    }

    void ColorLabel::updateSwatchColor()
    {
        m_swatch->setColor(m_color);
    }

    void ColorLabel::updateHexColor()
    {
        m_hexEdit->setRed(m_color.GetR());
        m_hexEdit->setGreen(m_color.GetG());
        m_hexEdit->setBlue(m_color.GetB());
    }
}

#include <Components/Widgets/moc_ColorLabel.cpp>
