/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QApplication>
#include <QColor>
#include <QPalette>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace ElementCommonInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        constexpr int LightShadeFactor = 115;               // QRenderRule::configurePalette $line.shade.light
        constexpr int MidlightShadeFactor = 107;            // QRenderRule::configurePalette $line.shade.midlight
        constexpr int DarkShadeFactor = 150;                // QRenderRule::configurePalette $line.shade.dark
        constexpr int ShadowShadeFactor = 300;              // QRenderRule::configurePalette $line.shade.shadow
    } // namespace ElementCommonInternal

    // ----------------------------------------------------------------------------------
    // Registration
    // ----------------------------------------------------------------------------------

    void resolvePolished(const QMetaObject* widgetClass, void (*again)(QWidget* widget))
    {
        if (!StyleManager::stylesheetsDisabled())
        {
            return;
        }
        const QWidgetList widgets = QApplication::allWidgets();
        for (QWidget* widget : widgets)
        {
            if (widget->testAttribute(Qt::WA_WState_Polished) && widget->metaObject()->inherits(widgetClass))
            {
                again(widget);
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // Selectors
    // ----------------------------------------------------------------------------------

    namespace Selectors
    {
        bool named(const QWidget* widget, const char* objectName)
        {
            return widget->objectName() == QLatin1String(objectName);
        }

        bool tagged(const QWidget* widget, const char* tag)
        {
            // A whole word of the class property, compared with case as QCss::StyleSelector::basicSelectorMatches does
            // (Style::hasClass compares without case)
            const QVariant tags = widget->property("class");
            return tags.isValid()
                && tags.toString().split(QLatin1Char(' '), Qt::SkipEmptyParts).contains(QLatin1String(tag), Qt::CaseSensitive);
        }

        bool hasProperty(const QWidget* widget, const char* name, const char* value)
        {
            // The property read as text, as QStyleSheetStyleSelector::attributeValue reads it (a bool is "true" or "false")
            return widget->property(name).toString() == QLatin1String(value);
        }

        bool childOf(const QWidget* widget, const char* parentName)
        {
            const QWidget* parent = widget->parentWidget();
            return parent && named(parent, parentName);
        }

        bool childOfClass(const QWidget* widget, const char* parentClass)
        {
            const QWidget* parent = widget->parentWidget();
            return parent && parent->inherits(parentClass);
        }

        bool within(const QWidget* widget, const char* ancestorName)
        {
            for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                if (named(ancestor, ancestorName))
                {
                    return true;
                }
            }
            return false;
        }

        bool withinClass(const QWidget* widget, const char* ancestorClass)
        {
            for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                if (ancestor->inherits(ancestorClass))
                {
                    return true;
                }
            }
            return false;
        }
    } // namespace Selectors

    // ----------------------------------------------------------------------------------
    // Palette
    // ----------------------------------------------------------------------------------

    void setShades(QPalette& palette, const QColor& fill)
    {
        using namespace ElementCommonInternal;
        palette.setColor(QPalette::Light, fill.lighter(LightShadeFactor));
        palette.setColor(QPalette::Midlight, fill.lighter(MidlightShadeFactor));
        palette.setColor(QPalette::Dark, fill.darker(DarkShadeFactor));
        palette.setColor(QPalette::Shadow, fill.darker(ShadowShadeFactor));
    }
} // namespace AzQtComponents
