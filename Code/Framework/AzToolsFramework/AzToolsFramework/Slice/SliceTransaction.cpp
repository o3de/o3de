/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/SliceEntityOwnershipServiceBus.h>
#include <AzFramework/Slice/SliceEntityBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Slice/SliceTransaction.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Undo/UndoSystem.h>


namespace AzToolsFramework
{
    namespace SliceUtilities
    {
        //=========================================================================
        namespace Internal
        {
            AZStd::string MakeTemporaryFilePathForSave(const char* fullPath);
            SliceTransaction::Result SaveSliceToDisk(const char* targetPath, AZStd::vector<AZ::u8>& sliceAssetEntityMemoryBuffer, AZ::SerializeContext* serializeContext = nullptr);

            class SaveSliceToDiskCommand
                : public UndoSystem::URSequencePoint
            {
                using ByteBuffer = AZStd::vector<AZ::u8>;
            public:
                AZ_RTTI(SaveSliceToDiskCommand, "{F036A88D-7487-4BE9-BD2C-41B80B86ACC5}", UndoSystem::URSequencePoint);
                AZ_CLASS_ALLOCATOR(SaveSliceToDiskCommand, AZ::SystemAllocator, 0);

                SaveSliceToDiskCommand(const char* friendlyName = nullptr)
                    : UndoSystem::URSequencePoint(friendlyName)
                    , m_isNewAsset(false)
                    , m_redoResult(AZ::Failure(AZStd::string("No redo run.")))
                {
                }

                ~SaveSliceToDiskCommand() override
                {
                }

                void Capture(const SliceTransaction::SliceAssetPtr& before, const SliceTransaction::SliceAssetPtr& after, const char* sliceAssetPath)
                {
                    AZ_PROFILE_FUNCTION(AzToolsFramework);

                    m_sliceAssetPath = sliceAssetPath;
                    m_isNewAsset = !before.GetId().IsValid();

                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    AZ_Assert(serializeContext, "Failed to retrieve serialize context.");

                    if (!m_isNewAsset)
                    {
                        AZ_PROFILE_SCOPE(AzToolsFramework, "SliceUtilities::Internal::SaveSliceToDiskCommand::Capture:SaveBefore");
                        AZ::SliceAsset* sliceBefore = before.Get();
                        AZ::Entity* sliceEntityBefore = sliceBefore->GetEntity();
                        AZ::IO::ByteContainerStream<ByteBuffer> beforeStream(&m_sliceAssetBeforeBuffer);
                        AZ::Utils::SaveObjectToStream(beforeStream, GetSliceStreamFormat(), sliceEntityBefore, serializeContext);
                    }

                    {
                        AZ_PROFILE_SCOPE(AzToolsFramework, "SliceUtilities::Internal::SaveSliceToDiskCommand::Capture:SaveAfter");
                        AZ::SliceAsset* sliceAfter = after.Get();
                        AZ::Entity* sliceEntityAfter = sliceAfter->GetEntity();
                        AZ::IO::ByteContainerStream<ByteBuffer> afterStream(&m_sliceAssetAfterBuffer);
                        AZ::Utils::SaveObjectToStream(afterStream, GetSliceStreamFormat(), sliceEntityAfter, serializeContext);
                    }
                }

                bool Changed() const override
                {
                    // If the undo/redo buffer becomes full of no-op slice pushes, then
                    // this should be implemented. For now, it's assumed that the slice system
                    // will prevent users from creating no-op slice pushes in the first place.
                    return true;
                }

                SliceTransaction::Result GetRedoResult()
                {
                    return m_redoResult;
                }

                void Redo() override
                {
                    AZ_PROFILE_FUNCTION(AzToolsFramework);
                    m_redoResult = Internal::SaveSliceToDisk(m_sliceAssetPath.c_str(), m_sliceAssetAfterBuffer);
                }

                void Undo() override
                {
                    AZ_PROFILE_FUNCTION(AzToolsFramework);
                    if (m_isNewAsset)
                    {
                        // New asset means we didn't have an existing asset, so we should instead remove the newly created asset as our undo
                        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                        AZ_Assert(fileIO, "File IO is not initialized.");

                        if (fileIO->Exists(m_sliceAssetPath.c_str()))
                        {
                            fileIO->Remove(m_sliceAssetPath.c_str());
                        }
                    }
                    else
                    {
                        Internal::SaveSliceToDisk(m_sliceAssetPath.c_str(), m_sliceAssetBeforeBuffer);
                    }
                }

            protected:

                bool m_isNewAsset;                          ///< True if this SaveSliceToDisk command is creating a new asset (meaning Undo will remove the created file)
                AZStd::string m_sliceAssetPath;
                ByteBuffer m_sliceAssetBeforeBuffer;
                ByteBuffer m_sliceAssetAfterBuffer;
                SliceTransaction::Result m_redoResult;

                // DISABLE COPY
                SaveSliceToDiskCommand(const SaveSliceToDiskCommand& other) = delete;
                const SaveSliceToDiskCommand& operator= (const SaveSliceToDiskCommand& other) = delete;
            };

        } // namespace Internal

        //=========================================================================
        SliceTransaction::TransactionPtr SliceTransaction::BeginNewSlice(const char* name,
            AZ::SerializeContext* serializeContext,
            AZ::u32 sliceCreationFlags)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            if (!serializeContext)
            {
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

                if (!serializeContext)
                {
                    AZ_Assert(false, "Failed to retrieve serialize context.");
                    return TransactionPtr();
                }
            }

            TransactionPtr newTransaction = aznew SliceTransaction(serializeContext);

            AZ::Entity* entity = aznew AZ::Entity(name ? name : "Slice");

            // Create new empty slice asset.
            newTransaction->m_targetAsset = AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), newTransaction->m_targetAsset.GetAutoLoadBehavior());
            AZ::SliceComponent* component = entity->CreateComponent<AZ::SliceComponent>();
            component->SetIsDynamic(sliceCreationFlags & CreateAsDynamic);
            newTransaction->m_targetAsset.Get()->SetData(entity, component);

            newTransaction->m_transactionType = TransactionType::NewSlice;

            return newTransaction;
        }

        SliceTransaction::TransactionPtr SliceTransaction::BeginSliceOverwrite(const SliceAssetPtr& asset,  const AZ::SliceComponent& overwriteComponent, AZ::SerializeContext* serializeContext)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            if (!serializeContext)
            {
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Assert(serializeContext, "Failed to retrieve serialize context");
            }

            if (!asset || !asset.Get()->GetEntity() || !asset.Get()->GetComponent())
            {
                AZ_Error("SliceTransaction", false, "Target asset is not loaded. Ensure the asset is loaded before attempting a push transaction.");
                return TransactionPtr();
            }

            AZ::SliceAssetSerializationNotificationBus::Broadcast(&AZ::SliceAssetSerializationNotificationBus::Events::OnBeginSlicePush, asset.Get()->GetId());
            TransactionPtr newTransaction = aznew SliceTransaction(serializeContext);

            AZ::Entity* entity = aznew AZ::Entity(asset.Get()->GetEntity()->GetId(), asset.Get()->GetEntity()->GetName().c_str());

            newTransaction->m_originalTargetAsset = asset;
            newTransaction->m_targetAsset = { aznew AZ::SliceAsset(asset.GetId()), AZ::Data::AssetLoadBehavior::Default };
            newTransaction->m_transactionType = TransactionType::OverwriteSlice;
            entity->AddComponent(overwriteComponent.Clone(*serializeContext));

            newTransaction->m_targetAsset.Get()->SetData(entity, entity->FindComponent<AZ::SliceComponent>());

            return newTransaction;
        }
        //=========================================================================
        SliceTransaction::TransactionPtr SliceTransaction::BeginSlicePush(const SliceAssetPtr& asset,
            AZ::SerializeContext* serializeContext,
            AZ::u32 /*slicePushFlags*/)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            if (!serializeContext)
            {
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Assert(serializeContext, "Failed to retrieve serialize context.");
            }

            if (!asset || !asset.Get()->GetEntity() || !asset.Get()->GetComponent())
            {
                AZ_Error("SliceTransaction", false, "Target asset is not loaded. Ensure the asset is loaded before attempting a push transaction.");
                return TransactionPtr();
            }

            AZ::SliceAssetSerializationNotificationBus::Broadcast(&AZ::SliceAssetSerializationNotificationBus::Events::OnBeginSlicePush, asset.Get()->GetId());
            TransactionPtr newTransaction = aznew SliceTransaction(serializeContext);

            // Clone the asset in-memory for manipulation.
            AZ::Entity* entity = aznew AZ::Entity(asset.Get()->GetEntity()->GetId(), asset.Get()->GetEntity()->GetName().c_str());
            entity->AddComponent(asset.Get()->GetComponent()->Clone(*serializeContext));
            newTransaction->m_originalTargetAsset = asset;
            newTransaction->m_targetAsset = { aznew AZ::SliceAsset(asset.GetId()), AZ::Data::AssetLoadBehavior::Default };
            newTransaction->m_targetAsset.Get()->SetData(entity, entity->FindComponent<AZ::SliceComponent>());

            newTransaction->m_transactionType = TransactionType::UpdateSlice;

            return newTransaction;
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::UpdateEntity(AZ::Entity* entity)
        {
            if (!entity)
            {
                return AZ::Failure(AZStd::string::format("Null source entity for push."));
            }

            if (m_transactionType != TransactionType::UpdateSlice)
            {
                return AZ::Failure(AZStd::string::format("UpdateEntity() is only valid during push transactions, not creation transactions."));
            }

            // Given the asset we're targeting, identify corresponding ancestor for the live entity.
            const AZ::EntityId targetId = FindTargetAncestorAndUpdateInstanceIdMap(entity->GetId(), m_liveToAssetIdMap);
            if (targetId.IsValid())
            {
                m_entitiesToPush.emplace_back(targetId, entity->GetId());
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Unable to locate entity %s [%llu] in target slice.",
                    entity->GetName().c_str(), static_cast<AZ::u64>(entity->GetId())));
            }

            return AZ::Success();
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::UpdateEntity(const AZ::EntityId& entityId)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
            return UpdateEntity(entity);
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::UpdateEntityField(AZ::Entity* entity,
            const InstanceDataNode::Address& fieldNodeAddress)
        {
            if (!entity)
            {
                return AZ::Failure(AZStd::string::format("Null source entity for push."));
            }

            if (m_transactionType != TransactionType::UpdateSlice)
            {
                return AZ::Failure(AZStd::string::format("UpdateEntityField() is only valid during push transactions, not creation transactions."));
            }

            // Given the asset we're targeting, identify corresponding ancestor for the live entity.
            const AZ::EntityId targetId = FindTargetAncestorAndUpdateInstanceIdMap(entity->GetId(), m_liveToAssetIdMap);
            if (targetId.IsValid())
            {
                m_entitiesToPush.emplace_back(targetId, entity->GetId(), fieldNodeAddress);
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Unable to locate entity %s [%llu] in target slice.",
                    entity->GetName().c_str(), static_cast<AZ::u64>(entity->GetId())));
            }

            return AZ::Success();
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::UpdateEntityField(const AZ::EntityId& entityId,
            const InstanceDataNode::Address& fieldNodeAddress)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
            return UpdateEntityField(entity, fieldNodeAddress);
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::AddEntity(const AZ::Entity* entity, AZ::u32 addEntityFlags /* = 0 */)
        {
            if (!entity)
            {
                return AZ::Failure(AZStd::string::format("Invalid entity passed to AddEntity()."));
            }

            if (m_transactionType == TransactionType::None)
            {
                return AZ::Failure(AZStd::string::format("AddEntity() is only valid during during a transaction. This transaction may've already been committed."));
            }

            AZ::SliceComponent::SliceInstanceAddress sliceAddress;
            AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entity->GetId(),
                &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

            // When adding entities to existing slices, we need to resolve to the asset's entity Ids.
            if (m_transactionType == TransactionType::UpdateSlice)
            {
                // Walk up parent transform chain until we find an entity with a slice ancestor in the target slice.
                // If we don't find one, fail. We need an associated instance so we can fix up Id references.
                AZ::EntityId parentId;
                AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, entity->GetId(), &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
                AZ::EntityId ancestorId;
                AZ::SliceComponent::EntityIdToEntityIdMap liveToAssetIdMap;
                while (parentId.IsValid())
                {
                    liveToAssetIdMap.clear();
                    ancestorId = FindTargetAncestorAndUpdateInstanceIdMap(parentId, liveToAssetIdMap, &sliceAddress);
                    if (ancestorId.IsValid())
                    {
                        break;
                    }
                    
                    AZ::EntityId currentParentId = parentId;
                    parentId.SetInvalid();
                    AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, currentParentId, &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
                }

                if (!ancestorId.IsValid())
                {
                    return AZ::Failure(AZStd::string::format("Attempting to add an entity to an existing slice, but the entity could not be found in a hierarchy belonging to the target slice."));
                }

                for (const auto& idPair : liveToAssetIdMap)
                {
                    m_liveToAssetIdMap[idPair.first] = idPair.second;
                }
            }

            if (sliceAddress.IsValid() && !(addEntityFlags & SliceAddEntityFlags::DiscardSliceAncestry) && m_transactionType != TransactionType::OverwriteSlice)
            {
                // Add entity with its slice ancestry
                auto addedSliceInstanceIt = m_addedSliceInstances.find(sliceAddress);
                if (addedSliceInstanceIt == m_addedSliceInstances.end())
                {
                    // This slice instance hasn't been added to the transaction yet, add it
                    SliceTransaction::SliceInstanceToPush& instanceToPush = m_addedSliceInstances[sliceAddress];
                    instanceToPush.m_includeEntireInstance = false;
                    instanceToPush.m_instanceAddress = sliceAddress;
                    instanceToPush.m_entitiesToInclude.emplace(entity->GetId());

                    m_addedEntityIdRemaps[entity->GetId()] = entity->GetId();

                    for (const auto& mapPair : sliceAddress.GetInstance()->GetEntityIdMap())
                    {
                        // When making a NewSlice the entities used in its construction can be promoted into its first slice instance
                        // Because of this we want to map the asset EntityID of existing slice instances to a new asset EntityID since this mapping will be saved in the asset
                        // This new asset EntityID will then be pointed to the original EntityID of the instance entity that made it
                        // This completes the slice ancestry chain from the initial slice asset the instance came from to the new slice asset the instance is being placed into
                        // While the first live instance can retain the original EntityID when its moved into this deeper slice hierarchy
                        m_liveToAssetIdMap[mapPair.second] = m_transactionType == TransactionType::NewSlice ? AZ::Entity::MakeId() : mapPair.second;
                    }
                }
                else
                {
                    SliceTransaction::SliceInstanceToPush& instanceToPush = addedSliceInstanceIt->second;
                    if (!instanceToPush.m_includeEntireInstance)
                    {
                        instanceToPush.m_entitiesToInclude.insert(entity->GetId());
                        m_addedEntityIdRemaps[entity->GetId()] = entity->GetId();
                    }
                    else
                    {
                        // Adding a specific entity from a slice instance that is already
                        // being completely included, don't need to do anything (it'll already be covered)
                        return AZ::Success();
                    }
                }
            }
            else
            {
                // Add as loose entity; clone the entity and assign a new Id.
                AZ::Entity* clonedEntity = m_serializeContext->CloneObject(entity);
                clonedEntity->SetId(AZ::Entity::MakeId());
                m_liveToAssetIdMap[entity->GetId()] = clonedEntity->GetId();
                m_addedEntityIdRemaps[entity->GetId()] = clonedEntity->GetId();

                m_targetAsset.Get()->GetComponent()->AddEntity(clonedEntity);
            }

            m_hasEntityAdds = true;

            return AZ::Success();
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::AddEntity(AZ::EntityId entityId, AZ::u32 addEntityFlags /* = 0 */)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
            return AddEntity(entity, addEntityFlags);
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::AddSliceInstance(const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
        {
            if (!sliceAddress.IsValid())
            {
                return AZ::Failure(AZStd::string::format("Invalid slice instance address passed to AddSliceInstance()."));
            }

            if (m_transactionType == TransactionType::None)
            {
                return AZ::Failure(AZStd::string::format("AddSliceInstance() is only valid during during a transaction. This transaction may've already been committed."));
            }

            auto addedSliceInstanceIt = m_addedSliceInstances.find(sliceAddress);
            if (addedSliceInstanceIt == m_addedSliceInstances.end())
            {
                // This slice instance hasn't been added to the transaction yet, add it
                SliceTransaction::SliceInstanceToPush& instanceToPush = m_addedSliceInstances[sliceAddress];
                instanceToPush.m_includeEntireInstance = true;
                instanceToPush.m_instanceAddress = sliceAddress;
            }
            else
            {
                SliceTransaction::SliceInstanceToPush& instanceToPush = addedSliceInstanceIt->second;
                if (instanceToPush.m_includeEntireInstance)
                {
                    return AZ::Failure(AZStd::string::format("Slice instance has already been added to the transaction."));
                }
                else
                {
                    // Transaction already has had individual entities from this slice instance added to it, so we just convert
                    // that entry to include all entities
                    instanceToPush.m_includeEntireInstance = true;
                }
            }

            for (const auto& mapPair : sliceAddress.GetInstance()->GetEntityIdMap())
            {
                // We keep the entity ids in the source instances, so our live Id will match the one we write to the asset.
                m_liveToAssetIdMap[mapPair.second] = mapPair.second;
                m_addedEntityIdRemaps[mapPair.second] = mapPair.second;
            }

            m_hasEntityAdds = true;

            return AZ::Success();
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::RemoveEntity(AZ::Entity* entity)
        {
            if (!entity)
            {
                return AZ::Failure(AZStd::string::format("Invalid entity passed to RemoveEntity()."));
            }

            return RemoveEntity(entity->GetId());
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::RemoveEntity(AZ::EntityId entityId)
        {
            if (!entityId.IsValid())
            {
                return AZ::Failure(AZStd::string::format("Invalid entity Id passed to RemoveEntity()."));
            }

            if (m_transactionType != TransactionType::UpdateSlice)
            {
                return AZ::Failure(AZStd::string::format("RemoveEntity() is only valid during during a push transaction."));
            }

            // The user needs to provide the entity as it exists in the target asset, since we can't resolve deleted entities.
            // so the caller isn't required to in that case.
            m_entitiesToRemove.push_back(entityId);

            return AZ::Success();
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::Commit(const char* fullPath,
            SliceTransaction::PreSaveCallback preSaveCallback,
            SliceTransaction::PostSaveCallback postSaveCallback,
            AZ::u32 sliceCommitFlags)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // Clone asset for final modifications and save.
            // This also releases borrowed entities and slice instances.
            SliceAssetPtr finalAsset = CloneAssetForSave();

            // Check out target asset.
            {
                using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;

                bool checkedOutSuccessfully = false;
                ApplicationBus::BroadcastResult(checkedOutSuccessfully, &ApplicationBus::Events::CheckSourceControlConnectionAndRequestEditForFileBlocking,
                    fullPath, "Checking out for edit...", ApplicationBus::Events::RequestEditProgressCallback());

                if (!checkedOutSuccessfully)
                {
                    return AZ::Failure(AZStd::string::format("Unable to checkout target file \"%s\".", fullPath));
                }
            }

            // Process the transaction.
            switch (m_transactionType)
            {
            case TransactionType::NewSlice:
            case TransactionType::OverwriteSlice:
            {
                // No additional work required; slice asset is populated.
            }
            break;

            case TransactionType::UpdateSlice:
            {
                AZ::SliceComponent* sliceAsset = finalAsset.Get()->GetComponent();

                // Remove any requested entities from the slice.
                for (const AZ::EntityId& removeId : m_entitiesToRemove)
                {
                    // Find the entity's ancestor in the target asset.
                    if (!sliceAsset->RemoveEntity(removeId))
                    {
                        return AZ::Failure(AZStd::string::format("Unable to remove entity [%llu] from target slice.", static_cast<AZ::u64>(removeId)));
                    }
                }

                // Loop through each field to push, generate an InstanceDataHierarchy for the source entity, and synchronize the field data to the target.
                // We can combine with the above loop, but organizing in two passes makes the processes clearer.
                for (const EntityToPush& entityToPush : m_entitiesToPush)
                {
                    AZ::Entity* sourceEntity = nullptr;
                    if (entityToPush.m_sourceEntityId != entityToPush.m_targetEntityId)
                    {
                        AZ::ComponentApplicationBus::BroadcastResult(sourceEntity, &AZ::ComponentApplicationBus::Events::FindEntity, entityToPush.m_sourceEntityId);
                    }
                    else
                    {
                        sourceEntity = sliceAsset->FindEntity(entityToPush.m_sourceEntityId);
                    }

                    if (!sourceEntity)
                    {
                        return AZ::Failure(AZStd::string::format("Unable to locate source entity with id %s for slice data push. It was not found in the slice, or an instance of the slice.",
                            entityToPush.m_sourceEntityId.ToString().c_str()));
                    }

                    AZ::Entity* targetEntity = sliceAsset->FindEntity(entityToPush.m_targetEntityId);
                    if (!targetEntity)
                    {
                        return AZ::Failure(AZStd::string::format("Unable to locate entity with Id %llu in the target slice.",
                            static_cast<AZ::u64>(entityToPush.m_targetEntityId)));
                    }

                    InstanceDataHierarchy targetHierarchy;
                    targetHierarchy.AddRootInstance<AZ::Entity>(targetEntity);
                    targetHierarchy.Build(m_serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

                    InstanceDataHierarchy sourceHierarchy;
                    sourceHierarchy.AddRootInstance<AZ::Entity>(sourceEntity);
                    sourceHierarchy.Build(m_serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

                    const InstanceDataNode* sourceNode = &sourceHierarchy;
                    InstanceDataNode* targetNode = &targetHierarchy;

                    // If we're pushing a specific field, resolve the corresponding nodes in both hierarchies.
                    InstanceDataNode::Address elementAddress;
                    if (!entityToPush.m_fieldNodeAddress.empty())
                    {
                        sourceNode = sourceHierarchy.FindNodeByAddress(entityToPush.m_fieldNodeAddress);
                        targetNode = targetHierarchy.FindNodeByAddress(entityToPush.m_fieldNodeAddress);

                        // If the node is a container element, we push at the container level but filter by the element.
                        if (sourceNode && !targetNode)
                        {
                            // Element exists in the source, but not the target. We want to add it to the target.
                            elementAddress = entityToPush.m_fieldNodeAddress;

                            // Recurse up trying to find the first matching source/target node
                            // This is necessary anytime we're trying to push a node that requires more than just a leaf node be added
                            while (sourceNode && !targetNode)
                            {
                                sourceNode = sourceNode->GetParent();
                                if (sourceNode)
                                {
                                    targetNode = targetHierarchy.FindNodeByAddress(sourceNode->ComputeAddress());
                                }
                            }
                        }
                        else if (targetNode && !sourceNode)
                        {
                            // Element exists in the target, but not the source. We want to remove it from the target.
                            elementAddress = entityToPush.m_fieldNodeAddress;
                            targetNode = targetNode->GetParent();
                            sourceNode = sourceHierarchy.FindNodeByAddress(targetNode->ComputeAddress());
                        }
                    }

                    if (!sourceNode)
                    {
                        return AZ::Failure(AZStd::string::format("Unable to locate source data node for slice push."));
                    }
                    if (!targetNode)
                    {
                        return AZ::Failure(AZStd::string::format("Unable to locate target data node for slice push."));
                    }

                    bool copyResult = InstanceDataHierarchy::CopyInstanceData(sourceNode, targetNode, m_serializeContext, nullptr, nullptr, elementAddress);
                    if (!copyResult)
                    {
                        return AZ::Failure(AZStd::string::format("Unable to push data node to target for slice push."));
                    }
                }
            }
            break;

            default:
            {
                return AZ::Failure(AZStd::string::format("Transaction cannot be committed because it was never started."));
            }
            break;
            }

            Result result = PreSave(fullPath, finalAsset, preSaveCallback, sliceCommitFlags);
            if (!result)
            {
                return AZ::Failure(AZStd::string::format("Pre-save callback reported failure:\n%s", result.TakeError().c_str()));
            }

            // Save slice to disk
            const bool disableUndoCapture = sliceCommitFlags & SliceCommitFlags::DisableUndoCapture;
            if (disableUndoCapture)
            {
                AZStd::vector<AZ::u8> sliceBuffer;
                AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > sliceStream(&sliceBuffer);
                AZ::Utils::SaveObjectToStream(sliceStream, GetSliceStreamFormat(), finalAsset.Get()->GetEntity());
                result = Internal::SaveSliceToDisk(fullPath, sliceBuffer, m_serializeContext);
            }
            else
            {
                ScopedUndoBatch undoBatch("SliceTransaction SaveSliceToDisk");

                Internal::SaveSliceToDiskCommand* saveCommand = aznew Internal::SaveSliceToDiskCommand("SaveSliceToDisk");
                saveCommand->SetParent(undoBatch.GetUndoBatch());
                saveCommand->Capture(m_originalTargetAsset, finalAsset, fullPath);
                saveCommand->RunRedo();
                result = saveCommand->GetRedoResult();
            }
            if (!result)
            {
                return AZ::Failure(AZStd::string::format("Slice asset could not be saved to disk.\n\nAsset path: %s \n\nDetails: %s", fullPath, result.TakeError().c_str()));
            }

            if (postSaveCallback)
            {
                postSaveCallback(TransactionPtr(this), fullPath, finalAsset);
            }

            AZ::SliceAssetSerializationNotificationBus::Broadcast(&AZ::SliceAssetSerializationNotificationBus::Events::OnEndSlicePush, m_originalTargetAsset.Get()->GetId(), finalAsset.Get()->GetId());
            // Reset the transaction.
            Reset();
            return AZ::Success();
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::Commit(const AZ::Data::AssetId& targetAssetId,
            SliceTransaction::PreSaveCallback preSaveCallback,
            SliceTransaction::PostSaveCallback postSaveCallback,
            AZ::u32 sliceCommitFlags)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZStd::string sliceAssetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, targetAssetId);
            if (sliceAssetPath.empty())
            {
                return AZ::Failure(AZStd::string::format("Failed to resolve path for slice asset %s. Aborting slice push. No assets have been affected.",
                    targetAssetId.ToString<AZStd::string>().c_str()));
            }

            bool fullPathFound = false;
            AZStd::string assetFullPath;
            AssetSystemRequestBus::BroadcastResult(fullPathFound, &AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, sliceAssetPath, assetFullPath);

            if (!fullPathFound)
            {
                assetFullPath = AZStd::string::format("@projectroot@/%s", sliceAssetPath.c_str());
            }

            return Commit(assetFullPath.c_str(), preSaveCallback, postSaveCallback, sliceCommitFlags);
        }

        //=========================================================================
        const AZ::SliceComponent::EntityIdToEntityIdMap& SliceTransaction::GetLiveToAssetEntityIdMap() const
        {
            return m_liveToAssetIdMap;
        }

        bool SliceTransaction::AddLiveToAssetEntityIdMapping(const AZStd::pair<AZ::EntityId, AZ::EntityId>& mapping)
        {
            return m_liveToAssetIdMap.emplace(mapping).second;
        }

        const AZ::SliceComponent::EntityIdToEntityIdMap& SliceTransaction::GetAddedEntityIdRemaps() const
        {
            return m_addedEntityIdRemaps;
        }

        //=========================================================================
        SliceTransaction::SliceTransaction(AZ::SerializeContext* serializeContext)
            : m_transactionType(SliceTransaction::TransactionType::None)
            , m_refCount(0)
        {
            if (!serializeContext)
            {
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Assert(serializeContext, "No serialize context was provided, and none could be found.");
            }

            m_serializeContext = serializeContext;
        }

        //=========================================================================
        SliceTransaction::~SliceTransaction()
        {
        }

        //=========================================================================
        SliceTransaction::SliceAssetPtr SliceTransaction::CloneAssetForSave()
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // Move included slice instances to the target asset temporarily so that they are included in the clone
            for (auto& addedSliceInstanceIt : m_addedSliceInstances)
            {
                SliceTransaction::SliceInstanceToPush& instanceToPush = addedSliceInstanceIt.second;
                instanceToPush.m_instanceAddress = m_targetAsset.Get()->GetComponent()->AddSliceInstance(instanceToPush.m_instanceAddress.GetReference(), instanceToPush.m_instanceAddress.GetInstance());
            }

            // Clone the asset.
            AZ::Entity* finalSliceEntity = aznew AZ::Entity(m_targetAsset.Get()->GetEntity()->GetId(), m_targetAsset.Get()->GetEntity()->GetName().c_str());
            AZ::SliceComponent::SliceInstanceToSliceInstanceMap sourceToCloneSliceInstanceMap;
            finalSliceEntity->AddComponent(m_targetAsset.Get()->GetComponent()->Clone(*m_serializeContext, &sourceToCloneSliceInstanceMap));
            AZ::Data::Asset<AZ::SliceAsset> finalAsset = AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), AZ::Data::AssetLoadBehavior::Default);
            finalAsset.Get()->SetData(finalSliceEntity, finalSliceEntity->FindComponent<AZ::SliceComponent>());

            // Clean up the cloned slice instances before save
            AZStd::vector<AZ::Entity*> entitiesToDelete;
            for (const auto& addedSliceInstanceIt : m_addedSliceInstances)
            {
                const SliceTransaction::SliceInstanceToPush& instanceToPush = addedSliceInstanceIt.second;
                AZ::SliceComponent::SliceInstanceAddress& finalAssetSliceInstance = sourceToCloneSliceInstanceMap[instanceToPush.m_instanceAddress];

                // For slice instances added that should only contain specified entities, cull the undesired entities from final asset
                if (!instanceToPush.m_includeEntireInstance)
                {
                    const AZ::SliceComponent::InstantiatedContainer* finalAssetInstantiatedContainer = finalAssetSliceInstance.GetInstance()->GetInstantiated();
                    for (AZ::Entity* finalAssetEntity : finalAssetInstantiatedContainer->m_entities)
                    {
                        AZ::EntityId finalAssetEntityId = finalAssetEntity->GetId();
                        auto foundIt = instanceToPush.m_entitiesToInclude.find(finalAssetEntityId);
                        if (foundIt == instanceToPush.m_entitiesToInclude.end())
                        {
                            entitiesToDelete.push_back(finalAssetEntity);
                        }
                    }

                    for (AZ::Entity* entityToDelete : entitiesToDelete)
                    {
                        finalAsset.Get()->GetComponent()->RemoveEntity(entityToDelete);
                    }
                    entitiesToDelete.clear();
                }

                // Added slice instances are cloned with a mapping from their "Asset" entity ID to an existing "Live" EntityID in an owning Entity Context
                // Before we save out the added instance we want to remap its EntityIdMap away from these "Live" EntityIDs
                // This is so the resulting slice ancestry of the asset does not reference the "Live" slice instance entities that contributed to the clone
                // This is important because these same "Live" instance entities can be moved into the first slice instance of our NewSlice
                // Leading to a double entry in the slice ancestry mapping chain
                if (m_transactionType == TransactionType::NewSlice)
                {
                    AZ::SliceComponent::EntityIdToEntityIdMap& finalAssetSliceInstanceEntityMap = finalAssetSliceInstance.GetInstance()->GetEntityIdMapForEdit();

                    for (AZStd::pair<AZ::EntityId, AZ::EntityId>& finalAssetSliceInstanceEntityMapping : finalAssetSliceInstanceEntityMap)
                    {
                        auto hasMapping = m_liveToAssetIdMap.find(finalAssetSliceInstanceEntityMapping.second);
                        if (hasMapping != m_liveToAssetIdMap.end())
                        {
                            finalAssetSliceInstanceEntityMapping.second = hasMapping->second;
                        }
                    }
                }
            }

            // Return borrowed slice instances that are no longer needed post-clone.
            // This will transfer them back to the editor entity context.
            {
                using namespace AzFramework;

                for (auto& addedSliceInstanceIt : m_addedSliceInstances)
                {
                    SliceTransaction::SliceInstanceToPush& instanceToPush = addedSliceInstanceIt.second;
                    const AZ::SliceComponent::InstantiatedContainer* instantiated = instanceToPush.m_instanceAddress.GetInstance()->GetInstantiated();
                    if (instantiated && !instantiated->m_entities.empty())
                    {
                        // Get the entity context owning this entity, and give back the slice instance.
                        EntityContextId owningContextId = EntityContextId::CreateNull();
                        EntityIdContextQueryBus::EventResult(owningContextId, instantiated->m_entities.front()->GetId(), &EntityIdContextQueries::GetOwningContextId);
                        if (!owningContextId.IsNull())
                        {
                            AZ::SliceComponent* rootSlice = nullptr;
                            SliceEntityOwnershipServiceRequestBus::EventResult(rootSlice, owningContextId,
                                &SliceEntityOwnershipServiceRequestBus::Events::GetRootSlice);
                            if (rootSlice)
                            {
                                rootSlice->AddSliceInstance(instanceToPush.m_instanceAddress.GetReference(), instanceToPush.m_instanceAddress.GetInstance());
                            }
                            else
                            {
                                AZ_Error("SliceTransaction", false, "Failed to get root slice of context for entity being added, slice instance will be lost.");
                            }
                        }
                        else
                        {
                            AZ_Error("SliceTransaction", false, "Failed to get owning context id for entity being added, slice instance will be lost.");
                        }
                    }
                }
            }

            return finalAsset;
        }

        //=========================================================================
        SliceTransaction::Result SliceTransaction::PreSave(const char* fullPath, SliceAssetPtr& asset, PreSaveCallback preSaveCallback, AZ::u32 /*sliceCommitFlags*/)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // Remap live Ids back to those of the asset.
            AZ::EntityUtils::SerializableEntityContainer assetEntities;
            asset.Get()->GetComponent()->GetEntities(assetEntities.m_entities);
            asset.Get()->GetComponent()->GetAllMetadataEntities(assetEntities.m_entities);

            AZ::IdUtils::Remapper<AZ::EntityId>::ReplaceIdsAndIdRefs(&assetEntities,
                [this](const AZ::EntityId& originalId, bool /*isEntityId*/, const AZStd::function<AZ::EntityId()>& /*idGenerator*/) -> AZ::EntityId
                    {
                        auto findIter = m_liveToAssetIdMap.find(originalId);
                        if (findIter != m_liveToAssetIdMap.end())
                        {
                            return findIter->second;
                        }

                        return originalId;
                    }, 
                m_serializeContext);

            // Invoke user pre-save callback.
            if (preSaveCallback)
            {
                Result preSaveResult = preSaveCallback(TransactionPtr(this), fullPath, asset);
                if (!preSaveResult)
                {
                    return preSaveResult;
                }
            }

            return AZ::Success();
        }

        //=========================================================================
        AZ::EntityId SliceTransaction::FindTargetAncestorAndUpdateInstanceIdMap(AZ::EntityId entityId, AZ::SliceComponent::EntityIdToEntityIdMap& liveToAssetIdMap, const AZ::SliceComponent::SliceInstanceAddress* ignoreSliceInstance) const
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZ::SliceComponent* slice = m_targetAsset.Get()->GetComponent();

            if (slice->FindEntity(entityId))
            {
                // Entity is already within the asset (not a live entity as part of an instance).
                return entityId;
            }

            // Entity is live entity, and we need to resolve the appropriate ancestor target.
            AZ::SliceComponent::SliceInstanceAddress instanceAddr;
            AzFramework::SliceEntityRequestBus::EventResult(instanceAddr, entityId,
                &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);
            if (!instanceAddr.IsValid()) // entityId here could be a newly added loose entity, hence doesn't belong to any slice instance.
            {   
                return AZ::EntityId();
            }

            const bool entityIsFromIgnoredSliceInstance = ignoreSliceInstance && ignoreSliceInstance->IsValid() && ignoreSliceInstance->GetReference()->GetSliceAsset().GetId() == instanceAddr.GetReference()->GetSliceAsset().GetId();
            if (!entityIsFromIgnoredSliceInstance)
            {
                bool foundTargetAncestor = false;

                const AZ::SliceComponent::EntityList& entitiesInInstance = instanceAddr.GetInstance()->GetInstantiated()->m_entities;

                // For every entity in the instance, get ancestry, and walk up the chain until we find
                // the ancestor corresponding to the target asset, building a fully resolved id map along the way.
                AZ::SliceComponent::EntityAncestorList ancestors;
                for (const AZ::Entity* entityInInstance : entitiesInInstance)
                {
                    ancestors.clear();
                    instanceAddr.GetReference()->GetInstanceEntityAncestry(entityInInstance->GetId(), ancestors, std::numeric_limits<AZ::u32>::max());
                    for (const AZ::SliceComponent::Ancestor& ancestor : ancestors)
                    {
                        auto& reverseIdMap = ancestor.m_sliceAddress.GetInstance()->GetEntityIdToBaseMap();
                        auto idIter = liveToAssetIdMap.find(entityInInstance->GetId());
                        if (idIter != liveToAssetIdMap.end())
                        {
                            auto reverseIdIter = reverseIdMap.find(idIter->second);
                            if (reverseIdIter != reverseIdMap.end())
                            {
                                liveToAssetIdMap[entityInInstance->GetId()] = reverseIdIter->second;
                            }
                        }
                        else
                        {
                            auto reverseIdIter = reverseIdMap.find(entityInInstance->GetId());
                            if (reverseIdIter != reverseIdMap.end())
                            {
                                liveToAssetIdMap[entityInInstance->GetId()] = reverseIdIter->second;
                            }
                        }

                        if (ancestor.m_sliceAddress.GetReference()->GetSliceAsset().GetId() == m_targetAsset.GetId())
                        {
                            // Found the target asset, so we've resolved the final target Id for this entity.
                            foundTargetAncestor = true;
                            break;
                        }
                    }
                }

                auto findEntityIter = liveToAssetIdMap.find(entityId);
                if (findEntityIter == liveToAssetIdMap.end())
                {
                    return AZ::EntityId();
                }

                AZ_Error("SliceTransaction", foundTargetAncestor,
                         "Failed to locate ancestor in target asset for entity [%llu]. Some Id references may not be updated.",
                         entityId);

                return findEntityIter->second;
            }

            return AZ::EntityId();
        }

        //=========================================================================
        void SliceTransaction::Reset()
        {
            m_transactionType = TransactionType::None;
            m_serializeContext = nullptr;
            m_targetAsset.Reset();
            m_addedSliceInstances.clear();
            m_liveToAssetIdMap.clear();
            m_entitiesToPush.clear();
            m_entitiesToRemove.clear();
            m_addedEntityIdRemaps.clear();
        }

        //=========================================================================
        void SliceTransaction::add_ref()          
        { 
            ++m_refCount; 
        }

        //=========================================================================
        void SliceTransaction::release()
        {
            if (--m_refCount == 0)
            {
                delete this;
            }
        }

        //=========================================================================
        namespace Internal
        {
            //=========================================================================
            AZStd::string MakeTemporaryFilePathForSave(const char* fullPath)
            {
                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                AZ_Assert(fileIO, "File IO is not initialized.");

                AZStd::string devAssetPath = fileIO->GetAlias("@projectroot@");
                AZStd::string userPath = fileIO->GetAlias("@user@");
                AZStd::string tempPath = fullPath;
                EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, devAssetPath);
                EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, userPath);
                EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, tempPath);
                AzFramework::StringFunc::Replace(tempPath, "@projectroot@", devAssetPath.c_str());
                AzFramework::StringFunc::Replace(tempPath, devAssetPath.c_str(), userPath.c_str());
                tempPath.append(".slicetemp");

                return tempPath;
            }

            //=========================================================================
            SliceTransaction::Result SaveSliceToDisk(const char* targetPath, AZStd::vector<AZ::u8>& sliceAssetEntityMemoryBuffer, AZ::SerializeContext* serializeContext)
            {
                AZ_PROFILE_FUNCTION(AzToolsFramework);

                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                AZ_Assert(fileIO, "File IO is not initialized.");

                if (!serializeContext)
                {
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    AZ_Assert(serializeContext, "Failed to retrieve serialize context.");
                }

                // Write to a temporary location, and later move to the target location.
                const AZStd::string tempFilePath = MakeTemporaryFilePathForSave(targetPath);

                AZ::IO::FileIOStream fileStream(tempFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);
                if (fileStream.IsOpen())
                {
                    AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > memoryStream(&sliceAssetEntityMemoryBuffer);

                    // Write the in-memory copy to file
                    bool savedToFile;
                    {
                        AZ_PROFILE_SCOPE(AzToolsFramework, "SliceUtilities::Internal::SaveSliceToDisk:SaveToFileStream");
                        memoryStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
                        savedToFile = fileStream.Write(memoryStream.GetLength(), memoryStream.GetData()->data()) != 0;
                    }
                    fileStream.Close();

                    if (savedToFile)
                    {
                        AZ_PROFILE_SCOPE(AzToolsFramework, "SliceUtilities::Internal::SaveSliceToDisk:TempToTargetFileReplacement");

                        // Copy scratch file to target location.
                        const bool targetFileExists = fileIO->Exists(targetPath);

                        bool removedTargetFile;
                        {
                            AZ_PROFILE_SCOPE(AzToolsFramework, "SliceUtilities::Internal::SaveSliceToDisk:TempToTargetFileReplacement:RemoveTarget");
                            removedTargetFile = fileIO->Remove(targetPath);
                        }

                        if (targetFileExists && !removedTargetFile)
                        {
                            return AZ::Failure(AZStd::string::format("Unable to modify existing target slice file. Please make the slice writeable and try again."));
                        }

                        {
                            AZ_PROFILE_SCOPE(AzToolsFramework, "SliceUtilities::Internal::SaveSliceToDisk:TempToTargetFileReplacement:RenameTempFile");
                            AZ::IO::Result renameResult = fileIO->Rename(tempFilePath.c_str(), targetPath);
                            if (!renameResult)
                            {
                                return AZ::Failure(AZStd::string::format("Unable to move temporary slice file \"%s\" to target location.", tempFilePath.c_str()));
                            }
                        }

                        // Bump the slice asset up in the asset processor's queue.
                        {
                            AZ_PROFILE_SCOPE(AzToolsFramework, "SliceUtilities::Internal::SaveSliceToDisk:TempToTargetFileReplacement:GetAssetStatus");
                            EBUS_EVENT(AzFramework::AssetSystemRequestBus, EscalateAssetBySearchTerm, targetPath);
                        }
                        return AZ::Success();
                    }
                    else
                    {
                        return AZ::Failure(AZStd::string::format("Unable to save slice to a temporary file at location: \"%s\".", tempFilePath.c_str()));
                    }
                }
                else
                {
                    return AZ::Failure(AZStd::string::format("Unable to create temporary slice file at location: \"%s\".", tempFilePath.c_str()));
                }
            }

        } // namespace Internal

    } // namespace SliceUtilities

} // namespace AzToolsFramework
