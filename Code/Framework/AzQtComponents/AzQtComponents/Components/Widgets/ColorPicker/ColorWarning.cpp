/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorWarning.h>
#include <AzQtComponents/Components/Style.h>

#include <QHBoxLayout>
#include <QLabel>

namespace AzQtComponents
{

    namespace
    {
        const int LINE_HEIGHT = 16;
    }

    ColorWarning::ColorWarning(QWidget* parent /* = nullptr */)
        : ColorWarning(Mode::Warning, {}, {}, parent)
    {
    }

    ColorWarning::ColorWarning(ColorWarning::Mode mode, const AZ::Color& color, const QString& message, QWidget* parent /* = nullptr */)
        : QWidget(parent)
    {
        setFocusPolicy(Qt::ClickFocus);

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_iconLabel = new QLabel(this);
        m_iconLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        layout->addWidget(m_iconLabel);

        m_messageLabel = new QLabel(this);
        m_messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(m_messageLabel);

        setMode(mode);
        setColor(color);
        setMessage(message);
    }

    ColorWarning::~ColorWarning()
    {
    }

    ColorWarning::Mode ColorWarning::mode() const
    {
        return m_mode;
    }

    void ColorWarning::setMode(ColorWarning::Mode mode)
    {
        m_mode = mode;
        switch (m_mode)
        {
        case Mode::Warning:
            m_iconLabel->setPixmap(Style::cachedPixmap(QStringLiteral(":/Cards/img/UI20/Cards/warning.png")).scaledToHeight(LINE_HEIGHT, Qt::SmoothTransformation));
            break;
        case Mode::Error:
            m_iconLabel->setPixmap(Style::cachedPixmap(QStringLiteral(":/Cards/img/UI20/Cards/error_icon.png")).scaledToHeight(LINE_HEIGHT, Qt::SmoothTransformation));
            break;
        default:
            break;
        }
    }

    AZ::Color ColorWarning::color() const
    {
        return m_color;
    }

    void ColorWarning::setColor(const AZ::Color& color)
    {
        m_color = color;
    }

    const QString& ColorWarning::message() const
    {
        return m_message;
    }

    void ColorWarning::setMessage(const QString& message)
    {
        m_message = message;
        setToolTip(m_message);
        m_messageLabel->setText(m_message);
        setVisible(!m_message.isEmpty());
    }

    void ColorWarning::set(ColorWarning::Mode mode, const QString& message)
    {
        setMode(mode);
        setMessage(message);
        setVisible(true);
    }

    void ColorWarning::clear()
    {
        setMessage({});
    }

} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_ColorWarning.cpp"
