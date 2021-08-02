/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/any.h>

#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    class NodeConfiguration
    {
    public:
        AZ_TYPE_INFO(NodeConfiguration, "{7DC45DA7-EEE1-4FCF-93F0-2D3F8A2E9DA9}");

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<NodeConfiguration>()
                    ->Version(3)
                    ->Field("Tooltip", &NodeConfiguration::m_tooltip)
                    ->Field("ShowInOutliner", &NodeConfiguration::m_showInOutliner)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<NodeConfiguration>("Configuration", "The Node's configuration data")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "Node's configuration class attributes")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &NodeConfiguration::m_tooltip, "Tooltip", "Tooltip explaining node functionality")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ;
                }
            }
        }

        // Default configuration
        NodeConfiguration()
            : m_tooltip("")
            , m_showInOutliner(true)
        {}

        void SetTooltip(const AZStd::string& tooltip) { m_tooltip = tooltip; }
        const AZStd::string GetTooltip() const { return m_tooltip; }

        void SetShowInOutliner(bool show) { m_showInOutliner = show; }
        bool GetShowInOutliner() const { return m_showInOutliner; }

    protected:
        AZStd::string m_tooltip;

        bool m_showInOutliner;
    };
}
