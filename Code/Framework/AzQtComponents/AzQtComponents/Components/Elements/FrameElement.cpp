/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>

#include <QAbstractScrollArea>
#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QStyleOption>
#include <QWidget>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace FrameElementInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        const QColor PanelFill = Foundations::Mid;          // BaseStyleSheet.qss:47 $fill.surface.idle.Mid
        const QColor SeparatorLine(0x99, 0x99, 0x99);       // Card.qss:262 $text.surface.idle.Alt5

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! A frame no rule gave a background: a scroll area's, or a QFrame with a class tag no element names
        const FrameElement ClearFrame{ .m_name = "Global.Frame.Clear" };

        //! `.QFrame[frameShape]`: the dividers, boxes and panels a QFrame draws itself
        const FrameElement PanelFrame{ .m_name = "Global.Frame.Panel", .m_fill = PanelFill, .m_fillShapes = FrameElement::PanelShapes };

        //! `.separator`: the separator line of a Card
        const FrameElement SeparatorFrame{ .m_name = "Global.Frame.Separator", .m_foreground = SeparatorLine };

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        ElementRegistry<FrameElement>& registry()
        {
            static ElementRegistry<FrameElement> rows("_o3de_frameElement");
            return rows;
        }

        //! A QFrame itself, as `.QFrame` matches it: by class name (a subclass without Q_OBJECT and a QFrame made in Python count)
        bool isExactFrame(const QWidget* widget)
        {
            const QMetaObject* metaObject = widget->metaObject();
            return metaObject == &QFrame::staticMetaObject || qstrcmp(metaObject->className(), "QFrame") == 0;
        }

        //! The widgets that may have an element: a QFrame itself, a scroll area, or a class a module registered
        bool isCandidate(const QWidget* widget)
        {
            return isExactFrame(widget) || qobject_cast<const QAbstractScrollArea*>(widget) || registry().covers(widget);
        }

        //! The Global element of a candidate; none for a class only a module covers
        const FrameElement* globalElement(const QWidget* widget)
        {
            if (isExactFrame(widget))
            {
                // A class tag replaces the class name `.QFrame` matches
                if (!widget->property("class").isValid())
                {
                    return &PanelFrame;
                }
                return Selectors::tagged(widget, "separator") ? &SeparatorFrame : &ClearFrame;
            }
            return qobject_cast<const QAbstractScrollArea*>(widget) ? &ClearFrame : nullptr;
        }

        //! The element of a candidate: a module's first, then the Global one
        const FrameElement* findElement(const QWidget* widget)
        {
            const FrameElement* element = registry().find(widget);
            return element ? element : globalElement(widget);
        }

        //! The background of a frame of this shape: the element's on the shapes it covers, else none
        QColor fillOf(const FrameElement& element, int frameShape)
        {
            return ((element.m_fillShapes >> frameShape) & 1u) ? element.m_fill : Foundations::Surface::Fill;
        }
    } // namespace FrameElementInternal

    // ----------------------------------------------------------------------------------
    // Drawing
    // ----------------------------------------------------------------------------------

    void FrameElement::drawBackground(QStyleOptionFrame* option, QPainter* painter, const QWidget* widget) const
    {
        // Under an inline style sheet, the background and the color the sheet's own rule gave the widget stay
        const bool throughSheet = widget->style()->inherits("QStyleSheetStyle");

        if (!(throughSheet && widget->testAttribute(Qt::WA_StyledBackground)))
        {
            // The background over the whole widget, then the frame's palette: Window, Base and Button are the background and the
            // shades derive from it; setColor sets every color group
            const QColor fill = FrameElementInternal::fillOf(*this, option->frameShape);
            if (fill.alpha() > 0)
            {
                painter->fillRect(widget->rect(), fill);
            }
            QPalette& palette = option->palette;
            palette.setColor(QPalette::Base, fill);
            palette.setColor(QPalette::Button, fill);
            palette.setColor(QPalette::Window, fill);
            setShades(palette, fill);
        }

        // The lines of a Plain frame draw in WindowText
        const bool sheetColor = throughSheet
            && widget->palette().color(QPalette::Active, QPalette::WindowText)
                != QApplication::palette(widget).color(QPalette::Active, QPalette::WindowText);
        if (!sheetColor)
        {
            option->palette.setColor(QPalette::WindowText, m_foreground);
            option->palette.setColor(QPalette::Text, m_foreground);
        }
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    void FrameElement::registerElement(
        const FrameElement* element, const QMetaObject* widgetClass, ElementMatcher matcher, ElementScope scope)
    {
        FrameElementInternal::registry().add(element, widgetClass, matcher, scope);
        resolvePolished(widgetClass, [](QWidget* widget)
            {
                resolve(widget);
                widget->update();
            });
    }

    void FrameElement::resolve(QWidget* widget)
    {
        using namespace FrameElementInternal;
        if (isCandidate(widget))
        {
            // A null element is kept too, so a widget a module's matcher rejected is not asked again on every paint
            registry().keep(widget, findElement(widget));
        }
    }

    const FrameElement* FrameElement::forWidget(const QWidget* widget)
    {
        using namespace FrameElementInternal;
        if (!widget || !isCandidate(widget))
        {
            return nullptr;
        }
        const std::optional<const FrameElement*> kept = registry().kept(widget);
        return kept ? *kept : findElement(widget);
    }
} // namespace AzQtComponents
