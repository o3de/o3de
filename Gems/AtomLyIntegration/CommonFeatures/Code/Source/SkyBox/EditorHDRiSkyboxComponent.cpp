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

#include <SkyBox/EditorHDRiSkyboxComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void EditorHDRiSkyboxComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorHDRiSkyboxComponent, BaseClass>()
                    ->Version(2, ConvertToEditorRenderComponentAdapter<2>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorHDRiSkyboxComponent>(
                        "HDRi Skybox", "SkyBox component render the background of your scene with cubemap")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "editor/icons/components/viewport/component_placeholder.png")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "")
                        ;

                    editContext->Class<HDRiSkyboxComponentController>(
                        "HDRiSkyboxComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &HDRiSkyboxComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<HDRiSkyboxComponentConfig>(
                        "HDRiSkyboxComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HDRiSkyboxComponentConfig::m_cubemapAsset, "Cubemap Texture", "The texture used for cubemap rendering")
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRiSkyboxComponentConfig::m_exposure, "Exposure", "Exposure in stops")
                                ->Attribute(AZ::Edit::Attributes::SoftMin, -5.0f)
                                ->Attribute(AZ::Edit::Attributes::SoftMax, 5.0f)
                                ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                                ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorHDRiSkyboxComponent>()->RequestBus("HDRiSkyboxRequestBus");

                behaviorContext->ConstantProperty("EditorHDRiSkyboxComponentTypeId", BehaviorConstant(Uuid(EditorHDRiSkyboxComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorHDRiSkyboxComponent::EditorHDRiSkyboxComponent(const HDRiSkyboxComponentConfig& config)
            : BaseClass(config)
        {
        }
    }
}
