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

#include <PhysX_precompiled.h>

#include "EditorSystemComponent.h"
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <Editor/ConfigStringLineEditCtrl.h>
#include <Editor/EditorJointConfiguration.h>

#include <I3DEngine.h>
#include <IEditor.h>
#include <ISurfaceType.h>

#include <System/PhysXSystem.h>

namespace PhysX
{
    static bool CreateSurfaceTypeMaterialLibrary(const AZStd::string & targetFilePath)
    {
        auto assetType = AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid();

        // Create File
        AZ::Data::Asset<AZ::Data::AssetData> newAsset = AZ::Data::AssetManager::Instance().CreateAsset(AZ::Uuid::CreateRandom(), assetType, AZ::Data::AssetLoadBehavior::Default);

        AZ::IO::FileIOStream fileStream(targetFilePath.c_str(), AZ::IO::OpenMode::ModeWrite);
        if (fileStream.IsOpen())
        {
            Physics::MaterialLibraryAsset* materialLibraryAsset = azrtti_cast<Physics::MaterialLibraryAsset*>(newAsset.GetData());
            if (materialLibraryAsset)
            {
                IEditor* editor = nullptr;
                AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);

                ISurfaceTypeEnumerator* surfaceTypeEnumerator = nullptr;
                if (editor)
                {
                    surfaceTypeEnumerator = editor->Get3DEngine()->GetMaterialManager()->GetSurfaceTypeManager()->GetEnumerator();
                }

                if (surfaceTypeEnumerator)
                {
                    // Enumerate through CryEngine surface types and create a Physics API material for each of them
                    for (ISurfaceType* pSurfaceType = surfaceTypeEnumerator->GetFirst(); pSurfaceType != nullptr;
                        pSurfaceType = surfaceTypeEnumerator->GetNext())
                    {
                        const ISurfaceType::SPhysicalParams& physicalParams = pSurfaceType->GetPhyscalParams();

                        Physics::MaterialFromAssetConfiguration configuration;
                        configuration.m_configuration = Physics::MaterialConfiguration();
                        configuration.m_configuration.m_dynamicFriction = AZ::GetMax(0.0f, physicalParams.friction);
                        configuration.m_configuration.m_staticFriction = AZ::GetMax(0.0f, physicalParams.friction);
                        configuration.m_configuration.m_restitution = AZ::GetClamp(physicalParams.bouncyness, 0.0f, 1.0f);
                        configuration.m_configuration.m_surfaceType = pSurfaceType->GetType();
                        configuration.m_id = Physics::MaterialId::Create();

                        materialLibraryAsset->AddMaterialData(configuration);
                    }
                }

                // check it out in the source control system
                AzToolsFramework::SourceControlCommandBus::Broadcast(
                    &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit, targetFilePath.c_str(), true,
                    [](bool /*success*/, const AzToolsFramework::SourceControlFileInfo& /*info*/) {});

                // Save the material library asset into a file
                auto assetHandler = const_cast<AZ::Data::AssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(assetType));
                if (assetHandler->SaveAssetData(newAsset, &fileStream))
                {
                    return true;
                }
                else
                {
                    AZ_Error("PhysX", false,
                        "CreateSurfaceTypeMaterialLibrary: Unable to save Surface Types Material Library Asset to %s",
                        targetFilePath.c_str());
                }
            }
        }

        return false;
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

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration editorWorldConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            editorWorldConfiguration.m_sceneName = AzPhysics::EditorPhysicsSceneName;
            editorWorldConfiguration.m_sceneName = "EditorScene";
            m_editorWorldSceneHandle = physicsSystem->AddScene(editorWorldConfiguration);
        }

        PhysX::RegisterConfigStringLineEditHandler(); // Register custom unique string line edit control

        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    }

    void EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        Physics::EditorWorldBus::Handler::BusDisconnect();

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_editorWorldSceneHandle);
        }
        m_editorWorldSceneHandle = AzPhysics::InvalidSceneHandle;
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

    AZ::Data::AssetId EditorSystemComponent::GenerateSurfaceTypesLibrary()
    {
        AZ::Data::AssetId resultAssetId;

        auto assetType = AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid();

        AZStd::vector<AZStd::string> assetTypeExtensions;
        AZ::AssetTypeInfoBus::Event(assetType, &AZ::AssetTypeInfo::GetAssetTypeExtensions, assetTypeExtensions);

        if (assetTypeExtensions.size() == 1)
        {
            const char* DefaultAssetFilename = "SurfaceTypeMaterialLibrary";

            // Constructing the path to the library asset
            const AZStd::string& assetExtension = assetTypeExtensions[0];

            // Use the path relative to the asset root to avoid hardcoding full path in the configuration
            AZStd::string relativePath = DefaultAssetFilename;
            AzFramework::StringFunc::Path::ReplaceExtension(relativePath, assetExtension.c_str());

            // Try to find an already existing material library
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(resultAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, relativePath.c_str(), azrtti_typeid<Physics::MaterialLibraryAsset>(), false);

            if (!resultAssetId.IsValid())
            {
                const char* assetRoot = AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@");

                AZStd::string fullPath;
                AzFramework::StringFunc::Path::ConstructFull(assetRoot, DefaultAssetFilename, assetExtension.c_str(), fullPath);

                if (CreateSurfaceTypeMaterialLibrary(fullPath))
                {
                    // Find out the asset ID for the material library we've just created
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        resultAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath,
                        relativePath.c_str(),
                        azrtti_typeid<Physics::MaterialLibraryAsset>(), true);
                }
                else
                {
                    AZ_Warning("PhysX", false,
                        "GenerateSurfaceTypesLibrary: Failed to create material library at %s. "
                        "Please check if the file is writable", fullPath.c_str());
                }
            }
        }
        else
        {
            AZ_Warning("PhysX", false, 
                "GenerateSurfaceTypesLibrary: Number of extensions for the physics material library asset is %u"
                " but should be 1. Please check if the asset registered itself with the asset system correctly",
                assetTypeExtensions.size())
        }

        return resultAssetId;
    }
}

