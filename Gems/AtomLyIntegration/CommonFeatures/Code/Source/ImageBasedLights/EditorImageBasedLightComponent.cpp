/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageBasedLights/EditorImageBasedLightComponent.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QMessageBox>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        void EditorImageBasedLightComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorImageBasedLightComponent, BaseClass>()
                    ->Version(2, ConvertToEditorRenderComponentAdapter<1>)
                    ->Field("diffuseImageAsset", &EditorImageBasedLightComponent::m_diffuseImageAsset)
                    ->Field("specularImageAsset", &EditorImageBasedLightComponent::m_specularImageAsset)
                    ->Field("exposure", &EditorImageBasedLightComponent::m_exposure)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorImageBasedLightComponent>(
                        "Global Skylight (IBL)", "Adds image based illumination to the scene")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Lighting")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/global-skylight-ibl/")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageBasedLightComponent::m_diffuseImageAsset, "Diffuse Image", "Cubemap image asset for determining diffuse lighting")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorImageBasedLightComponent::OnDiffuseImageAssetChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageBasedLightComponent::m_specularImageAsset, "Specular Image", "Cubemap image asset for determining specular lighting")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorImageBasedLightComponent::OnSpecularImageAssetChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorImageBasedLightComponent::m_exposure, "Exposure", "Exposure in stops")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorImageBasedLightComponent::OnExposureChanged)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -5.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 5.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ;

                    editContext->Class<ImageBasedLightComponentController>(
                        "ImageBasedLightComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ImageBasedLightComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorImageBasedLightComponent>()->RequestBus("ImageBasedLightComponentRequestBus");

                behaviorContext->ConstantProperty("EditorImageBasedLightComponentTypeId", BehaviorConstant(Uuid(EditorImageBasedLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorImageBasedLightComponent::EditorImageBasedLightComponent(const ImageBasedLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorImageBasedLightComponent::Activate()
        {
            BaseClass::Activate();

            ImageBasedLightComponentConfig& configuration = m_controller.m_configuration;

            m_diffuseImageAsset = configuration.m_diffuseImageAsset;
            m_specularImageAsset = configuration.m_specularImageAsset;
            m_exposure = configuration.m_exposure;
        }

        AZ::u32 EditorImageBasedLightComponent::OnDiffuseImageAssetChanged()
        {
            m_controller.SetDiffuseImageAsset(m_diffuseImageAsset);

            if (UpdateImageAsset(m_diffuseImageAsset, "_ibldiffuse", "Diffuse",
                                 m_specularImageAsset, "_iblspecular", "Specular"))
            {
                m_controller.SetSpecularImageAsset(m_specularImageAsset);
            }

            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

        AZ::u32 EditorImageBasedLightComponent::OnSpecularImageAssetChanged()
        {
            m_controller.SetSpecularImageAsset(m_specularImageAsset);

            if (UpdateImageAsset(m_specularImageAsset, "_iblspecular", "Specular",
                                 m_diffuseImageAsset, "_ibldiffuse", "Diffuse"))
            {
                m_controller.SetDiffuseImageAsset(m_diffuseImageAsset);
            }

            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

        bool EditorImageBasedLightComponent::UpdateImageAsset(
            Data::Asset<RPI::StreamingImageAsset>& asset1,
            const char* asset1Suffix,
            const char* asset1Name,
            Data::Asset<RPI::StreamingImageAsset>& asset2,
            const char* asset2Suffix,
            const char* asset2Name)
        {
            AZStd::string assetPath;
            if (asset1.GetId().IsValid())
            {
                // retrieve the file path for asset1
                AZ::Data::AssetInfo assetInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, asset1.GetId());
                AZ_Assert(assetInfo.m_assetId == asset1.GetId(), "Failed to retrieve AssetInfo for image asset");

                assetPath = assetInfo.m_relativePath;
            }

            AZ::Data::AssetId assetId;
            if (!assetPath.empty())
            {
                // try to load the matching image asset by replacing the asset1 suffix with the asset2 suffix in the file path
                AZ::StringFunc::Replace(assetPath, asset1Suffix, asset2Suffix);

                assetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(assetPath.c_str(), AZ::RPI::AssetUtils::TraceLevel::None);
                if (!assetId.IsValid())
                {
                    // unable to locate the matching image asset
                    return false;
                }
            }

            if (asset2.GetId().IsValid())
            {
                // prompt to see if the user wants to overwrite the image asset
                AZStd::string message = AZStd::string::format("Update %s image to match the selected %s image?", asset2Name, asset1Name);

                if (QMessageBox::question(QApplication::activeWindow(), "Global Skylight", message.c_str(), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
                {
                    return false;
                }
            }

            // create the matching image asset
            // Note: this will clear asset2 if asset1 was cleared
            asset2.Create(assetId);

            return true;
        }

        AZ::u32 EditorImageBasedLightComponent::OnExposureChanged()
        {
            m_controller.SetExposure(m_exposure);

            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

    } // namespace Render
} // namespace AZ
