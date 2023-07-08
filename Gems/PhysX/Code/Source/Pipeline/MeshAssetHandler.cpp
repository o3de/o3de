/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Pipeline/MeshAssetHandler.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
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
            return "Icons/Components/PhysXMeshCollider.svg";
        }

        const char* MeshAssetHandler::GetGroup() const
        {
            return "Physics";
        }

        // Disable spawning of physics asset entities on drag and drop
        AZ::Uuid MeshAssetHandler::GetComponentTypeId() const
        {
            // NOTE: This doesn't do anything when CanCreateComponent returns false
            return AZ::Uuid("{20382794-0E74-4860-9C35-A19F22DC80D4}"); // EditorMeshColliderComponent
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

        // Fixes up an asset by querying the asset catalog for its id using its hint path.
        // Returns true if it was able to find the asset in the catalog and set the asset id.
        static bool FixUpAssetIdByHint(AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, asset.GetHint().c_str(),
                AZ::Data::s_invalidAssetType, false);

            if (assetId.IsValid())
            {
                asset.Create(assetId, false);
                return true;
            }
            return false;
        }

        AZ::Data::AssetHandler::LoadResult MeshAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            MeshAsset* meshAsset = asset.GetAs<MeshAsset>();
            if (!meshAsset)
            {
                AZ_Error("PhysX Mesh Asset", false, "This should be a PhysXMeshAsset, as this is the only type we process.");
                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            if (!AZ::Utils::LoadObjectFromStreamInPlace(
                    *stream, meshAsset->m_assetData, serializeContext, AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB)))
            {
                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            // PhysX Mesh Asset could have been saved with material assets that only have the hint valid (asset path in the cache).
            // This could happen for example when MeshGroup was filled procedurally and the materials were only given the path (hint).
            // Now at runtime after the PhysX Mesh is loaded let's complete the asset by looking for it in the catalog and
            // assigning its id. At runtime physics material will always be in the catalog because it's a critical asset.
            for (size_t slotId = 0; slotId < meshAsset->m_assetData.m_materialSlots.GetSlotsCount(); ++slotId)
            {
                AZ::Data::Asset<AZ::Data::AssetData> materialAsset = meshAsset->m_assetData.m_materialSlots.GetMaterialAsset(slotId);
                // Does it need to resolve its id from the hint path?
                if (!materialAsset.GetId().IsValid() && !materialAsset.GetHint().empty())
                {
                    if (FixUpAssetIdByHint(materialAsset))
                    {
                        meshAsset->m_assetData.m_materialSlots.SetMaterialAsset(slotId, materialAsset);
                    }
                    else
                    {
                        AZ_Warning("PhysX Mesh Asset", false,
                            "Loading PhysX Mesh '%s' it didn't find physics material '%s', assigned to slot '%.*s'. Default physics material will be used.",
                            asset.GetHint().c_str(),
                            materialAsset.GetHint().c_str(),
                            AZ_STRING_ARG(meshAsset->m_assetData.m_materialSlots.GetSlotName(slotId)));
                    }
                }
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
                serializeContext->ClassDeprecate("MeshAssetCookedData", AZ::Uuid("{82955F2F-4DA1-4AEF-ACEF-0AE16BA20EF4}"));

                serializeContext->Class<MeshAssetData>()
                    ->Version(2)
                    ->Field("ColliderShapes", &MeshAssetData::m_colliderShapes)
                    ->Field("MaterialSlots", &MeshAssetData::m_materialSlots)
                    ->Field("MaterialIndexPerShape", &MeshAssetData::m_materialIndexPerShape)
                    ;
            }
        }

        AZ::Data::Asset<MeshAsset> MeshAssetData::CreateMeshAsset() const
        {
            AZ::Data::Asset<MeshAsset> meshAsset =
                AZ::Data::AssetManager::Instance().CreateAsset<MeshAsset>(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

            meshAsset->SetData(*this);

            return meshAsset;
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

        void MeshAsset::SetData(const MeshAssetData& assetData)
        {
            m_assetData = assetData;
            m_status = AZ::Data::AssetData::AssetStatus::Ready;
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
