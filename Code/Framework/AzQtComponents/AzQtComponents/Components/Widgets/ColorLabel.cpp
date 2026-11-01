/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorLabel.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>

#include <QLineEdit>
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

        if (StyleManager::stylesheetsDisabled())
        {
            // ColorLabel.qss:20-23 $size.surface.idle.min-width.Alt5 $size.surface.idle.max-width.Alt3 $size.surface.idle.min-height.Alt4
            // $size.surface.idle.max-height.Alt3
            m_swatch->setFixedSize(32, 16);
            // ColorLabel.qss:31-32 $size.surface.idle.max-width.Alt6 $size.surface.idle.min-width.Alt7
            m_hexEdit->setFixedWidth(100);
            if (auto hexLineEdit = m_hexEdit->findChild<QLineEdit*>())
            {
                const QMargins box = LineEdit::flattenedFieldBox(hexLineEdit);
                // Contents width in the box: ColorLabel.qss:37-38 $size.field.idle.max-width.Alt4 $size.field.idle.min-width.Alt3
                hexLineEdit->setFixedWidth(100 + box.left() + box.right());
            }
        }

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

