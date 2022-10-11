/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyHandlerWidget.h>
#include <QToolButton>

namespace AzToolsFramework
{
    class ContainerActionButtonHandler : public PropertyHandlerWidget<QToolButton>
    {
    public:
        ContainerActionButtonHandler();

        void SetValueFromDom(const AZ::Dom::Value& node);

        static constexpr const AZStd::string_view GetHandlerName()
        {
            return AZ::DocumentPropertyEditor::Nodes::ContainerActionButton::Name;
        }

    private:
        AZ::Dom::Value m_node;
    };
} // namespace AzToolsFramework
