/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/LightingChannel/LightingChannelConfiguration.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        void LightingChannelConfiguration::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<LightingChannelConfiguration>()
                    ->Version(0)
                    ->Field("lightingChannelFlags", &LightingChannelConfiguration::m_lightingChannelFlags)
                    ;
                
                if (auto* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<AZ::Render::LightingChannelConfiguration>("Lighting Channel Config", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &AZ::Render::LightingChannelConfiguration::m_lightingChannelFlags,
                            "Lighting Channels", "Lights can only shine the objects in the same lighting channel with the light.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->Attribute(AZ::Edit::Attributes::ContainerReorderAllow, false)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                            ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &LightingChannelConfiguration::GetLabelForIndex)
                     ;
                }
            }
        }

        void LightingChannelConfiguration::SetLightingChannelMask(const uint32_t mask)
        {
            for (uint32_t index = 0; index < m_lightingChannelFlags.size(); ++index)
            {
                m_lightingChannelFlags[index] = (static_cast<bool>(mask >> index) & 0x01);
            }
        }

        uint32_t LightingChannelConfiguration::GetLightingChannelMask() const
        {
            uint32_t mask = 0;
            for (uint32_t index = 0; index < m_lightingChannelFlags.size(); ++index)
            {
                mask |= (static_cast<uint32_t>(m_lightingChannelFlags[index]) << (index));
            }
            return mask;
        }

        AZStd::string LightingChannelConfiguration::GetLabelForIndex(int index) const
        {
            return AZStd::string::format("Lighting Channel %d", index);
        }
    } // namespace Render
} // namespace AZ
