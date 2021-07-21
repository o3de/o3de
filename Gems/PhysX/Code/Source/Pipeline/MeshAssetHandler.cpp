/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <PhysX_precompiled.h>

#include <Pipeline/MeshAssetHandler.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/EditContext.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/SystemComponentBus.h>
#include <Source/Pipeline/MeshAssetHandler.h>

namespace PhysX
{
    namespace Pipeline
    {
        const char* MeshAssetHandler::s_assetFileExtension = "pxmesh";

        MeshAssetHandler::MeshAssetHandler()
        {
            Register();
        }

        MeshAssetHandler::~MeshAssetHandler()
        {
            Unregister();
        }

        void MeshAssetHandler::Register()
        {
            bool assetManagerReady = AZ::Data::AssetManager::IsReady();
            AZ_Error("PhysX Mesh Asset", assetManagerReady, "Asset manager isn't ready.");
            if (assetManagerReady)
            {
                AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<MeshAsset>::Uuid());
            }

            AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<MeshAsset>::Uuid());
        }

        void MeshAssetHandler::Unregister()
        {
            AZ::AssetTypeInfoBus::Handler::BusDisconnect();

            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(this);
            }
        }

        // AZ::AssetTypeInfoBus
        AZ::Data::AssetType MeshAssetHandler::GetAssetType() const
        {
            return AZ::AzTypeInfo<MeshAsset>::Uuid();
        }

        void MeshAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back(s_assetFileExtension);
        }

        const char* MeshAssetHandler::GetAssetTypeDisplayName() const
        {
            return "PhysX Collision Mesh (PhysX Gem)";
        }

        const char* MeshAssetHandler::GetBrowserIcon() const
        {
            return "Icons/Components/ColliderMesh.svg";
        }

        const char* MeshAssetHandler::GetGroup() const
        {
            return "Physics";
        }

        // Disable spawning of physics asset entities on drag and drop
        AZ::Uuid MeshAssetHandler::GetComponentTypeId() const
        {
            // NOTE: This doesn't do anything when CanCreateComponent returns false
            return AZ::Uuid("{FD429282-A075-4966-857F-D0BBF186CFE6}"); // EditorColliderComponent
        }

        bool MeshAssetHandler::CanCreateComponent([[maybe_unused]] const AZ::Data::AssetId& assetId) const
        {
            return false;
        }

        // AZ::Data::AssetHandler
        AZ::Data::AssetPtr MeshAssetHandler::CreateAsset([[maybe_unused]] const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
        {
            if (type == AZ::AzTypeInfo<MeshAsset>::Uuid())
            {
                return aznew MeshAsset();
            }

            AZ_Error("PhysX Mesh Asset", false, "This handler deals only with PhysXMeshAsset type.");
            return nullptr;
        }

        AZ::Data::AssetHandler::LoadResult MeshAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            MeshAsset* meshAsset = asset.GetAs<MeshAsset>();
            if (!meshAsset)
            {
                AZ_Error("PhysX Mesh Asset", false, "This should be a PhysXMeshAsset, as this is the only type we process.");
                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            if (!AZ::Utils::LoadObjectFromStreamInPlace(*stream, meshAsset->m_assetData, serializeContext))
            {
                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

        void MeshAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
        {
            delete ptr;
        }

        void MeshAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
        {
            assetTypes.push_back(AZ::AzTypeInfo<MeshAsset>::Uuid());
        }

        void MeshAssetData::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                AssetColliderConfiguration::Reflect(context);
                serializeContext->ClassDeprecate("MeshAssetCookedData", "{82955F2F-4DA1-4AEF-ACEF-0AE16BA20EF4}");

                serializeContext->Class<MeshAssetData>()
                    ->Field("ColliderShapes", &MeshAssetData::m_colliderShapes)
                    ->Field("SurfaceNames", &MeshAssetData::m_materialNames)
                    ->Field("MaterialNames", &MeshAssetData::m_physicsMaterialNames)
                    ->Field("MaterialIndexPerShape", &MeshAssetData::m_materialIndexPerShape)
                    ;
            }
        }

        void MeshAsset::Reflect(AZ::ReflectContext* context)
        {
            MeshAssetData::Reflect(context);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MeshAsset>()
                    ->Field("MeshAssetData", &MeshAsset::m_assetData)
                    ;

                // Note: This class needs to have edit context reflection so PropertyAssetCtrl::OnEditButtonClicked
                //       can open the asset with the preferred asset editor (Scene Settings).
                if (auto* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<MeshAsset>("PhysX Mesh Asset", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }
        }

        void AssetColliderConfiguration::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AssetColliderConfiguration>()
                    ->Field("CollisionLayer", &AssetColliderConfiguration::m_collisionLayer)
                    ->Field("CollisionGroupId", &AssetColliderConfiguration::m_collisionGroupId)
                    ->Field("isTrigger", &AssetColliderConfiguration::m_isTrigger)
                    ->Field("Transform", &AssetColliderConfiguration::m_transform)
                    ->Field("Tag", &AssetColliderConfiguration::m_tag)
                    ;
            }
        }

        void AssetColliderConfiguration::UpdateColliderConfiguration(Physics::ColliderConfiguration& colliderConfiguration) const
        {
            if (m_collisionLayer)
            {
                colliderConfiguration.m_collisionLayer = *m_collisionLayer;
            }

            if (m_collisionGroupId)
            {
                colliderConfiguration.m_collisionGroupId = *m_collisionGroupId;
            }

            if (m_isTrigger)
            {
                colliderConfiguration.m_isTrigger = *m_isTrigger;
            }

            if (m_transform)
            {
                // Apply the local shape transform to the collider one
                AZ::Transform existingTransform = 
                    AZ::Transform::CreateFromQuaternionAndTranslation(colliderConfiguration.m_rotation, colliderConfiguration.m_position);

                AZ::Transform shapeTransform = *m_transform;
                shapeTransform.ExtractUniformScale();

                shapeTransform = existingTransform * shapeTransform;

                colliderConfiguration.m_position = shapeTransform.GetTranslation();
                colliderConfiguration.m_rotation = shapeTransform.GetRotation();
                colliderConfiguration.m_rotation.Normalize();
            }

            if (m_tag)
            {
                colliderConfiguration.m_tag = *m_tag;
            }
        }


} //namespace Pipeline
} // namespace PhysX
