/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/std/parallel/thread.h>

#include <QMessageBox>
#include <QApplication>

namespace SerializeHelpers
{
    bool s_initializedReflection = false;

    //! Simple helper class for serializing a vector of entities, their child entities
    //! and their slice instance information. This is only serialized for the undo system
    //! or the clipboard so it does not require version conversion.
    //! m_entities is the set of entities that were chosen to be serialized (e.g. by a copy
    //! command), m_childEntities are all the descendants of the entities in m_entities.
    class SerializedElementContainer
    {
    public:
        virtual ~SerializedElementContainer() { }
        AZ_CLASS_ALLOCATOR(SerializedElementContainer, AZ::SystemAllocator);
        AZ_RTTI(SerializedElementContainer, "{4A12708F-7EC5-4F56-827A-6E67C3C49B3D}");
        AZStd::vector<AZ::Entity*> m_entities;
        AZStd::vector<AZ::Entity*> m_childEntities;
        EntityRestoreVec m_entityRestoreInfos;
        EntityRestoreVec m_childEntityRestoreInfos;
    };


    namespace Internal
    {
        void DetachEntitiesIfFullSliceInstanceNotBeingCopied(SerializedElementContainer& entitiesToSerialize)
        {
            // We simplify this a bit in the same was as SandboxIntegrationManager::CloneSelection and instead say that,
            // unless every entity in the slice instance is being copied we do not preserve the connection to the slice.

            // make a set of all the entities in entitiesToSerialize
            AZStd::unordered_set<AZ::EntityId> allEntitiesBeingCopied;
            for (auto entity : entitiesToSerialize.m_entities)
            {
                allEntitiesBeingCopied.insert(entity->GetId());
            }
            for (auto entity : entitiesToSerialize.m_childEntities)
            {
                allEntitiesBeingCopied.insert(entity->GetId());
            }

            // Create a local function to avoid duplicating code because we have two sets of lists to process
            auto CheckEntities = [allEntitiesBeingCopied](AZStd::vector<AZ::Entity*>& entities, EntityRestoreVec& entityRestoreInfos)
            {
                for (int i = 0; i < entities.size(); ++i)
                {
                    AZ::Entity* entity = entities[i];

                    AZ::SliceComponent::SliceInstanceAddress sliceAddress;
                    AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entity->GetId(),
                        &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                    if (sliceAddress.IsValid())
                    {
                        const AZ::SliceComponent::EntityList& entitiesInSlice = sliceAddress.GetInstance()->GetInstantiated()->m_entities;
                        for (AZ::Entity* entityInSlice : entitiesInSlice)
                        {
                            if (allEntitiesBeingCopied.end() == allEntitiesBeingCopied.find(entityInSlice->GetId()))
                            {
                                // at least one of the entities in the slice instance is not in the set being copied so
                                // remove this entities connection to the slice
                                entityRestoreInfos[i].m_assetId.SetInvalid();
                                break;
                            }
                        }
                    }
                }
            };

            CheckEntities(entitiesToSerialize.m_entities, entitiesToSerialize.m_entityRestoreInfos);
            CheckEntities(entitiesToSerialize.m_childEntities, entitiesToSerialize.m_childEntityRestoreInfos);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void InitializeReflection()
    {
        // Reflect the SerializedElementContainer on first use.
        if (!s_initializedReflection)
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "No serialize context");

            context->Class<SerializedElementContainer>()
                ->Version(1)
                ->Field("Entities", &SerializedElementContainer::m_entities)
                ->Field("ChildEntities", &SerializedElementContainer::m_childEntities)
                ->Field("RestoreInfos", &SerializedElementContainer::m_entityRestoreInfos)
                ->Field("ChildRestoreInfos", &SerializedElementContainer::m_childEntityRestoreInfos);

            s_initializedReflection = true;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void RestoreSerializedElements(
        AZ::EntityId canvasEntityId,
        AZ::Entity*  parent,
        AZ::Entity*  insertBefore,
        UiEditorEntityContext* entityContext,
        const AZStd::string& xml,
        bool isCopyOperation,
        LyShine::EntityArray* cumulativeListOfCreatedEntities)
    {
        LyShine::EntityArray listOfNewlyCreatedTopLevelElements;
        LyShine::EntityArray listOfAllCreatedEntities;
        EntityRestoreVec entityRestoreInfos;

        LoadElementsFromXmlString(
            canvasEntityId,
            xml.c_str(),
            isCopyOperation,
            parent,
            insertBefore,
            listOfNewlyCreatedTopLevelElements,
            listOfAllCreatedEntities,
            entityRestoreInfos);

        if (listOfNewlyCreatedTopLevelElements.empty())
        {
            // This happens when the serialization version numbers DON'T match.
            QMessageBox(QMessageBox::Critical,
                "Error",
                QString("Failed to restore elements. The clipboard serialization format is incompatible."),
                QMessageBox::Ok, QApplication::activeWindow()).exec();

            // Nothing more to do.
            return;
        }

        // This is for error handling only. In the case of an error RestoreSliceEntity will delete the
        // entity. We need to know when this has happened. So we record all the entityIds here and check
        // them afterwards.
        AzToolsFramework::EntityIdList idsOfNewlyCreatedTopLevelElements;
        for (auto entity : listOfNewlyCreatedTopLevelElements)
        {
            idsOfNewlyCreatedTopLevelElements.push_back(entity->GetId());
        }

        // Now we need to restore the slice info for all the created elements
        // In the case of a copy operation we need to generate new sliceInstanceIds. We use a map so that
        // all entities copied from the same slice instance will end up in the same new slice instance.
        AZStd::unordered_map<AZ::SliceComponent::SliceInstanceId, AZ::SliceComponent::SliceInstanceId> sliceInstanceMap;
        for (int i=0; i < listOfAllCreatedEntities.size(); ++i)
        {
            AZ::Entity* entity = listOfAllCreatedEntities[i];

            AZ::SliceComponent::EntityRestoreInfo& sliceRestoreInfo = entityRestoreInfos[i];

            if (sliceRestoreInfo)
            {
                if (isCopyOperation)
                {
                    // if a copy we can't use the instanceId of the instance that was copied from so generate
                    // a new one - but only want one new id per original slice instance - so we use a map to
                    // keep track of which instance Ids we have created new Ids for.
                    auto iter = sliceInstanceMap.find(sliceRestoreInfo.m_instanceId);
                    if (iter == sliceInstanceMap.end())
                    {
                        sliceInstanceMap[sliceRestoreInfo.m_instanceId] = AZ::SliceComponent::SliceInstanceId::CreateRandom();
                    }

                    sliceRestoreInfo.m_instanceId = sliceInstanceMap[sliceRestoreInfo.m_instanceId];
                }

                UiEditorEntityContextRequestBus::Event(
                    entityContext->GetContextId(), &UiEditorEntityContextRequestBus::Events::RestoreSliceEntity, entity, sliceRestoreInfo);
            }
            else
            {
                entityContext->AddUiEntity(entity);
            }
        }

        // If we are restoring slice entities and any of the entities are the first to be using that slice
        // then we have to wait for that slice to be reloaded. We need to wait because we can't create hierarchy
        // items for entities before they are recreated. We have tried trying to solve this by deferring the creation
        // of the hierarchy items on a queue but that gets really complicated because this function is called
        // in several situations. It also seems problematic to return control to the user - they could add or
        // delete more items while we are waiting for the assets to load.
        if (AZ::Data::AssetManager::IsReady())
        {
            bool areRequestsPending = false;
            UiEditorEntityContextRequestBus::EventResult(
                areRequestsPending, entityContext->GetContextId(), &UiEditorEntityContextRequestBus::Events::HasPendingRequests);
            while (areRequestsPending)
            {
                AZ::Data::AssetManager::Instance().DispatchEvents();
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(50));
                UiEditorEntityContextRequestBus::EventResult(
                    areRequestsPending, entityContext->GetContextId(), &UiEditorEntityContextRequestBus::Events::HasPendingRequests);
            }
        }

        // Because RestoreSliceEntity can delete the entity we have some recovery code here that will
        // create a new list of top level entities excluding any that have been removed.
        // An error should already have been reported in this case so we don't report it again.
        LyShine::EntityArray validatedListOfNewlyCreatedTopLevelElements;
        for (auto entityId : idsOfNewlyCreatedTopLevelElements)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

            // Only add it to the validated list if the entity still exists
            if (entity)
            {
                validatedListOfNewlyCreatedTopLevelElements.push_back(entity);
            }
        }

        // Fixup the created entities, we do this before adding the top level element to the parent so that
        // MakeUniqueChileName works correctly
        UiCanvasBus::Event(
            canvasEntityId,
            &UiCanvasBus::Events::FixupCreatedEntities,
            validatedListOfNewlyCreatedTopLevelElements,
            isCopyOperation,
            parent);

        // Now add the top-level created elements as children of the parent
        for (auto entity : validatedListOfNewlyCreatedTopLevelElements)
        {
            // add this new entity as a child of the parent (insertionPoint or root)
            UiCanvasBus::Event(canvasEntityId, &UiCanvasBus::Events::AddElement, entity, parent, insertBefore);
        }

        // if a list of entities was passed then add all the entities that we added
        // to the list
        if (cumulativeListOfCreatedEntities)
        {
            cumulativeListOfCreatedEntities->insert(
                        cumulativeListOfCreatedEntities->end(),
                        validatedListOfNewlyCreatedTopLevelElements.begin(),
                        validatedListOfNewlyCreatedTopLevelElements.end());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string SaveElementsToXmlString(const LyShine::EntityArray& elements, AZ::SliceComponent* rootSlice, bool isCopyOperation, AZStd::unordered_set<AZ::Data::AssetId>& referencedSliceAssets)
    {
        InitializeReflection();

        // The easiest way to write multiple elements to a stream is to create class that contains them
        // that has an allocator. SerializedElementContainer exists for this purpose.
        // It saves/loads two lists. One is a list of top-level elements, the second is a list of all of
        // the children of those elements.
        SerializedElementContainer entitiesToSerialize;
        for (auto element : elements)
        {
            entitiesToSerialize.m_entities.push_back(element);

            // add the slice restore info for this top level element
            AZ::SliceComponent::EntityRestoreInfo sliceRestoreInfo;
            rootSlice->GetEntityRestoreInfo(element->GetId(), sliceRestoreInfo);
            entitiesToSerialize.m_entityRestoreInfos.push_back(sliceRestoreInfo);

            LyShine::EntityArray childElements;
            UiElementBus::Event(
                element->GetId(),
                &UiElementBus::Events::FindDescendantElements,
                []([[maybe_unused]] const AZ::Entity* entity)
                {
                    return true;
                },
                childElements);

            for (auto child : childElements)
            {
                entitiesToSerialize.m_childEntities.push_back(child);

                // add the slice restore info for this child element
                rootSlice->GetEntityRestoreInfo(child->GetId(), sliceRestoreInfo);
                entitiesToSerialize.m_childEntityRestoreInfos.push_back(sliceRestoreInfo);
            }
        }

        // if this is a copy operation we could be copying some elements in a slice instance without copying the root element
        // of the slice instance. This would cause issues. So we need to detect that situation and change the entity restore infos
        // to remove the slice instance association.
        if (isCopyOperation)
        {
            Internal::DetachEntitiesIfFullSliceInstanceNotBeingCopied(entitiesToSerialize);
        }

        // now record the referenced slice assets
        for (auto& sliceRestoreInfo : entitiesToSerialize.m_entityRestoreInfos)
        {
            if (sliceRestoreInfo)
            {
                referencedSliceAssets.insert(sliceRestoreInfo.m_assetId);
            }
        }

        for (auto& sliceRestoreInfo : entitiesToSerialize.m_childEntityRestoreInfos)
        {
            if (sliceRestoreInfo)
            {
                referencedSliceAssets.insert(sliceRestoreInfo.m_assetId);
            }
        }

        // save the entitiesToSerialize structure to the buffer
        AZStd::string charBuffer;
        AZ::IO::ByteContainerStream<AZStd::string> charStream(&charBuffer);
        [[maybe_unused]] bool success = AZ::Utils::SaveObjectToStream(charStream, AZ::ObjectStream::ST_XML, &entitiesToSerialize);
        AZ_Assert(success, "Failed to serialize elements to XML");

        return charBuffer;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LoadElementsFromXmlString(
        [[maybe_unused]] AZ::EntityId canvasEntityId,
        const AZStd::string& string,
        bool makeNewIDs,
        [[maybe_unused]] AZ::Entity* insertionPoint,
        [[maybe_unused]] AZ::Entity* insertBefore,
        LyShine::EntityArray& listOfCreatedTopLevelElements,
        LyShine::EntityArray& listOfAllCreatedElements,
        EntityRestoreVec& entityRestoreInfos)
    {
        InitializeReflection();

        AZ::IO::ByteContainerStream<const AZStd::string> charStream(&string);
        SerializedElementContainer* unserializedEntities =
            AZ::Utils::LoadObjectFromStream<SerializedElementContainer>(charStream);

        // If we want new IDs then generate them and fixup all references within the list of entities
        if (makeNewIDs)
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "No serialization context found");

            AZ::SliceComponent::EntityIdToEntityIdMap entityIdMap;
            AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(unserializedEntities, entityIdMap, context);
        }

        // copy unserializedEntities into the return output list of top-level entities
        for (auto newEntity : unserializedEntities->m_entities)
        {
            listOfCreatedTopLevelElements.push_back(newEntity);
        }

        // we also return a list of all of the created entities (not just top level ones)
        listOfAllCreatedElements.insert(listOfAllCreatedElements.end(),
            unserializedEntities->m_entities.begin(), unserializedEntities->m_entities.end());
        listOfAllCreatedElements.insert(listOfAllCreatedElements.end(),
            unserializedEntities->m_childEntities.begin(), unserializedEntities->m_childEntities.end());

        // return a list of the EntityRestoreInfos in the same order
        entityRestoreInfos.insert(entityRestoreInfos.end(),
            unserializedEntities->m_entityRestoreInfos.begin(), unserializedEntities->m_entityRestoreInfos.end());
        entityRestoreInfos.insert(entityRestoreInfos.end(),
            unserializedEntities->m_childEntityRestoreInfos.begin(), unserializedEntities->m_childEntityRestoreInfos.end());
    }

}   // namespace EntityHelpers
