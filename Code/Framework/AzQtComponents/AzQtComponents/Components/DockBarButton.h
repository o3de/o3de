/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QProxyStyle>
#include <QToolButton>
#endif

class QSettings;
class QEvent;
class QMouseEvent;
class QWidget;

namespace AzQtComponents
{
    class Style;

    class AZ_QT_COMPONENTS_API DockBarButton
        : public QToolButton
    {
        Q_OBJECT
    public:
        struct Config
        {
            int buttonIconSize;
            int defaultButtonMargin;
            int menuIndicatorWidth;
            QString menuIndicatorIcon;
            QSize menuIndicatorIconSize;
            QColor hoverBackgroundColor;
            QColor closeHoverBackgroundColor;
            QColor selectedBackgroundColor;
            QColor closeSelectedBackgroundColor;
        };

        /*!
        * Loads the button config data from a settings object.
        */
        static Config loadConfig(QSettings& settings);

        /*!
        * Returns default button config data.
        */
        static Config defaultConfig();

        enum WindowDecorationButton
        {
            CloseButton,
            MaximizeButton,
            MinimizeButton,
            DividerButton
        };
        Q_ENUM(WindowDecorationButton)

        explicit DockBarButton(DockBarButton::WindowDecorationButton buttonType, QWidget* parent = nullptr, bool darkStyle = false);

        DockBarButton::WindowDecorationButton buttonType() const { return m_buttonType; }

        /*
        * Expose the button type using a QT property so that test automation can read it
        */
        Q_PROPERTY(WindowDecorationButton buttonType MEMBER m_buttonType CONSTANT)

    Q_SIGNALS:
        void buttonPressed(const DockBarButton::WindowDecorationButton type);

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        friend class Style;

        static bool drawDockBarButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config);

        void handleButtonClick();
        const DockBarButton::WindowDecorationButton m_buttonType;
        bool m_isDarkStyle;

    };
} // namespace AzQtComponents
