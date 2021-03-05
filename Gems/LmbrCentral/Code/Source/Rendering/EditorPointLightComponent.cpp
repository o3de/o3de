/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "LmbrCentral_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include "EditorPointLightComponent.h"

namespace LmbrCentral
{
    void EditorPointLightComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorPointLightComponent, EditorLightComponent>()->
                Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorPointLightComponent>(
                    "Point Light", "The Point Light component allows an entity to create a point of light")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PointLight.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/PointLight.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-point-light.html")
                        ->Attribute(AZ::Edit::Attributes::ExportIfAllPlatformTags, AZStd::vector<AZ::Crc32>{AZ_CRC("renderer", 0xf199a19c)}) // Only export on platforms that render
                        ->Attribute(AZ::Edit::Attributes::RuntimeExportCallback, &EditorLightComponent::ExportLightComponent)
                    ;
            }
        }
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<EditorPointLightComponent>()->RequestBus("EditorPointLightComponentBus");
        }
    }

    void EditorPointLightComponent::Init()
    {
        SetLightType(EditorLightConfiguration::LightType::Point);
        EditorLightComponent::Init();
    }

} // namespace LmbrCentral
