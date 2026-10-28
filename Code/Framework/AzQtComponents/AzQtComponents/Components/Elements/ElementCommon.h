/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QHash>
#include <QMetaObject>
#include <QVariant>
#include <QVector>
#include <QWidget>

#include <optional>

class QColor;
class QPalette;

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // Registration
    // ----------------------------------------------------------------------------------

    //! Who owns a registration: a module's rows are checked before the Global ones
    enum class ElementScope
    {
        Module,
        Global,
    };

    //! Picks the widgets a registration covers among those of its widget class; the Selectors build one
    using ElementMatcher = bool (*)(const QWidget* widget);

    //! The registrations of one element family, and the dynamic property a widget keeps its element in. Rows are kept by the
    //! class of the widgets they cover. Of the rows that match a widget the first wins: a module's before a Global one, each in
    //! the order registered.
    template<typename Element>
    class ElementRegistry
    {
    public:
        explicit ElementRegistry(const char* property)
            : m_property(property)
        {
        }

        void add(const Element* element, const QMetaObject* widgetClass, ElementMatcher matcher, ElementScope scope)
        {
            const Row row{ element, matcher, scope == ElementScope::Global, m_count++ };
            QVector<Row>& bucket = m_rows[widgetClass];
            int at = 0;
            while (at < bucket.size() && !row.before(bucket[at]))
            {
                ++at;
            }
            bucket.insert(at, row);
        }

        //! The element of the first row that matches the widget; null: no row does
        const Element* find(const QWidget* widget) const
        {
            // A widget looks at the rows of its own class chain only, whatever the number of rows
            const Row* winner = nullptr;
            for (const QMetaObject* meta = widget->metaObject(); meta; meta = meta->superClass())
            {
                const auto bucket = m_rows.constFind(meta);
                if (bucket == m_rows.constEnd())
                {
                    continue;
                }
                for (const Row& row : bucket.value())
                {
                    if (winner && !row.before(*winner))
                    {
                        break;
                    }
                    if (row.m_matcher(widget))
                    {
                        winner = &row;
                        break;
                    }
                }
            }
            return winner ? winner->m_element : nullptr;
        }

        //! True if a row covers the widget's class or a base of it, whatever its matcher says
        bool covers(const QWidget* widget) const
        {
            for (const QMetaObject* meta = widget->metaObject(); meta; meta = meta->superClass())
            {
                if (m_rows.contains(meta))
                {
                    return true;
                }
            }
            return false;
        }

        bool isEmpty() const
        {
            return m_rows.isEmpty();
        }

        //! Keeps the element on the widget, a null one too
        void keep(QWidget* widget, const Element* element) const
        {
            widget->setProperty(m_property, QVariant::fromValue(reinterpret_cast<quintptr>(element)));
        }

        void forget(QWidget* widget) const
        {
            if (widget->property(m_property).isValid())
            {
                widget->setProperty(m_property, QVariant());
            }
        }

        //! The element kept on the widget; none: the widget keeps nothing
        std::optional<const Element*> kept(const QWidget* widget) const
        {
            const QVariant stored = widget->property(m_property);
            if (!stored.isValid())
            {
                return std::nullopt;
            }
            return reinterpret_cast<const Element*>(stored.value<quintptr>());
        }

    private:
        struct Row
        {
            const Element* m_element;
            ElementMatcher m_matcher;
            bool m_global;
            int m_order;

            //! Of two matching rows the first wins: a module's, then the one registered first
            bool before(const Row& other) const
            {
                return m_global != other.m_global ? other.m_global : m_order < other.m_order;
            }
        };

        QHash<const QMetaObject*, QVector<Row>> m_rows;
        int m_count = 0;
        const char* m_property;
    };

    //! A module may set up its styles after it built some windows: in the flattened mode, `again` resolves each widget of
    //! widgetClass polished already
    void resolvePolished(const QMetaObject* widgetClass, void (*again)(QWidget* widget));

    // ----------------------------------------------------------------------------------
    // Selectors
    // ----------------------------------------------------------------------------------

    //! The tests of the legacy style sheets' selectors, for matchers
    namespace Selectors
    {
        AZ_QT_COMPONENTS_API bool named(const QWidget* widget, const char* objectName);                     //!< #name
        AZ_QT_COMPONENTS_API bool tagged(const QWidget* widget, const char* tag);                           //!< .tag
        AZ_QT_COMPONENTS_API bool hasProperty(const QWidget* widget, const char* name, const char* value);  //!< [name="value"]
        AZ_QT_COMPONENTS_API bool childOf(const QWidget* widget, const char* parentName);                   //!< #name > W
        AZ_QT_COMPONENTS_API bool childOfClass(const QWidget* widget, const char* parentClass);             //!< Class > W
        AZ_QT_COMPONENTS_API bool within(const QWidget* widget, const char* ancestorName);                  //!< #name W
        AZ_QT_COMPONENTS_API bool withinClass(const QWidget* widget, const char* ancestorClass);            //!< Class W
    } // namespace Selectors

    // ----------------------------------------------------------------------------------
    // Palette
    // ----------------------------------------------------------------------------------

    //! The shades the base styles draw a bevel in, derived from a solid fill
    void setShades(QPalette& palette, const QColor& fill);
} // namespace AzQtComponents
