/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/LightingChannel/LightingChannelConfiguration.h>

namespace AZ
{
    namespace Render
    {
        LightingChannelConfiguration::LightingChannelConfiguration()
        {
            m_lightingChannelFlags = {false};
            m_lightingChannelFlags[0] = true;
            UpdateLightingChannelMask();
        }

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
                    editContext->Class<AZ::Render::LightingChannelConfiguration>("Lighting Channels Config", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &AZ::Render::LightingChannelConfiguration::m_lightingChannelFlags,
                                        "Lighting Channels", "Lights can only shine the objects in the same lighting channel with the light.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ;
                }
            }
        }

        void LightingChannelConfiguration::UpdateLightingChannelMask()
        {
            AZ::u32 index = 0;
            m_mask = 0;
            AZStd::for_each(m_lightingChannelFlags.begin(), m_lightingChannelFlags.end(), [this, &index](const bool value){
                this->m_mask |= (static_cast<AZ::u32>(value) << (index++));
            });
        }
    }
}