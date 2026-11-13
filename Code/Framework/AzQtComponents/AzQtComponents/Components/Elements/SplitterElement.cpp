/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/SplitterElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>

#include <QCoreApplication>
#include <QEvent>
#include <QPainter>
#include <QSplitter>
#include <QStyleOption>
#include <QWidget>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace SplitterElementInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        // Splitter.qss:10 $fill.scroll-handle.idle.BG. TOKENIZE: SplitterHandleFill
        const QColor HandleFill = Foundations::BG;
        // Splitter.qss:23 $fill.scroll-handle.hover. TOKENIZE: SplitterHandleHoverFill
        const QColor HandleHoverFill(0x1E, 0x70, 0xEB);
        // Splitter.qss:18 $size.scroll-handle.idle.height. TOKENIZE: SplitterHandleThickness
        constexpr int HandleThickness = 1;

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! `QSplitter::handle`, `::handle:vertical` and `::handle:hover` (Splitter.qss). The `:horizontal` rule's width (Splitter.qss:14)
        //! is never read: the width of a handle is asked with no option, in the state of a vertical one.
        const SplitterElement GlobalHandle{
            .m_name = "Global.Splitter.Handle",
            .m_thickness = HandleThickness,
            .m_fill = HandleFill,
            .m_hoverFill = HandleHoverFill,
        };

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        ElementRegistry<SplitterElement>& registry()
        {
            static ElementRegistry<SplitterElement> rows("_o3de_splitterElement");
            return rows;
        }

        //! The element of a splitter: a module's first, then the Global one
        const SplitterElement& findElement(const QWidget* widget)
        {
            const SplitterElement* element = registry().find(widget);
            return element ? *element : GlobalHandle;
        }
    } // namespace SplitterElementInternal

    // ----------------------------------------------------------------------------------
    // Drawing
    // ----------------------------------------------------------------------------------

    QColor SplitterElement::fillOf(const QStyleOption* option) const
    {
        // QStyleSheetStyle::pseudoClass: :hover needs the widget enabled as well as under the pointer
        const bool hover = option->state.testFlag(QStyle::State_Enabled) && option->state.testFlag(QStyle::State_MouseOver);
        return (hover && m_hoverFill.isValid()) ? m_hoverFill : m_fill;
    }

    bool SplitterElement::draw(QPainter* painter, const QStyleOption* option) const
    {
        // QStyleSheetStyle::drawControl, CE_Splitter: a rule with a background draws it, and the base style does not
        const QColor fill = fillOf(option);
        if (!fill.isValid())
        {
            return false;
        }
        if (fill.alpha() > 0)
        {
            painter->fillRect(option->rect, fill);
        }
        return true;
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    void SplitterElement::registerElement(
        const SplitterElement* element, const QMetaObject* widgetClass, ElementMatcher matcher, ElementScope scope)
    {
        SplitterElementInternal::registry().add(element, widgetClass, matcher, scope);
        resolvePolished(widgetClass, [](QWidget* widget)
            {
                resolve(widget);
                widget->update();
            });
    }

    void SplitterElement::resolve(QWidget* widget)
    {
        using namespace SplitterElementInternal;
        if (!qobject_cast<QSplitter*>(widget))
        {
            return;
        }

        const std::optional<const SplitterElement*> before = registry().kept(widget);
        const SplitterElement& element = findElement(widget);
        registry().keep(widget, &element);

        // QSplitter reads the width of its handles when its style changes (QSplitter::changeEvent): the sheet's change did so
        if (before && *before && (*before)->m_thickness != element.m_thickness)
        {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(widget, &styleChange);
        }
    }

    const SplitterElement& SplitterElement::forWidget(const QWidget* widget)
    {
        using namespace SplitterElementInternal;
        if (!qobject_cast<const QSplitter*>(widget))
        {
            return GlobalHandle;
        }
        const std::optional<const SplitterElement*> kept = registry().kept(widget);
        return (kept && *kept) ? **kept : findElement(widget);
    }
} // namespace AzQtComponents
