/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/DOM/DomValue.h>
#include <AzCore/DOM/DomUtils.h>
#include <QWidget>
#endif //defined(Q_MOC_RUN)

namespace AzToolsFramework
{
    class PropertyHandlerWidgetInterface
    {
    public:
        QWidget* GetWidget()
        {
            return dynamic_cast<QWidget*>(this);
        }
        virtual void SetValueFromDom(const AZ::Dom::Value& value) = 0;

        static bool CanHandleValue([[maybe_unused]] const AZ::Dom::Value& value)
        {
            return true;
        }

        static constexpr AZStd::string_view HandlerName = "<undefined handler name>";

        //signal
        virtual void ValueChangedByUser(const AZ::Dom::Value& newValue) = 0;
    };

    template <typename BaseWidget>
    class PropertyHandlerWidget : public BaseWidget
    {
        Q_OBJECT

    public:
        PropertyHandlerWidget(QWidget* parent = nullptr)
            : BaseWidget(parent)
        {
        }

    signals:
        void ValueChangedByUser(const AZ::Dom::Value& newValue) override;
    };

    template <typename T>
    class TypedPropertyHandlerWidget : public PropertyHandlerWidget
    {
        Q_OBJECT

    public:
        TypedPropertyHandlerWidget(QWidget* parent)
            : PropertyHandlerWidget(parent)
        {
        }

        virtual void SetValue(T&& value) = 0;
        void SetValueFromDom(const AZ::Dom::Value& value) override
        {
            //...
        }

        static bool CanHandleValue(const AZ::Dom::Value& value)
        {
            return ; // ... 
        }

    signals:
        void ValueChangedByUser(T&& value) = 0;
    };
}
