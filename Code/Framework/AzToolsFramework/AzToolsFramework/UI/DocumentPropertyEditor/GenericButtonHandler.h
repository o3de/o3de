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
    class GenericButtonHandler : public PropertyHandlerWidget<QToolButton>
    {
    public:
        GenericButtonHandler();

        virtual void SetValueFromDom(const AZ::Dom::Value& node) override;
        virtual bool ResetToDefaults() override
        {
            m_node.SetNull();
            return true;
        }

        static constexpr const AZStd::string_view GetHandlerName()
        {
            return AZ::DocumentPropertyEditor::Nodes::GenericButton::Name;
        }

    protected:
        AZ::Dom::Value m_node;

        virtual void OnClicked();
    };
} // namespace AzToolsFramework
