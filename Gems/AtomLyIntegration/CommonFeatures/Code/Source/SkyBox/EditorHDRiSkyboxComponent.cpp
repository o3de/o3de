/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Environment")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/hdri-skybox/")
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
                                ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, false)
                                ->Attribute(AZ::Edit::Attributes::HideProductFilesInAssetPicker, true)
                                ->Attribute(AZ::Edit::Attributes::AssetPickerTitle, "Cubemap Asset")

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
