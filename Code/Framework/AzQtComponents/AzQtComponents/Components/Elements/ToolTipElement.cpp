/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/ToolTipElement.h>

#include <QAbstractScrollArea>
#include <QApplication>
#include <QBrush>
#include <QPalette>
#include <QPointer>
#include <QTimer>
#include <QToolTip>
#include <QVariant>
#include <QWidget>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace ToolTipElementInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        constexpr const char* TipClassName = "QTipLabel";                   // qtooltip.cpp: Qt's tooltip label
        constexpr const char* OwnerProperty = "_o3de_toolTipOwner";         // the widget a tip claimed
        constexpr const char* LookProperty = "_o3de_toolTipLook";           // the look of that widget when the tip claimed it

        // ------------------------------------------------------------------------------
        // State
        // ------------------------------------------------------------------------------

        ElementRegistry<ToolTipElement>& registry()
        {
            static ElementRegistry<ToolTipElement> rows("_o3de_toolTipElement");
            return rows;
        }

        //! The widget the help event being handled was delivered to: Qt makes the tip while it handles the event, so a tip created
        //! then is the event widget's. Held weakly, and cleared when the event loop is next reached.
        QPointer<QWidget>& pendingOwner()
        {
            static QPointer<QWidget> owner;
            return owner;
        }

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        //! Every property of the look has a declaration: no widget above can add one
        bool isComplete(const ToolTipElement::Look& look)
        {
            return look.m_fill.isValid() && look.m_text;
        }

        //! The widget that shows the tip for a help event delivered to `receiver`: the area for its viewport
        //! (QAbstractScrollArea::viewportEvent hands a ToolTip to the area, whose QWidget::event shows the tip; an item view's
        //! delegate shows it for the view)
        QWidget* ownerOfReceiver(QWidget* receiver)
        {
            if (auto* area = qobject_cast<QAbstractScrollArea*>(receiver->parentWidget()))
            {
                if (area->viewport() == receiver)
                {
                    return area;
                }
            }
            return receiver;
        }
    } // namespace ToolTipElementInternal

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    void ToolTipElement::registerElement(
        const ToolTipElement* element, const QMetaObject* ownerClass, ElementMatcher matcher, ElementScope scope)
    {
        ToolTipElementInternal::registry().add(element, ownerClass, matcher, scope);
    }

    ToolTipElement::Look ToolTipElement::lookFor(const QWidget* owner)
    {
        Look look;
        if (ToolTipElementInternal::registry().isEmpty())
        {
            return look;
        }

        // The sheets of the owner and of each widget above it: the nearest declaration of a property wins
        for (const QWidget* widget = owner; widget && !ToolTipElementInternal::isComplete(look); widget = widget->parentWidget())
        {
            const ToolTipElement* element = ToolTipElementInternal::registry().find(widget);
            if (!element)
            {
                continue;
            }
            if (!look.m_fill.isValid() && element->m_fill.isValid())
            {
                look.m_fill = element->m_fill;
            }
            if (!look.m_text && element->m_text)
            {
                look.m_text = element->m_text;
            }
        }
        return look;
    }

    // ----------------------------------------------------------------------------------
    // The widget a tip is shown for
    // ----------------------------------------------------------------------------------

    bool ToolTipElement::isToolTip(const QWidget* widget)
    {
        return widget && widget->isWindow() && qstrcmp(widget->metaObject()->className(), ToolTipElementInternal::TipClassName) == 0;
    }

    QWidget* ToolTipElement::noteHelpEvent(QWidget* receiver)
    {
        using namespace ToolTipElementInternal;

        QWidget* owner = ownerOfReceiver(receiver);
        pendingOwner() = owner;

        // The tip is made while the event is handled: once the event loop is reached again the note is stale. A help event comes when
        // the pointer rests, a few times a second at most
        QTimer::singleShot(0, qApp, [] { pendingOwner().clear(); });
        return owner;
    }

    QWidget* ToolTipElement::visibleTip()
    {
        // Qt keeps one tip at a time (a new QTipLabel deletes the one before it, qtooltip.cpp): with none on screen, no window is looked at
        if (!QToolTip::isVisible())
        {
            return nullptr;
        }

        const QWidgetList windows = QApplication::topLevelWidgets();
        for (QWidget* window : windows)
        {
            if (window->isVisible() && isToolTip(window))
            {
                return window;
            }
        }
        return nullptr;
    }

    void ToolTipElement::claim(QWidget* tip)
    {
        using namespace ToolTipElementInternal;

        if (!isToolTip(tip))
        {
            return;
        }

        // Qt makes the tip a child window of its widget where it can (not on Windows)
        QWidget* owner = tip->parentWidget() ? tip->parentWidget() : pendingOwner().data();
        if (owner)
        {
            tip->setProperty(OwnerProperty, QVariant::fromValue(QPointer<QWidget>(owner)));
            tip->setProperty(LookProperty, QVariant::fromValue(lookFor(owner)));
        }
    }

    const QWidget* ToolTipElement::ownerOf(const QWidget* tip)
    {
        using namespace ToolTipElementInternal;

        const QVariant claimed = tip->property(OwnerProperty);
        if (claimed.isValid())
        {
            return claimed.value<QPointer<QWidget>>().data();
        }
        return tip->parentWidget() ? tip->parentWidget() : pendingOwner().data();
    }

    ToolTipElement::Look ToolTipElement::lookOf(const QWidget* tip)
    {
        const QVariant made = tip->property(ToolTipElementInternal::LookProperty);
        if (made.isValid())
        {
            return made.value<Look>();
        }
        const QWidget* owner = ownerOf(tip);
        return owner ? lookFor(owner) : Look();
    }

    const TextElement* ToolTipElement::textOf(const QWidget* tip)
    {
        return lookOf(tip).m_text;
    }

    void ToolTipElement::applyPalette(QWidget* tip)
    {
        if (!isToolTip(tip))
        {
            return;
        }
        const QColor fill = lookOf(tip).m_fill;
        if (!fill.isValid())
        {
            return;
        }

        // QRenderRule::configurePalette: the background of a rule is the Base, Button, Window and background role of the widget (a
        // tip's is ToolTipBase), in every colour group
        QPalette palette = tip->palette();
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            for (const auto role : { QPalette::Base, QPalette::Button, QPalette::Window, QPalette::ToolTipBase })
            {
                palette.setBrush(group, role, QBrush(fill));
            }
        }
        tip->setPalette(palette);
    }
} // namespace AzQtComponents
