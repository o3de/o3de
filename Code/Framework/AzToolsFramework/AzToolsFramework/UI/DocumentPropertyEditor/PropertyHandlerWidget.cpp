/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyHandlerWidget.h>

namespace AzToolsFramework
{
    void PropertyHandlerWidgetInterface::SetValueChangedHandler(ValueChangedHandler handler)
    {
        m_handler = AZStd::move(handler);
    }

    void PropertyHandlerWidgetInterface::ValueChangedByUser(const AZ::Dom::Value& newValue, ValueChangeType changeType)
    {
        if (m_handler)
        {
            m_handler(newValue, changeType);
        }
    }

    QWidget* PropertyHandlerWidgetInterface::GetFirstInTabOrder()
    {
        return GetWidget();
    }

    QWidget* PropertyHandlerWidgetInterface::GetLastInTabOrder()
    {
        return GetWidget();
    }
} // namespace AzToolsFramework
