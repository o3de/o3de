/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorSequenceComponent.h"

#include "EditorSequenceAgentComponent.h"
#include "TrackView/TrackViewSequenceManager.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <Cinematics/AnimSequence.h>
#include <Maestro/Bus/SequenceAgentComponentBus.h>
#include <Maestro/Types/AnimNodeType.h>
#include <Maestro/Types/AnimValueType.h>
#include <Maestro/Types/SequenceType.h>

namespace Maestro
{
    /*static*/ AZ::ScriptTimePoint EditorSequenceComponent::s_lastPropertyRefreshTime;
    /*static*/ const double        EditorSequenceComponent::s_refreshPeriodMilliseconds = 200.0;  // 5 Hz refresh rate
    /*static*/ const uint32        EditorSequenceComponent::s_invalidSequenceId = std::numeric_limits<uint32>::max();

    namespace ClassConverters
    {
        static bool UpVersionAnimationData(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode&);
    } // namespace ClassConverters

    EditorSequenceComponent::EditorSequenceComponent()
        : m_sequenceId(s_invalidSequenceId)
    {
        AZ_Trace("EditorSequenceComponent", "EditorSequenceComponent %p", this);
    }

    EditorSequenceComponent::~EditorSequenceComponent()
    {
        AZ_Trace("EditorSequenceComponent", "~EditorSequenceComponent %p", this);

        bool isDuringUndo = false;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

        // Don't RemoveEntityToAnimate if we are in the middle of an Undo event.
        // Doing so will mark this entity dirty and break the undo system.
        if (!isDuringUndo && m_sequence)
        {
            for (int i = m_sequence->GetNodeCount(); --i >= 0;)
            {
                IAnimNode* animNode = m_sequence->GetNode(i);
                if (animNode->GetType() == AnimNodeType::AzEntity)
                {
                    RemoveEntityToAnimate(animNode->GetAzEntityId());
                    // TODO (AnimSequence linking):
                    // Consider destroying the now ambiguous CAnimAzEntityNode right here,
                    // or in the RemoveEntityToAnimate(), as this method call is also available
                    // through the EditorSequenceComponentRequestBus::Events::DisconnectSequence,
                    // because the corresponding agent component is now detached and "dead"
                    // (please see comments in the EditorSequenceAgentComponent::DisconnectSequence());
                    // with something like:
                    //  constexpr const bool removeChildRelationships = false;
                    //  m_sequence->RemoveNode(animNode, removeChildRelationships);
                    // In ideal world the governing code calling RemoveEntityToAnimate() is then to call the AddEntityToAnimate()
                    // in case the Sequence <-> SequenceAgent link is further used.
                    // In real world, - currently, - such removal interferes with the current implementation
                    // of the governing code.
                }
            }
        }

        if (m_sequence)
        {
            IEditor* editor = nullptr;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::Bus::Events::GetEditor);
            if (editor)
            {
                ITrackViewSequenceManager* pSequenceManager = editor->GetSequenceManager();
                if (pSequenceManager && pSequenceManager->GetSequenceByEntityId(m_sequence->GetSequenceEntityId()))
                {
                    pSequenceManager->OnDeleteSequenceEntity(m_sequence->GetSequenceEntityId());
                }
            }
            m_sequence = nullptr;
            m_sequenceId = s_invalidSequenceId;
        }
    }

    /*static*/ void EditorSequenceComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<AnimSerialize::AnimationData>()
                ->Field("SerializedString", &AnimSerialize::AnimationData::m_serializedData)
                ->Version(1, &ClassConverters::UpVersionAnimationData);

            serializeContext->Class<EditorSequenceComponent, EditorComponentBase>()
                ->Field("Sequence", &EditorSequenceComponent::m_sequence)
                ->Version(4);

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<EditorSequenceComponent>(
                    "Sequence", "Plays Cinematic Animations")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Cinematics")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Sequence.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Sequence.png")
                      //->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, false)     // SequenceAgents are only added by TrackView
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
        
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<EditorSequenceComponent>()->RequestBus("SequenceComponentRequestBus");
        }
    }

    void EditorSequenceComponent::Init()
    {
        EditorComponentBase::Init();
        m_sequenceId = s_invalidSequenceId;
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::Bus::Events::GetEditor);

        if (editor)
        {
            bool sequenceWasDeserialized = false;
            if (m_sequence)
            {
                // m_sequence is already filled if the component it was deserialized - register it with Track View
                sequenceWasDeserialized = true;
                editor->GetSequenceManager()->OnCreateSequenceComponent(m_sequence);
            }
            else
            {
                // if m_sequence is NULL, we're creating a new sequence - request the creation from the Track view
                m_sequence = static_cast<CAnimSequence*>(editor->GetSequenceManager()->OnCreateSequenceObject(m_entity->GetName().c_str(), false, GetEntityId()));
            }

            if (m_sequence)
            {
                m_sequenceId = m_sequence->GetId();
            }

            if (sequenceWasDeserialized)
            {
                // Notify Trackview of the load
                ITrackViewSequence* trackViewSequence = editor->GetSequenceManager()->GetSequenceByEntityId(GetEntityId());
                if (trackViewSequence)
                {
                    trackViewSequence->Load();
                }
            }
        }
    }

    void EditorSequenceComponent::Activate()
    {
        EditorComponentBase::Activate();

        Maestro::EditorSequenceComponentRequestBus::Handler::BusConnect(GetEntityId());
        Maestro::SequenceComponentRequestBus::Handler::BusConnect(GetEntityId());

        AZ_Trace("EditorSequenceComponent::Activate", "SequenceComponentRequestBus connected to %s", GetEntityId().ToString().c_str());

        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::Bus::Events::GetEditor);
        if (editor)
        {
            editor->GetSequenceManager()->OnSequenceActivated(GetEntityId());
        }

        IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();

        // Add this sequence into the game movie system.
        if (m_sequence && movieSystem)
        {
            movieSystem->AddSequence(m_sequence.get());
        }
    }

    void EditorSequenceComponent::Deactivate()
    {
        Maestro::EditorSequenceComponentRequestBus::Handler::BusDisconnect();
        Maestro::SequenceComponentRequestBus::Handler::BusDisconnect();

        AZ_Trace("EditorSequenceComponent::Deactivate", "SequenceComponentRequestBus disconnected from %s", GetEntityId().ToString().c_str());

        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::Bus::Events::GetEditor);
        if (editor && m_sequence)
        {
            ITrackViewSequenceManager* sequenceManager = editor->GetSequenceManager();
            const AZ::EntityId& sequenceEntityId = m_sequence->GetSequenceEntityId();
            if (sequenceManager->GetSequenceByEntityId(sequenceEntityId))
            {
                sequenceManager->OnSequenceDeactivated(GetEntityId());
            }

            IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
            // Remove this sequence from the game movie system.
            if (movieSystem)
            {
                movieSystem->RemoveSequence(m_sequence.get());
            }
        }

        // disconnect from TickBus if we're connected (which would only happen if we deactivated during a pending property refresh)
        AZ::TickBus::Handler::BusDisconnect();

        EditorComponentBase::Deactivate();
    }

    bool EditorSequenceComponent::AddEntityToAnimate(AZ::EntityId entityToAnimate)
    {
        Maestro::EditorSequenceAgentComponent* agentComponent = nullptr;
        auto component = AzToolsFramework::FindComponent<EditorSequenceAgentComponent>::OnEntity(entityToAnimate);
        if (component)
        {
            agentComponent = static_cast<Maestro::EditorSequenceAgentComponent*>(component);
        }
        else
        {
            // #TODO LY-21846: Use "SequenceAgentComponentService" to find component, rather than specific component-type.
            auto addComponentResult = AzToolsFramework::AddComponents<EditorSequenceAgentComponent>::ToEntities(entityToAnimate);

            if (addComponentResult.IsSuccess())
            {
                if (!addComponentResult.GetValue()[entityToAnimate].m_componentsAdded.empty())
                {
                    // We need to register our Entity and Component Ids with the SequenceAgentComponent so we can communicate over EBuses
                    // with it. We can't do this registration over an EBus because we haven't registered with it yet - do it with pointers?
                    // Is this safe?
                    agentComponent = static_cast<Maestro::EditorSequenceAgentComponent*>(
                        addComponentResult.GetValue()[entityToAnimate].m_componentsAdded[0]);
                }
                else
                {
                    AZ_Assert(
                        !addComponentResult.GetValue()[entityToAnimate].m_componentsAdded.empty(),
                        "Add component result was successful, but the EditorSequenceAgentComponent wasn't added.  "
                        "This can happen if the entity id isn't found for some reason: entity id = %s",
                        entityToAnimate.ToString().c_str());
                }
            }
        }

        AZ_Assert(agentComponent, "EditorSequenceComponent::AddEntityToAnimate unable to create or find sequenceAgentComponent.");
        // Notify the SequenceAgentComponent that we're connected to it - after this call, all communication with the Agent is over an EBus
        if (agentComponent)
        {
            agentComponent->ConnectSequence(GetEntityId());
        }
        return true;
    }

    void EditorSequenceComponent::RemoveEntityToAnimate(AZ::EntityId removedEntityId)
    {
        if (!GetEntity())
        {
            // This check removes ambiguous warning in Component::GetEntityId()
            // while destroying an instance when sanitizing a prefab DOM :
            // entities are not activated, components have no pointers to parent entities,
            // buses are not connected -> no need to try fetching owner EntityId from m_sequence.
            return;
        }

        const Maestro::SequenceAgentEventBusId ebusId(GetEntityId(), removedEntityId);

        // Notify the SequenceAgentComponent that we're disconnecting from it
        Maestro::SequenceAgentComponentRequestBus::Event(ebusId, &Maestro::SequenceAgentComponentRequestBus::Events::DisconnectSequence);
    }

    void EditorSequenceComponent::GetAllAnimatablePropertiesForComponent(IAnimNode::AnimParamInfos& properties, AZ::EntityId animatedEntityId, AZ::ComponentId componentId)
    {
        const Maestro::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);

        Maestro::EditorSequenceAgentComponentRequestBus::Event(ebusId, &Maestro::EditorSequenceAgentComponentRequestBus::Events::GetAllAnimatableProperties, properties, componentId);
    }

    void EditorSequenceComponent::GetAnimatableComponents(AZStd::vector<AZ::ComponentId>& componentIds, AZ::EntityId animatedEntityId)
    {
        const Maestro::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);

        Maestro::EditorSequenceAgentComponentRequestBus::Event(
            ebusId, &Maestro::EditorSequenceAgentComponentRequestBus::Events::GetAnimatableComponents, componentIds);
    }

    AZ::Uuid EditorSequenceComponent::GetAnimatedAddressTypeId(const AZ::EntityId& animatedEntityId, const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress)
    {
        AZ::Uuid typeId = AZ::Uuid::CreateNull();
        const Maestro::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);

        Maestro::SequenceAgentComponentRequestBus::EventResult(typeId, ebusId, &Maestro::SequenceAgentComponentRequestBus::Events::GetAnimatedAddressTypeId, animatableAddress);

        return typeId;
    }

    void EditorSequenceComponent::GetAssetDuration(AnimatedValue& returnValue, const AZ::EntityId& animatedEntityId, AZ::ComponentId componentId, const AZ::Data::AssetId& assetId)
    {
        const Maestro::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);
        Maestro::SequenceAgentComponentRequestBus::Event(
            ebusId, &Maestro::SequenceAgentComponentRequestBus::Events::GetAssetDuration, returnValue, componentId, assetId);
    }

    void EditorSequenceComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        SequenceComponent *gameSequenceComponent = gameEntity->CreateComponent<SequenceComponent>();
        gameSequenceComponent->m_sequence = m_sequence;
    }

    AnimValueType EditorSequenceComponent::GetValueType([[maybe_unused]] const AZStd::string& animatableAddress)
    {
        // TODO: look up type from BehaviorContext Property
        return AnimValueType::Float;
    }

    bool EditorSequenceComponent::SetAnimatedPropertyValue(const AZ::EntityId& animatedEntityId, const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress, const AnimatedValue& value)
    {
        const Maestro::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);
        bool changed = false;
        bool animatedEntityIsSelected = false;

        // put this component on the TickBus to refresh propertyGrids if it is Selected (and hence it's values will be shown in the EntityInspector)
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(animatedEntityIsSelected, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsSelected, animatedEntityId);
        if (animatedEntityIsSelected && !AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }

        Maestro::SequenceAgentComponentRequestBus::EventResult(
            changed, ebusId, &Maestro::SequenceAgentComponentRequestBus::Events::SetAnimatedPropertyValue, animatableAddress, value);

        return changed;
    }

    void EditorSequenceComponent::OnTick([[maybe_unused]] float deltaTime, AZ::ScriptTimePoint time)
    {
        // refresh the property displays at a lower refresh rate
        if ((time.GetMilliseconds() - s_lastPropertyRefreshTime.GetMilliseconds()) > s_refreshPeriodMilliseconds)
        {
            s_lastPropertyRefreshTime = time;

            // refresh.  We have to refresh the entire property tree system since sequences can modify
            // multiple different shapes in multiple different components.
            
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);

            // disconnect from tick bus now that we've refreshed
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    bool EditorSequenceComponent::GetAnimatedPropertyValue(AnimatedValue& returnValue, const AZ::EntityId& animatedEntityId, const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress)
    {
        const Maestro::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);
        Maestro::SequenceAgentComponentRequestBus::Event(
            ebusId, &Maestro::SequenceAgentComponentRequestBus::Events::GetAnimatedPropertyValue, returnValue, animatableAddress);
        return true;
    }

    bool EditorSequenceComponent::MarkEntityAsDirty() const
    {
        return false;
    }

    namespace ClassConverters
    {
        // recursively traverses XML tree rooted at node converting transform nodes. Returns true if any node was converted.
        static bool convertTransformXMLNodes(XmlNodeRef node)
        {
            bool nodeConverted = false;

            // recurse through children
            for (int i = node->getChildCount(); --i >= 0;)
            {
                if (convertTransformXMLNodes(node->getChild(i)))
                {
                    nodeConverted = true;
                }
            }

            XmlString nodeType;
            if (node->isTag("Node") && node->getAttr("Type", nodeType) && nodeType == "Component")
            {
                XmlString componentTypeId;
                if (node->getAttr("ComponentTypeId", componentTypeId) && componentTypeId == "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0}")   // Type Uuid AZ::EditorTransformComponentTypeId
                {
                    static const char* paramTypeName = "paramType";
                    static const char* paramUserValueName = "paramUserValue";
                    static const char* virtualPropertyName = "virtualPropertyName";

                    // go through child nodes. Convert previous Position, Rotation or Scale tracks ByString to enumerated param types
                    for (const XmlNodeRef& childNode : node)
                    {
                        XmlString paramType;
                        if (childNode->isTag("Track") && childNode->getAttr(paramTypeName, paramType) && paramType == "ByString")
                        {
                            XmlString paramUserValue;
                            if (childNode->getAttr(paramUserValueName, paramUserValue) && paramUserValue == "Position")
                            {
                                childNode->setAttr(paramTypeName, "Position");
                                childNode->setAttr(virtualPropertyName, "Position");
                                childNode->delAttr(paramUserValueName);
                                nodeConverted = true;
                            }
                            else if (childNode->getAttr(paramUserValueName, paramUserValue) && paramUserValue == "Rotation")
                            {
                                childNode->setAttr(paramTypeName, "Rotation");
                                childNode->setAttr(virtualPropertyName, "Rotation");
                                childNode->delAttr(paramUserValueName);
                                nodeConverted = true;
                            }
                            else if (childNode->getAttr(paramUserValueName, paramUserValue) && paramUserValue == "Scale")
                            {
                                childNode->setAttr(paramTypeName, "Scale");
                                childNode->setAttr(virtualPropertyName, "Scale");
                                childNode->delAttr(paramUserValueName);
                                nodeConverted = true;
                            }
                        }
                    }
                }
            }

            return nodeConverted;
        }

        static bool UpVersionAnimationData(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() == 0)
            {

                // upgrade V0 to V1 - change "Position", "Rotation", "Scale" anim params in Transform Component Nodes from AnimParamType::ByString to
                // AnimParamType::Position, AnimParamType::Rotation, AnimParamType::Scale respectively
                int serializedAnimStringIdx = classElement.FindElement(AZ::Crc32("SerializedString"));
                if (serializedAnimStringIdx == -1)
                {
                    AZ_Error("Serialization", false, "Failed to find 'SerializedString' element.");
                    return false;
                }

                AZStd::string serializedAnimString;
                classElement.GetSubElement(serializedAnimStringIdx).GetData(serializedAnimString);

                const char* buffer = serializedAnimString.c_str();
                size_t size = serializedAnimString.length();
                if (size > 0)
                {
                    XmlNodeRef xmlArchive = gEnv->pSystem->LoadXmlFromBuffer(buffer, size);

                    // recursively traverse and convert through all nodes
                    if (convertTransformXMLNodes(xmlArchive))
                    {
                        // if a node was converted, replace the classElement Data with the converted XML
                        serializedAnimString = xmlArchive->getXML();
                        classElement.GetSubElement(serializedAnimStringIdx).SetData(context, serializedAnimString);
                    }
                }
            }

            return true;
        }
    } // namespace ClassConverters

} // namespace Maestro
