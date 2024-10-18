/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/Feature/SkyBox/SkyBoxFogBus.h>
#include <Atom/Feature/SkyBox/SkyBoxFogSettings.h>

namespace AZ
{
    namespace Render
    {
        void SkyBoxFogSettings::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SkyBoxFogSettings>()
                    ->Version(1)
                    ->Field("Enable", &SkyBoxFogSettings::m_enable)
                    ->Field("Color", &SkyBoxFogSettings::m_color)
                    ->Field("TopHeight", &SkyBoxFogSettings::m_topHeight)
                    ->Field("BottomHeight", &SkyBoxFogSettings::m_bottomHeight)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<SkyBoxFogSettings>("SkyBoxFogSettings", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &SkyBoxFogSettings::m_enable, "Enable Fog", "Toggle fog on or off")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &SkyBoxFogSettings::m_color, "Fog Color", "Color of the fog")
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, &SkyBoxFogSettings::IsFogDisabled)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyBoxFogSettings::m_topHeight, "Fog Top Height", "Height of the fog upwards from the horizon")
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, &SkyBoxFogSettings::IsFogDisabled)
                                ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                                ->Attribute(AZ::Edit::Attributes::Max, 0.5)
                                ->Attribute(AZ::Edit::Attributes::Step, 0.01)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyBoxFogSettings::m_bottomHeight, "Fog Bottom Height", "Height of the fog downwards from the horizon")
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, &SkyBoxFogSettings::IsFogDisabled)
                                ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                                ->Attribute(AZ::Edit::Attributes::Max, 0.3)
                                ->Attribute(AZ::Edit::Attributes::Step, 0.01)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<SkyBoxFogRequestBus>("SkyBoxFogRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Event("SetEnabled", &SkyBoxFogRequestBus::Events::SetEnabled)
                    ->Event("IsEnabled", &SkyBoxFogRequestBus::Events::IsEnabled)
                    ->Event("SetColor", &SkyBoxFogRequestBus::Events::SetColor)
                    ->Event("GetColor", &SkyBoxFogRequestBus::Events::GetColor)
                    ->Event("SetTopHeight", &SkyBoxFogRequestBus::Events::SetTopHeight)
                    ->Event("GetTopHeight", &SkyBoxFogRequestBus::Events::GetTopHeight)
                    ->Event("SetBottomHeight", &SkyBoxFogRequestBus::Events::SetBottomHeight)
                    ->Event("GetBottomHeight", &SkyBoxFogRequestBus::Events::GetBottomHeight)
                    ->VirtualProperty("Enable", "IsEnabled", "SetEnabled")
                    ->VirtualProperty("Color", "GetColor", "SetColor")
                    ->VirtualProperty("TopHeight", "GetTopHeight", "SetTopHeight")
                    ->VirtualProperty("BottomHeight", "GetTopHeight", "SetBottomHeight")
                ;
            }
        }

        bool SkyBoxFogSettings::IsFogDisabled() const
        {
            return !m_enable;
        }
    }
}
