/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyHandlerWidget.h>

#include <AzCore/DOM/DomValue.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>

namespace AzToolsFramework
{
    QWidget* PropertyHandlerWidgetInterface::GetFirstInTabOrder()
    {
        return GetWidget();
    }

    QWidget* PropertyHandlerWidgetInterface::GetLastInTabOrder()
    {
        return GetWidget();
    }

    void PropertyHandlerWidgetInterface::SetValueFromDom_Internal(const AZ::Dom::Value& node, AzToolsFramework::DocumentPropertyEditor* owningDPE)
    {
        SetValueFromDom(node);
        if (owningDPE)
        {
            owningDPE->AddDirtyHandler(this);
        }
    }

} // namespace AzToolsFramework
