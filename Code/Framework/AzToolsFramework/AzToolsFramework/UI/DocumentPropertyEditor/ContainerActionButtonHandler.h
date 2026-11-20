/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/GenericButtonHandler.h>

namespace AzToolsFramework
{
    class AZTF_API ContainerActionButtonHandler : public GenericButtonHandler
    {
    public:
        ContainerActionButtonHandler();

        void SetValueFromDom(const AZ::Dom::Value& node) override;
        virtual bool ResetToDefaults() override;
        void RefreshUI() override;

        static constexpr const AZStd::string_view GetHandlerName()
        {
            return AZ::DocumentPropertyEditor::Nodes::ContainerActionButton::Name;
        }

    protected:
        AZ::DocumentPropertyEditor::Nodes::ContainerAction m_action = AZ::DocumentPropertyEditor::Nodes::ContainerAction::None;
        AZ::DocumentPropertyEditor::Nodes::ContainerAction m_priorAction = AZ::DocumentPropertyEditor::Nodes::ContainerAction::None;

        void OnClicked() override;
    };
} // namespace AzToolsFramework
