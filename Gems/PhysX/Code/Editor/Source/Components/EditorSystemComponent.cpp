/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSystemComponent.h"
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

#include <IEditor.h>

#include <Editor/ConfigStringLineEditCtrl.h>
#include <Editor/EditorJointConfiguration.h>
#include <Editor/EditorWindow.h>
#include <Editor/PropertyTypes.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    constexpr const char* DefaultAssetFilePath = "Assets/Physics/SurfaceTypeMaterialLibrary";
    constexpr const char* TemplateAssetFilename = "PhysX/TemplateMaterialLibrary";

    static AZStd::optional<AZ::Data::Asset<AZ::Data::AssetData>> GetMaterialLibraryTemplate()
    {
        const auto& assetType = AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid();

        AZStd::vector<AZStd::string> assetTypeExtensions;
        AZ::AssetTypeInfoBus::Event(assetType, &AZ::AssetTypeInfo::GetAssetTypeExtensions, assetTypeExtensions);

        if (assetTypeExtensions.size() == 1)
        {
            // Constructing the path to the library asset
            const AZStd::string& assetExtension = assetTypeExtensions[0];

            // Use the path relative to the asset root to avoid hardcoding full path in the configuration
            AZStd::string relativePath = TemplateAssetFilename;
            AzFramework::StringFunc::Path::ReplaceExtension(relativePath, assetExtension.c_str());

            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, relativePath.c_str(), assetType, false /*autoRegisterIfNotFound*/);

            if (assetId.IsValid())
            {
                return AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(assetId, AZ::Data::AssetLoadBehavior::NoLoad);
            }
        }

        return AZStd::nullopt;
    }

    static AZStd::optional<AZ::Data::Asset<AZ::Data::AssetData>> CreateMaterialLibrary(const AZStd::string& fullTargetFilePath, const AZStd::string& relativePath)
    {
        AZ::IO::FileIOStream fileStream(fullTargetFilePath.c_str(), AZ::IO::OpenMode::ModeWrite);
        if (fileStream.IsOpen())
        {
            const auto& assetType = AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid();
            AZ::Data::AssetId assetId;

            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, relativePath.c_str(), assetType, true /*autoRegisterIfNotFound*/);

            AZ::Data::Asset<AZ::Data::AssetData> newAsset =
                AZ::Data::AssetManager::Instance().FindOrCreateAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::Default);

            if (auto* newMaterialLibraryData = azrtti_cast<Physics::MaterialLibraryAsset*>(newAsset.GetData()))
            {
                if (auto templateLibraryOpt = GetMaterialLibraryTemplate())
                {
                    if (const auto* templateMaterialLibData = azrtti_cast<Physics::MaterialLibraryAsset*>(templateLibraryOpt->GetData()))
                    {
                        templateLibraryOpt->QueueLoad();
                        templateLibraryOpt->BlockUntilLoadComplete();

                        // Fill the newly created material library using the template data
                        for (const auto& materialData : templateMaterialLibData->GetMaterialsData())
                        {
                            newMaterialLibraryData->AddMaterialData(materialData);
                        }

                        // check it out in the source control system
                        AzToolsFramework::SourceControlCommandBus::Broadcast(
                            &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit, fullTargetFilePath.c_str(), true /*allowMultiCheckout*/,
                            [](bool /*success*/, const AzToolsFramework::SourceControlFileInfo& /*info*/) {});

                        // Save the material library asset into a file
                        auto assetHandler = AZ::Data::AssetManager::Instance().GetHandler(assetType);
                        if (assetHandler->SaveAssetData(newAsset, &fileStream))
                        {
                            return newAsset;
                        }
                        else
                        {
                            AZ_Error(
                                "PhysX", false,
                                "CreateSurfaceTypeMaterialLibrary: Unable to save Surface Types Material Library Asset to %s",
                                fullTargetFilePath.c_str());
                        }
                    }
                }
            }
        }

        return AZStd::nullopt;
    }

    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorJointLimitConfig::Reflect(context);
        EditorJointLimitPairConfig::Reflect(context);
        EditorJointLimitConeConfig::Reflect(context);
        EditorJointConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void EditorSystemComponent::Activate()
    {
        Physics::EditorWorldBus::Handler::BusConnect();
        AzToolsFramework::EditorContextMenuBus::Handler::BusConnect();

        m_onMaterialLibraryLoadErrorEventHandler = AzPhysics::SystemEvents::OnMaterialLibraryLoadErrorEvent::Handler(
            [this]([[maybe_unused]] AzPhysics::SystemEvents::MaterialLibraryLoadErrorType error)
            {
                // Attempt to set/create the default material library if there was an error
                if (auto* physxSystem = GetPhysXSystem())
                {
                    if (auto retrievedMaterialLibrary = RetrieveDefaultMaterialLibrary())
                    {
                        physxSystem->UpdateMaterialLibrary(retrievedMaterialLibrary.value());

                        // After setting the default material library, save the physx configuration.
                        auto saveCallback = []([[maybe_unused]] const PhysXSystemConfiguration& config, [[maybe_unused]] PhysXSettingsRegistryManager::Result result)
                        {
                            AZ_Warning("PhysX", result == PhysXSettingsRegistryManager::Result::Success,
                                "Unable to save the PhysX configuration after setting default material library.");
                        };
                        physxSystem->GetSettingsRegistryManager().SaveSystemConfiguration(physxSystem->GetPhysXConfiguration(), saveCallback);
                    }
                }
            }
        );

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration editorWorldConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            editorWorldConfiguration.m_sceneName = AzPhysics::EditorPhysicsSceneName;
            m_editorWorldSceneHandle = physicsSystem->AddScene(editorWorldConfiguration);
            physicsSystem->RegisterOnMaterialLibraryLoadErrorEventHandler(m_onMaterialLibraryLoadErrorEventHandler);
        }

        PhysX::RegisterConfigStringLineEditHandler(); // Register custom unique string line edit control
        PhysX::Editor::RegisterPropertyTypes();

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    }

    void EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::EditorContextMenuBus::Handler::BusDisconnect();
        Physics::EditorWorldBus::Handler::BusDisconnect();

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_editorWorldSceneHandle);
        }
        m_editorWorldSceneHandle = AzPhysics::InvalidSceneHandle;

        m_onMaterialLibraryLoadErrorEventHandler.Disconnect();
    }

    AzPhysics::SceneHandle EditorSystemComponent::GetEditorSceneHandle() const
    {
        return m_editorWorldSceneHandle;
    }

    void EditorSystemComponent::OnStartPlayInEditorBegin()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (AzPhysics::Scene* scene = physicsSystem->GetScene(m_editorWorldSceneHandle))
            {
                scene->SetEnabled(false);
            }
        }
    }

    void EditorSystemComponent::OnStopPlayInEditor()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (AzPhysics::Scene* scene = physicsSystem->GetScene(m_editorWorldSceneHandle))
            {
                scene->SetEnabled(true);
            }
        }
    }

    void EditorSystemComponent::PopulateEditorGlobalContextMenu([[maybe_unused]] QMenu* menu, [[maybe_unused]] const AZ::Vector2& point, [[maybe_unused]] int flags)
    {

    }

    void EditorSystemComponent::NotifyRegisterViews()
    {
        PhysX::Editor::EditorWindow::RegisterViewClass();
    }

    AZStd::optional<AZ::Data::Asset<AZ::Data::AssetData>> EditorSystemComponent::RetrieveDefaultMaterialLibrary()
    {
        AZ::Data::AssetId resultAssetId;

        auto assetType = AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid();

        AZStd::vector<AZStd::string> assetTypeExtensions;
        AZ::AssetTypeInfoBus::Event(assetType, &AZ::AssetTypeInfo::GetAssetTypeExtensions, assetTypeExtensions);

        if (assetTypeExtensions.size() == 1)
        {
            // Constructing the path to the library asset
            const AZStd::string& assetExtension = assetTypeExtensions[0];

            // Use the path relative to the asset root to avoid hardcoding full path in the configuration
            AZStd::string relativePath = DefaultAssetFilePath;
            AzFramework::StringFunc::Path::ReplaceExtension(relativePath, assetExtension.c_str());

            // Try to find an already existing material library
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(resultAssetId,
                &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, relativePath.c_str(), azrtti_typeid<Physics::MaterialLibraryAsset>(), false /*autoRegisterIfNotFound*/);

            if (!resultAssetId.IsValid())
            {
                // No file for the default material library, create it
                AZ::IO::Path fullPath;
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    settingsRegistry->Get(fullPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
                }
                fullPath /= DefaultAssetFilePath;
                fullPath.ReplaceExtension(AZ::IO::PathView(assetExtension));

                if (auto materialLibraryOpt = CreateMaterialLibrary(fullPath.Native(), relativePath))
                {
                    return materialLibraryOpt;
                }
                else
                {
                    AZ_Warning("PhysX", false,
                        "CreateMaterialLibrary: Failed to create material library at %s. "
                        "Please check if the file is writable", fullPath.c_str());
                }
            }
            else
            {
                AZ::Data::Asset<AZ::Data::AssetData> existingMaterialLibrary =
                    AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(resultAssetId, AZ::Data::AssetLoadBehavior::NoLoad);

                return existingMaterialLibrary;
            }
        }
        else
        {
            AZ_Warning("PhysX", false, 
                "RetrieveDefaultMaterialLibrary: Number of extensions for the physics material library asset is %u"
                " but should be 1. Please check if the asset registered itself with the asset system correctly",
                assetTypeExtensions.size())
        }

        return AZStd::nullopt;
    }
}

