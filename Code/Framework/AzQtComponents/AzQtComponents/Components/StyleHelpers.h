/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
#pragma once

#include <AzQtComponents/Components/StyleManager.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QtWidgets/private/qstylesheetstyle_p.h>
AZ_POP_DISABLE_WARNING

namespace AzQtComponents
{
    namespace StyleHelpers
    {
        /* StyleHelpers::repolishWhenPropertyChanges is used to ensure that style sheet rules that
         * depend on the property of a widget are correctly updated when that property changes.
         *
         * For example, given a Widget with a QLabel as a child, and property called 'drawSimple'
         * one can declaratively change the pixmap of the label using the following stylesheet:
         *
         *      Widget QLabel#icon
         *      {
         *          qproperty-pixmap: url(:/normal-pixmap.png);
         *      }
         *
         *      Widget[drawSimple="true"] QLabel#icon
         *      {
         *          qproperty-pixmap: url(:/simple-pixmap.png);
         *      }
         *
         * If the NOTIFY signal of the property change is drawSimpleChanged, make the following call
         * in the Widget constructor:
         *
         *      StyleHelpers::repolishWhenPropertyChanges(this, &Widget::drawSimpleChanged);
         *
         * See Example::Widget in StyleSheetPage.h and ExampleWidget.qss for a working example.
         */
        template <typename T, typename ...Args>
        void repolishWhenPropertyChanges(T* widget, void (T::*signal)(Args...))
        {
#if !defined(AZ_PLATFORM_LINUX)

            QObject::connect(widget, signal, widget, [widget]() {
                // Prevent asserts in Unit Tests
                if (!StyleManager::isInstanced())
                {
                    return;
                }

                if (auto styleSheet = StyleManager::styleSheetStyle(widget))
                {
                    // For the widget and each of its children, QStyleSheetStyle::repolish clears
                    // the existing render rules, polishes the widget and sends it a StyleChange
                    // event. This ensure that both render rules which depend on properties, and
                    // properties that are set in style sheets via qproperty- are correctly updated.
                    styleSheet->repolish(widget);
                }
            });

#endif // !defined(AZ_PLATFORM_LINUX)
        }

        /* StyleHelpers::findParent<T> is an utility function to find recursively a parent object of
         * type T. If none is found, the function will return a null pointer.
         */
        template <typename T>
        static T* findParent(const QObject* obj)
        {
            if (!obj)
            {
                return nullptr;
            }

            QObject* parent = obj->parent();

            if (auto p = qobject_cast<T*>(parent))
            {
                return p;
            }

            return findParent<T>(parent);
        }

    } // namespace StyleHelpers
} // namespace AzQtComponents
