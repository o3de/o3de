/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SerializeHelpers.h"

#include "UiEditorEntityContextBus.h"
#include "UiEditorEntityContext.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <Shine/Bus/UiElementBus.h>
#include <Shine/Bus/UiCanvasBus.h>

#include <QMessageBox>
#include <QApplication>

namespace SerializeHelpers
{
    bool s_initializedReflection = false;

    //! Simple helper class for serializing a vector of entities and their child entities.
    //! This is only serialized for the undo system
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
    };


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
                ->Version(2)
                ->Field("Entities", &SerializedElementContainer::m_entities)
                ->Field("ChildEntities", &SerializedElementContainer::m_childEntities);

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
        Shine::EntityArray* cumulativeListOfCreatedEntities)
    {
        Shine::EntityArray listOfNewlyCreatedTopLevelElements;
        Shine::EntityArray listOfAllCreatedEntities;

        LoadElementsFromXmlString(
            canvasEntityId,
            xml.c_str(),
            isCopyOperation,
            parent,
            insertBefore,
            listOfNewlyCreatedTopLevelElements,
            listOfAllCreatedEntities);

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

        // Add all created entities to the entity context
        for (auto entity : listOfAllCreatedEntities)
        {
            entityContext->AddUiEntity(entity);
        }

        // Fixup the created entities, we do this before adding the top level element to the parent so that
        // MakeUniqueChileName works correctly
        UiCanvasBus::Event(
            canvasEntityId,
            &UiCanvasBus::Events::FixupCreatedEntities,
            listOfNewlyCreatedTopLevelElements,
            isCopyOperation,
            parent);

        // Now add the top-level created elements as children of the parent
        for (auto entity : listOfNewlyCreatedTopLevelElements)
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
                        listOfNewlyCreatedTopLevelElements.begin(),
                        listOfNewlyCreatedTopLevelElements.end());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string SaveElementsToXmlString(const Shine::EntityArray& elements, [[maybe_unused]] bool isCopyOperation)
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

            Shine::EntityArray childElements;
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
        Shine::EntityArray& listOfCreatedTopLevelElements,
        Shine::EntityArray& listOfAllCreatedElements)
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

            AZStd::unordered_map<AZ::EntityId, AZ::EntityId> entityIdMap;
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
    }

}   // namespace EntityHelpers
