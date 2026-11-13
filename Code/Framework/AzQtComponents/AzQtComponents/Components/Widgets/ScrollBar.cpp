/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ScrollBar.h>
#include <AzQtComponents/Components/Widgets/ScrollBarProperties.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/function/invoke.h>

#include <QObject>
#include <QAbstractScrollArea>
#include <QMap>
#include <QSettings>
#include <QScrollBar>
#include <QEvent>

namespace AzQtComponents
{
    static constexpr const char* g_scrollBarModeProperty = "ScrollBarMode";

    class ScrollBarWatcher : public QObject
    {
    public:
        explicit ScrollBarWatcher(QObject* parent = nullptr)
            : QObject(parent)
        {
        }

        bool install(QObject* widget)
        {
            auto scrollArea = qobject_cast<QAbstractScrollArea*>(widget);
            if (scrollArea && !m_widgets.contains(scrollArea))
            {
                auto mode = m_config.defaultMode;
                const auto scrollBarMode = scrollArea->property(g_scrollBarModeProperty);
                if (scrollBarMode.isValid() && scrollBarMode.canConvert<ScrollBar::ScrollBarMode>())
                {
                    mode = scrollBarMode.value<ScrollBar::ScrollBarMode>();
                }
                else
                {
                    scrollArea->setProperty(g_scrollBarModeProperty, QVariant::fromValue(mode));
                }
                auto scrollBars = scrollArea->findChildren<QScrollBar*>();
                if (!scrollBars.isEmpty())
                {
                    ScrollAreaData data;
                    data.hoverWasEnabled = scrollArea->testAttribute(Qt::WA_Hover);
                    for (auto scrollBar : scrollBars)
                    {
                        if (mode == ScrollBar::ScrollBarMode::ShowOnHover)
                        {
                            scrollBar->hide();
                        }
                        data.scrollBars.append(scrollBar);
                        scrollBar->installEventFilter(this);
                    }
                    scrollArea->setAttribute(Qt::WA_Hover);
                    scrollArea->installEventFilter(this);
                    m_widgets.insert(widget, data);
                    auto cornerWidget = new QWidget();
                    cornerWidget->setProperty(ScrollBarProperties::ShowBackground, false);
                    scrollArea->setCornerWidget(cornerWidget);
                    cornerWidget->installEventFilter(this);

                    return true;
                }

                connect(scrollArea, &QObject::destroyed, this, &ScrollBarWatcher::scrollAreaDestroyed);
            }
            return false;
        }

        bool uninstall(QObject* widget)
        {
            auto scrollArea = qobject_cast<QAbstractScrollArea*>(widget);
            if (scrollArea && m_widgets.contains(widget))
            {
                disconnect(scrollArea, &QObject::destroyed, this, &ScrollBarWatcher::scrollAreaDestroyed);

                ScrollAreaData data = m_widgets.take(widget);
                scrollArea->removeEventFilter(this);
                scrollArea->setAttribute(Qt::WA_Hover, data.hoverWasEnabled);
                return true;
            }
            return false;
        }

        bool eventFilter(QObject* obj, QEvent* event) override
        {
            if (auto scrollArea = qobject_cast<QAbstractScrollArea*>(obj))
            {
                auto scrollBarMode = scrollArea->property(g_scrollBarModeProperty).value<ScrollBar::ScrollBarMode>();

                switch (event->type())
                {
                    case QEvent::HoverEnter:
                    {
                        if (scrollBarMode == ScrollBar::ScrollBarMode::ShowOnHover)
                        {
                            perScrollBar(obj, &QScrollBar::show);
                        }
                    }
                    break;

                    case QEvent::HoverLeave:
                    {
                        if (scrollBarMode == ScrollBar::ScrollBarMode::ShowOnHover)
                        {
                            perScrollBar(obj, &QScrollBar::hide);
                        }
                    }
                    break;

                    default:
                        break;
                }
            }
            else if (auto scrollBar = qobject_cast<QScrollBar*>(obj))
            {
                QAbstractScrollArea* parentScrollArea;
                QWidget* parent = scrollBar->parentWidget();

                while (parent && !qobject_cast<QAbstractScrollArea*>(parent))
                {
                    parent = parent->parentWidget();
                }

                if (parent)
                {
                    parentScrollArea = qobject_cast<QAbstractScrollArea*>(parent);

                    switch (event->type())
                    {
                        case QEvent::HoverEnter:
                        {
                            if (parentScrollArea->cornerWidget())
                            {
                                parentScrollArea->cornerWidget()->setProperty(ScrollBarProperties::ShowBackground, true);
                            }
                        }
                        break;
                        case QEvent::HoverLeave:
                        {
                            if (parentScrollArea->cornerWidget())
                            {
                                parentScrollArea->cornerWidget()->setProperty(ScrollBarProperties::ShowBackground, false);
                            }
                        }
                        break;
                    }
                }
            }
            else if (auto cornerWidget = qobject_cast<QWidget*>(obj))
            {
                switch (event->type())
                {
                    case QEvent::DynamicPropertyChange:
                    {
                        // ignore properties coming from inside the style sheet system itself, which are all by convention
                        // prefixed with _q_
                        QDynamicPropertyChangeEvent* eventFull = static_cast<QDynamicPropertyChangeEvent*>(event);
                        QString propertyName = QString::fromUtf8(eventFull->propertyName());
                        if (!propertyName.startsWith(QStringLiteral("_q_")))
                        {
                            if (auto styleSheet = StyleManager::styleSheetStyle(cornerWidget))
                            {
                                styleSheet->unpolish(cornerWidget);
                                styleSheet->polish(cornerWidget);
                            }
                        }
                    }
                    break;
                }
            }
            return QObject::eventFilter(obj, event);
        }

    private:
        friend class ScrollBar;
        ScrollBar::Config m_config;

        struct ScrollAreaData
        {
            bool hoverWasEnabled;

            // Store QPointers so that if the scrollbars are cleaned up before the scroll area, we don't continue to reference it
            QList<QPointer<QScrollBar>> scrollBars;
        };
        QMap<QObject*, ScrollAreaData> m_widgets;

        void perScrollBar(QObject* scrollArea, void (QScrollBar::*callback)())
        {
            auto iterator = m_widgets.find(scrollArea);
            if (iterator != m_widgets.end())
            {
                for (auto& scrollBar : iterator.value().scrollBars)
                {
                    if (scrollBar)
                    {
                        scrollBar->setAttribute(Qt::WA_OpaquePaintEvent, false);
                        AZStd::invoke(callback, scrollBar.data());
                    }
                }
            }
        }

        void scrollAreaDestroyed(QObject* scrollArea)
        {
            m_widgets.remove(scrollArea);
        }
    };

    void ScrollBar::applyDarkStyle(QAbstractScrollArea* scrollArea)
    {
        // Note: ScrollBars default to light
        Style::addClass(scrollArea, ScrollBarProperties::DarkStyleClass);
        StyleManager::repolishStyleSheet(scrollArea);
    }

    void ScrollBar::applyLightStyle(QAbstractScrollArea* scrollArea)
    {
        // Note: ScrollBars default to light
        Style::removeClass(scrollArea, ScrollBarProperties::DarkStyleClass);
        StyleManager::repolishStyleSheet(scrollArea);
    }

    void ScrollBar::setBackgroundColor(QAbstractScrollArea* scrollArea, const QColor& color)
    {
        if (!scrollArea)
        {
            return;
        }

        if (!StyleManager::stylesheetsDisabled())
        {
            scrollArea->setStyleSheet(QString("background: %1").arg(color.name(QColor::HexRgb)));
            return;
        }

        scrollArea->setProperty(ScrollBarProperties::BackgroundColor, color);
        applyBackgroundColor(scrollArea);
    }

    void ScrollBar::applyBackgroundColor(QWidget* widget)
    {
        if (!qobject_cast<QAbstractScrollArea*>(widget))
        {
            return;
        }

        const QVariant background = widget->property(ScrollBarProperties::BackgroundColor);
        if (!background.isValid())
        {
            return;
        }

        // The inline sheet's `background` filled the area, its viewport and the lanes of its scroll bars, in every state of the widget:
        // the viewport fills with Base, the area itself (under the scroll bars) with Window. A deeper sheet than the module's, it
        // outranked the module's fill of the same widget (the Python Console output's `background-color: #222222`, Editor.qss:196).
        const QColor color = background.value<QColor>();
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Base, color);
        palette.setColor(QPalette::Window, color);
        widget->setPalette(palette);

        // A text edit the flattened style draws as a field (Style::polish gave it a styled background and took its autofill) paints
        // this colour inside its margin at PE_Widget, whether the field is the style's or sits in the box of a module's element. The
        // legacy widget had no autofill either (its sheet's background filled the border box), so the margin around its hover or
        // focus ring, or above its box, shows what is behind it.
        const bool flattenedTextEditField = StyleManager::stylesheetsDisabled() && widget->testAttribute(Qt::WA_StyledBackground)
            && (widget->inherits("QTextEdit") || widget->inherits("QPlainTextEdit"));
        if (!flattenedTextEditField)
        {
            widget->setAutoFillBackground(true);
        }
    }

    QPointer<ScrollBarWatcher> ScrollBar::s_scrollBarWatcher = nullptr;
    unsigned int ScrollBar::s_watcherReferenceCount = 0;

    ScrollBar::Config ScrollBar::loadConfig(QSettings& settings)
    {
        Q_UNUSED(settings);
        Config config = ScrollBar::defaultConfig();

        ConfigHelpers::read<AzQtComponents::ScrollBar::ScrollBarMode>(settings, QStringLiteral("DefaultMode"), config.defaultMode);

        return config;
    }

    ScrollBar::Config ScrollBar::defaultConfig()
    {
        Config config;

        config.defaultMode = ScrollBarMode::AlwaysShow;

        return config;
    }

    void ScrollBar::setDisplayMode(QAbstractScrollArea* scrollArea, ScrollBarMode mode)
    {
        if (!scrollArea)
        {
            return;
        }

        scrollArea->setProperty(g_scrollBarModeProperty, QVariant::fromValue(mode));
    }

    void ScrollBar::initializeWatcher()
    {
        if (!s_scrollBarWatcher)
        {
            Q_ASSERT(s_watcherReferenceCount == 0);
            s_scrollBarWatcher = new ScrollBarWatcher;
        }

        ++s_watcherReferenceCount;
    }

    void ScrollBar::uninitializeWatcher()
    {
        Q_ASSERT(!s_scrollBarWatcher.isNull());
        Q_ASSERT(s_watcherReferenceCount > 0);

        --s_watcherReferenceCount;

        if (s_watcherReferenceCount == 0)
        {
            delete s_scrollBarWatcher;
            s_scrollBarWatcher = nullptr;
        }
    }

    bool ScrollBar::polish(Style* style, QWidget* widget, const ScrollBar::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        s_scrollBarWatcher->m_config = config;

        return s_scrollBarWatcher->install(widget);
    }

    bool ScrollBar::unpolish(Style* style, QWidget* widget, const ScrollBar::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        return s_scrollBarWatcher->uninstall(widget);
    }

    bool ScrollBar::drawScrollBar(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(config);

        QStyle* styleSheetStyle = style->baseStyle();
        if (styleSheetStyle)
        {
            styleSheetStyle->drawComplexControl(QStyle::CC_ScrollBar, option, painter, widget);
            return true;
        }

        return false;
    }

} // namespace AzQtComponents
