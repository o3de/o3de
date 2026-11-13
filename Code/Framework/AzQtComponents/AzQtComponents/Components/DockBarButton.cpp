/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/DockBarButton.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Elements/TabElement.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QApplication>
#include <QEvent>
#include <QStyleOptionToolButton>
#include <QStylePainter>

namespace AzQtComponents
{
    //////////////////////////////////////////////////////////////////////////
    // Flattened window buttons: the fills the sheet gave the minimize, maximize and close buttons while the pointer is over them
    // (TitleBar.qss:97-105). The sheet has no :pressed rule for them, so these are the fills of a pressed button under the pointer too.
    //////////////////////////////////////////////////////////////////////////

    //! TitleBar.qss:100 `rgba(255,255,255,0.1)`: Qt reads an alpha of 1 or less as a fraction and truncates it, 0.1 * 255 = 25
    //! (QCss::parseColorValue). TOKENIZE: DockBarButtonHoverFill
    static constexpr QColor g_dockBarButtonHoverFill(255, 255, 255, 25);
    //! TitleBar.qss:104 `#B80D1C`. TOKENIZE: DockBarButtonCloseHoverFill
    static constexpr QColor g_dockBarButtonCloseHoverFill(0xB8, 0x0D, 0x1C);

    DockBarButton::Config DockBarButton::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

        ConfigHelpers::read<int>(settings, QStringLiteral("ButtonIconSize"), config.buttonIconSize);
        ConfigHelpers::read<int>(settings, QStringLiteral("DefaultButtonMargin"), config.defaultButtonMargin);
        ConfigHelpers::read<int>(settings, QStringLiteral("MenuIndicatorWidth"), config.menuIndicatorWidth);
        ConfigHelpers::read<QString>(settings, QStringLiteral("MenuIndicatorIcon"), config.menuIndicatorIcon);
        ConfigHelpers::read<QSize>(settings, QStringLiteral("MenuIndicatorIconSize"), config.menuIndicatorIconSize);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("HoverBackgroundColor"), config.hoverBackgroundColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("CloseHoverBackgroundColor"), config.closeHoverBackgroundColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("SelectedBackgroundColor"), config.selectedBackgroundColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("CloseSelectedBackgroundColor"), config.closeSelectedBackgroundColor);

        return config;
    }

    DockBarButton::Config DockBarButton::defaultConfig()
    {
        Config config;

        config.buttonIconSize = 16;
        config.defaultButtonMargin = 1;
        config.menuIndicatorWidth = 10;
        config.menuIndicatorIcon = QStringLiteral(":/stylesheet/img/UI20/menu-indicator.svg");
        config.menuIndicatorIconSize = QSize(6, 3);
        config.hoverBackgroundColor = QStringLiteral("#11FFFFFF");
        config.closeHoverBackgroundColor = QStringLiteral("#B80D1C");
        config.selectedBackgroundColor = QStringLiteral("#1A1A1A");
        config.closeSelectedBackgroundColor = QStringLiteral("#850914");

        return config;
    }

    /**
     * Create a dock bar button that can be shared between any kind of docking bars for common actions
     */
    DockBarButton::DockBarButton(DockBarButton::WindowDecorationButton buttonType, QWidget* parent, bool darkStyle)
        : QToolButton(parent)
        , m_buttonType(buttonType)
        , m_isDarkStyle(darkStyle)
    {
        Style::addClass(this, NoMargins);
        switch (m_buttonType)
        {
            case DockBarButton::CloseButton:
                Style::addClass(this, QStringLiteral("close"));
                setObjectName("closeButton");
                setToolTip(tr("Close"));
                break;
            case DockBarButton::MaximizeButton:
                SetMaximizeRestoreButton();
                setObjectName("maximizeButton");
                break;
            case DockBarButton::MinimizeButton:
                Style::addClass(this, QStringLiteral("minimize"));
                setObjectName("minimizeButton");
                setToolTip(tr("Minimize"));
                break;
            case DockBarButton::DividerButton:
                break;
        }

        QWidget* topLevelWindow = window();
        if (topLevelWindow)
        {
            topLevelWindow->installEventFilter(this);
        }

        if (m_isDarkStyle)
        {
            Style::addClass(this, QStringLiteral("dark"));
        }
        
        // Handle when our button is clicked
        QObject::connect(this, &QToolButton::clicked, this, &DockBarButton::handleButtonClick);

        // Our dock bar buttons only need click focus, they don't need to accept
        // focus by tabbing
        setFocusPolicy(Qt::ClickFocus);
    }

    bool DockBarButton::eventFilter(QObject* object, QEvent* event)
    {
        if (event->type() == QEvent::WindowStateChange && m_buttonType == DockBarButton::MaximizeButton)
        {
            SetMaximizeRestoreButton();
        }
        return QWidget::eventFilter(object, event);
    }

    void DockBarButton::paintEvent(QPaintEvent *)
    {
        QStylePainter p(this);
        QStyleOptionToolButton option;
        initStyleOption(&option);
        // Contents margins stand for a stylesheet margin (flattened GUI); none are set otherwise
        option.rect = contentsRect();

        // Set the icon based on m_buttonType. This allows the icon to be changed in a QStyle, or in
        // a Qt Style Sheet by changing the titlebar-close-icon, titlebar-maximize-icon, and
        // titlebar-minimize-icon properties.
        // Used in combination with the close, maximize, minimize and dark classes set in the
        // constructor, and :hover and :pressed selectors available to buttons, we have full control
        // of the pixmap in the style sheet.
        switch (m_buttonType)
        {
            case DockBarButton::CloseButton:
                option.icon = style()->standardIcon(QStyle::SP_TitleBarCloseButton, &option, this);
                break;
            case DockBarButton::MaximizeButton:
                option.icon = style()->standardIcon(QStyle::SP_TitleBarMaxButton, &option, this);
                break;
            case DockBarButton::MinimizeButton:
                option.icon = style()->standardIcon(QStyle::SP_TitleBarMinButton, &option, this);
                break;
            default:
                break;
        }

        p.drawComplexControl(QStyle::CC_ToolButton, option);
    }

    /**
     * Handle our button clicks by emitting a signal with our button type
     */
    void DockBarButton::handleButtonClick()
    {
        if (!window())
        {
            return;
        }

        emit buttonPressed(m_buttonType);
    }

    void DockBarButton::SetMaximizeRestoreButton()
    {
        if (window() && window()->isMaximized())
        {
            Style::removeClass(this, QStringLiteral("maximize"));
            Style::addClass(this, QStringLiteral("restore"));
            setToolTip(tr("Restore"));
        }
        else
        {
            Style::removeClass(this, QStringLiteral("restore"));
            Style::addClass(this, QStringLiteral("maximize"));
            setToolTip(tr("Maximize"));
        }
    }

    bool DockBarButton::drawDockBarButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        auto dockBarButton = qobject_cast<const DockBarButton*>(widget);
        auto buttonOption = qstyleoption_cast<const QStyleOptionToolButton*>(option);
        if (!dockBarButton || !buttonOption)
        {
            return false;
        }

        QRect buttonRect = style->subControlRect(QStyle::CC_ToolButton, option, QStyle::SC_ToolButton, widget);

        painter->save();

        QStyleOptionToolButton label = *buttonOption;

        // Do not draw the config's hover rect if the button is used in a Tab. In the legacy GUI Qt's
        // stylesheet path paints a dock tab's close button (TitleBar.qss) and never reaches this
        // function; the flattened GUI takes that button's hover fill from the tab element.
        auto tabBarParent = qobject_cast<const AzQtComponents::TabBar*>(widget->parent());
        const TabElement* tabElement = (tabBarParent && dockBarButton->m_buttonType == DockBarButton::CloseButton && StyleManager::stylesheetsDisabled())
            ? TabElement::forTabBar(tabBarParent)
            : nullptr;

        bool mouseOver = buttonOption->state & QStyle::State_MouseOver;
        Qt::MouseButtons mouseButtons = QApplication::mouseButtons();
        bool mouseDown = mouseButtons & Qt::LeftButton;

        // The flattened GUI draws the fills of the sheet (TitleBar.qss:94-105): no idle fill (`background: transparent`), and one fill
        // under the pointer. That fill is the :hover rule's, which needs the button enabled (QStyleSheetStyle pseudoClass), and it is
        // the same pressed or not: the sheet has no :pressed rule for these buttons, and it does not read the application's mouse
        // buttons. The fills of the config are the legacy GUI's.
        const bool flat = StyleManager::stylesheetsDisabled();

        if (tabElement && mouseOver && tabElement->m_closeHoverFill.isValid())
        {
            // over the button's box, its margin excluded (paintEvent passes the contents rect)
            painter->fillRect(buttonOption->rect, tabElement->m_closeHoverFill);
        }
        else if (!tabBarParent && mouseOver && (!flat || buttonOption->state.testFlag(QStyle::State_Enabled)))
        {
            painter->setPen(Qt::NoPen);

            if (flat)
            {
                painter->setBrush(dockBarButton->m_buttonType == DockBarButton::CloseButton
                    ? g_dockBarButtonCloseHoverFill : g_dockBarButtonHoverFill);
            }
            else if (dockBarButton->m_buttonType == DockBarButton::CloseButton)
            {
                if (mouseDown)
                {
                    painter->setBrush(config.closeSelectedBackgroundColor);
                }
                else
                {
                    painter->setBrush(config.closeHoverBackgroundColor);
                }
            }
            else
            {
                if (mouseDown)
                {
                    painter->setBrush(config.selectedBackgroundColor);
                }
                else
                {
                    painter->setBrush(config.hoverBackgroundColor);
                }
                
            }

            const QRect highlightRect = buttonOption->rect;
            painter->drawRect(highlightRect);
        }

        label.rect = buttonRect;
        style->drawControl(QStyle::CE_ToolButtonLabel, &label, painter, widget);

        painter->restore();

        return true;
    }

} // namespace AzQtComponents
