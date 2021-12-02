/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewPythonFuncs.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimNodeType.h>
#include <CryCommon/Maestro/Types/AnimParamType.h>
#include <CryCommon/Maestro/Types/AnimValueType.h>

// Editor
#include "AnimationContext.h"

#include <AzCore/Asset/AssetSerializer.h>

namespace
{
    CTrackViewSequence* GetSequenceByEntityIdOrName(const CTrackViewSequenceManager* pSequenceManager, const char* entityIdOrName)
    {
        // the "name" string will be an AZ::EntityId in string form if this was called from
        // TrackView code. But for backward compatibility we also support a sequence name.
        bool isNameAValidU64 = false;
        QString entityIdString = entityIdOrName;
        AZ::u64 nameAsU64 = entityIdString.toULongLong(&isNameAValidU64);

        CTrackViewSequence* pSequence = nullptr;
        if (isNameAValidU64)
        {
            // "name" string was a valid u64 represented as a string. Use as an entity Id to search for sequence.
            pSequence = pSequenceManager->GetSequenceByEntityId(AZ::EntityId(nameAsU64));
        }

        if (!pSequence)
        {
            // name passed in could not find a sequence by using it as an EntityId. Use it as a
            // sequence name for backward compatibility
            pSequence = pSequenceManager->GetSequenceByName(entityIdOrName);
        }

        return pSequence;
    }

}

namespace
{
    //////////////////////////////////////////////////////////////////////////
    // Misc
    //////////////////////////////////////////////////////////////////////////
    void PyTrackViewSetRecording(bool bRecording)
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        if (pAnimationContext)
        {
            pAnimationContext->SetRecording(bRecording);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Sequences
    //////////////////////////////////////////////////////////////////////////
    void PyTrackViewNewSequence(const char* name, int sequenceType)
    {
        CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();

        CTrackViewSequence* pSequence = pSequenceManager->GetSequenceByName(name);
        if (pSequence)
        {
            throw std::runtime_error("A sequence with this name already exists");
        }

        CUndo undo("Create TrackView sequence");
        pSequenceManager->CreateSequence(name, static_cast<SequenceType>(sequenceType));
    }

    void PyTrackViewDeleteSequence(const char* name)
    {
        CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* pSequence = GetSequenceByEntityIdOrName(pSequenceManager, name);
        if (pSequence)
        {
            pSequenceManager->DeleteSequence(pSequence);
            return;
        }

        throw std::runtime_error("Could not find sequence");
    }

    void PyTrackViewSetCurrentSequence(const char* name)
    {
        const CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* sequence = GetSequenceByEntityIdOrName(sequenceManager, name);
        CAnimationContext* animationContext = GetIEditor()->GetAnimation();
        bool force = false;
        bool noNotify = false;
        bool user = true;
        animationContext->SetSequence(sequence, force, noNotify, user);
    }

    int PyTrackViewGetNumSequences()
    {
        const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        AZ_TracePrintf("", "PyTrackViewGetNumSequences called")
        return pSequenceManager->GetCount();
    }

    AZStd::string PyTrackViewGetSequenceName(unsigned int index)
    {
        if (static_cast<int>(index) < PyTrackViewGetNumSequences())
        {
            const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
            return pSequenceManager->GetSequenceByIndex(index)->GetName();
        }

        throw std::runtime_error("Could not find sequence");
    }

    Range PyTrackViewGetSequenceTimeRange(const char* name)
    {
        const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* pSequence = GetSequenceByEntityIdOrName(pSequenceManager, name);
        if (!pSequence)
        {
            throw std::runtime_error("A sequence with this name doesn't exists");
        }

        return pSequence->GetTimeRange();
    }

    void PyTrackViewSetSequenceTimeRange(const char* name, float start, float end)
    {
        const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* pSequence = GetSequenceByEntityIdOrName(pSequenceManager, name);
        if (!pSequence)
        {
            throw std::runtime_error("A sequence with this name doesn't exists");
        }

        CUndo undo("Set sequence time range");
        pSequence->SetTimeRange(Range(start, end));
        pSequence->MarkAsModified();
    }

    void PyTrackViewPlaySequence()
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        if (pAnimationContext->IsPlaying())
        {
            throw std::runtime_error("A sequence is already playing");
        }

        pAnimationContext->SetPlaying(true);
    }

    void PyTrackViewStopSequence()
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        if (!pAnimationContext->IsPlaying())
        {
            throw std::runtime_error("No sequence is playing");
        }

        pAnimationContext->SetPlaying(false);
    }

    void PyTrackViewSetSequenceTime(float time)
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        pAnimationContext->SetTime(time);
    }

    //////////////////////////////////////////////////////////////////////////
    // Nodes
    //////////////////////////////////////////////////////////////////////////
    void PyTrackViewAddNode(const char* nodeTypeString, const char* nodeName)
    {
        CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
        if (!pSequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        const AnimNodeType nodeType = GetIEditor()->GetMovieSystem()->GetNodeTypeFromString(nodeTypeString);
        if (nodeType == AnimNodeType::Invalid)
        {
            throw std::runtime_error("Invalid node type");
        }

        CUndo undo("Create anim node");
        pSequence->CreateSubNode(nodeName, nodeType);
    }

    void PyTrackViewAddSelectedEntities()
    {
        CAnimationContext* animationContext = GetIEditor()->GetAnimation();
        CTrackViewSequence* sequence = animationContext->GetSequence();
        if (!sequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        AZStd::vector<AnimParamType> tracks = {
            AnimParamType::Position,
            AnimParamType::Rotation
        };

        AzToolsFramework::ScopedUndoBatch undoBatch("Add entities to Track View");
        sequence->AddSelectedEntities(tracks);
        sequence->BindToEditorObjects();
        undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
    }

    void PyTrackViewAddLayerNode()
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
        if (!pSequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        CUndo undo("Add current layer to TrackView");
        pSequence->AddCurrentLayer();
    }

    CTrackViewAnimNode* GetNodeFromName(const char* nodeName, const char* parentDirectorName)
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
        if (!pSequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        CTrackViewAnimNode* pParentDirector = pSequence;
        if (strlen(parentDirectorName) > 0)
        {
            CTrackViewAnimNodeBundle foundNodes = pSequence->GetAnimNodesByName(parentDirectorName);
            if (foundNodes.GetCount() == 0 || foundNodes.GetNode(0)->GetType() != AnimNodeType::Director)
            {
                throw std::runtime_error("Director node not found");
            }

            pParentDirector = foundNodes.GetNode(0);
        }

        CTrackViewAnimNodeBundle foundNodes = pParentDirector->GetAnimNodesByName(nodeName);
        return (foundNodes.GetCount() > 0) ? foundNodes.GetNode(0) : nullptr;
    }

    void PyTrackViewDeleteNode(AZStd::string_view nodeName, AZStd::string_view parentDirectorName)
    {
        CTrackViewAnimNode* pNode = GetNodeFromName(nodeName.data(), parentDirectorName.data());
        if (pNode == nullptr)
        {
            throw std::runtime_error("Couldn't find node");
        }

        CTrackViewAnimNode* pParentNode = static_cast<CTrackViewAnimNode*>(pNode->GetParentNode());

        CUndo undo("Delete TrackView Node");
        pParentNode->RemoveSubNode(pNode);
    }

    void PyTrackViewAddTrack(const char* paramName, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewAnimNode* pNode = GetNodeFromName(nodeName, parentDirectorName);
        if (!pNode)
        {
            throw std::runtime_error("Couldn't find node");
        }

        // Add tracks to menu, that can be added to animation node.
        const int paramCount = pNode->GetParamCount();
        for (int i = 0; i < paramCount; ++i)
        {
            CAnimParamType paramType = pNode->GetParamType(i);

            if (paramType == AnimParamType::Invalid)
            {
                continue;
            }

            IAnimNode::ESupportedParamFlags paramFlags = pNode->GetParamFlags(paramType);

            CTrackViewTrack* pTrack = pNode->GetTrackForParameter(paramType);
            if (!pTrack || (paramFlags & IAnimNode::eSupportedParamFlags_MultipleTracks))
            {
                AZStd::string name = pNode->GetParamName(paramType);
                if (name == paramName)
                {
                    CUndo undo("Create track");
                    if (!pNode->CreateTrack(paramType))
                    {
                        undo.Cancel();
                        throw std::runtime_error("Could not create track");
                    }

                    pNode->SetSelected(true);
                    return;
                }
            }
        }

        throw std::runtime_error("Could not create track");
    }

    void PyTrackViewDeleteTrack(const char* paramName, uint32 index, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewAnimNode* pNode = GetNodeFromName(nodeName, parentDirectorName);
        if (!pNode)
        {
            throw std::runtime_error("Couldn't find node");
        }

        const CAnimParamType paramType = GetIEditor()->GetMovieSystem()->GetParamTypeFromString(paramName);
        CTrackViewTrack* pTrack = pNode->GetTrackForParameter(paramType, index);
        if (!pTrack)
        {
            throw std::runtime_error("Could not find track");
        }

        CUndo undo("Delete TrackView track");
        pNode->RemoveTrack(pTrack);
    }

    int PyTrackViewGetNumNodes(AZStd::string_view parentDirectorName)
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
        if (!pSequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        CTrackViewAnimNode* pParentDirector = pSequence;
        if (!parentDirectorName.empty())
        {
            CTrackViewAnimNodeBundle foundNodes = pSequence->GetAnimNodesByName(parentDirectorName.data());
            if (foundNodes.GetCount() == 0 || foundNodes.GetNode(0)->GetType() != AnimNodeType::Director)
            {
                throw std::runtime_error("Director node not found");
            }

            pParentDirector = foundNodes.GetNode(0);
        }

        CTrackViewAnimNodeBundle foundNodes = pParentDirector->GetAllAnimNodes();
        return foundNodes.GetCount();
    }

    AZStd::string PyTrackViewGetNodeName(int index, AZStd::string_view parentDirectorName)
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
        if (!pSequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        CTrackViewAnimNode* pParentDirector = pSequence;
        if (!parentDirectorName.empty())
        {
            CTrackViewAnimNodeBundle foundNodes = pSequence->GetAnimNodesByName(parentDirectorName.data());
            if (foundNodes.GetCount() == 0 || foundNodes.GetNode(0)->GetType() != AnimNodeType::Director)
            {
                throw std::runtime_error("Director node not found");
            }

            pParentDirector = foundNodes.GetNode(0);
        }

        CTrackViewAnimNodeBundle foundNodes = pParentDirector->GetAllAnimNodes();
        if (index < 0 || index >= static_cast<int>(foundNodes.GetCount()))
        {
            throw std::runtime_error("Invalid node index");
        }

        return foundNodes.GetNode(index)->GetName();
    }

    //////////////////////////////////////////////////////////////////////////
    // Tracks
    //////////////////////////////////////////////////////////////////////////
    CTrackViewTrack* GetTrack(const char* paramName, uint32 index, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewAnimNode* pNode = GetNodeFromName(nodeName, parentDirectorName);
        if (!pNode)
        {
            throw std::runtime_error("Couldn't find node");
        }

        const CAnimParamType paramType = GetIEditor()->GetMovieSystem()->GetParamTypeFromString(paramName);
        CTrackViewTrack* pTrack = pNode->GetTrackForParameter(paramType, index);
        if (!pTrack)
        {
            throw std::runtime_error("Track doesn't exist");
        }

        return pTrack;
    }

    std::set<float> GetKeyTimeSet(CTrackViewTrack* pTrack)
    {
        std::set<float> keyTimeSet;
        for (uint i = 0; i < pTrack->GetKeyCount(); ++i)
        {
            CTrackViewKeyHandle keyHandle = pTrack->GetKey(i);
            keyTimeSet.insert(keyHandle.GetTime());
        }

        return keyTimeSet;
    }

    int PyTrackViewGetNumTrackKeys(const char* paramName, int trackIndex, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewTrack* pTrack = GetTrack(paramName, trackIndex, nodeName, parentDirectorName);
        return (int)GetKeyTimeSet(pTrack).size();
    }

    AZStd::any PyTrackViewGetInterpolatedValue(const char* paramName, int trackIndex, float time, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewTrack* pTrack = GetTrack(paramName, trackIndex, nodeName, parentDirectorName);

        switch (pTrack->GetValueType())
        {
        case AnimValueType::Float:
        case AnimValueType::DiscreteFloat:
        {
            float value;
            pTrack->GetValue(time, value);
            return AZStd::make_any<float>(value);
        }
        break;
        case AnimValueType::Bool:
        {
            bool value;
            pTrack->GetValue(time, value);
            return AZStd::make_any<bool>(value);
        }
        break;
        case AnimValueType::Quat:
        {
            Quat value;
            pTrack->GetValue(time, value);
            Ang3 rotation(value);
            return AZStd::make_any<AZ::Vector3>(rotation.x, rotation.y, rotation.z);
        }
        case AnimValueType::Vector:
        {
            Vec3 value;
            pTrack->GetValue(time, value);
            return AZStd::make_any<AZ::Vector3>(value.x, value.y, value.z);
        }
        break;
        case AnimValueType::Vector4:
        {
            Vec4 value;
            pTrack->GetValue(time, value);
            return AZStd::make_any<AZ::Vector4>(value.x, value.y, value.z, value.w);
        }
        break;
        case AnimValueType::RGB:
        {
            Vec3 value;
            pTrack->GetValue(time, value);
            return AZStd::make_any<AZ::Color>(value.x, value.y, value.z, 0.0f);
        }
        break;
        default:
            throw std::runtime_error("Unsupported key type");
        }
    }

    AZStd::any PyTrackViewGetKeyValue(const char* paramName, int trackIndex, int keyIndex, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewTrack* pTrack = GetTrack(paramName, trackIndex, nodeName, parentDirectorName);

        std::set<float> keyTimeSet = GetKeyTimeSet(pTrack);
        if (keyIndex < 0 || keyIndex >= keyTimeSet.size())
        {
            throw std::runtime_error("Invalid key index");
        }

        auto keyTimeIter = keyTimeSet.begin();
        std::advance(keyTimeIter, keyIndex);
        const float keyTime = *keyTimeIter;

        return PyTrackViewGetInterpolatedValue(paramName, trackIndex, keyTime, nodeName, parentDirectorName);
    }
}


namespace AzToolsFramework
{
    void TrackViewComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorLayerTrackViewRequestBus>("EditorLayerTrackViewRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "track_view")
                ->Event("AddNode", &EditorLayerTrackViewRequestBus::Events::AddNode)
                ->Event("AddTrack", &EditorLayerTrackViewRequestBus::Events::AddTrack)
                ->Event("AddLayerNode", &EditorLayerTrackViewRequestBus::Events::AddLayerNode)
                ->Event("AddSelectedEntities", &EditorLayerTrackViewRequestBus::Events::AddSelectedEntities)
                ->Event("DeleteNode", &EditorLayerTrackViewRequestBus::Events::DeleteNode)
                ->Event("DeleteTrack", &EditorLayerTrackViewRequestBus::Events::DeleteTrack)
                ->Event("DeleteSequence", &EditorLayerTrackViewRequestBus::Events::DeleteSequence)
                ->Event("GetInterpolatedValue", &EditorLayerTrackViewRequestBus::Events::GetInterpolatedValue)
                ->Event("GetKeyValue", &EditorLayerTrackViewRequestBus::Events::GetKeyValue)
                ->Event("GetNodeName", &EditorLayerTrackViewRequestBus::Events::GetNodeName)
                ->Event("GetNumNodes", &EditorLayerTrackViewRequestBus::Events::GetNumNodes)
                ->Event("GetNumSequences", &EditorLayerTrackViewRequestBus::Events::GetNumSequences)
                ->Event("GetNumTrackKeys", &EditorLayerTrackViewRequestBus::Events::GetNumTrackKeys)
                ->Event("GetSequenceName", &EditorLayerTrackViewRequestBus::Events::GetSequenceName)
                ->Event("GetSequenceTimeRange", &EditorLayerTrackViewRequestBus::Events::GetSequenceTimeRange)
                ->Event("NewSequence", &EditorLayerTrackViewRequestBus::Events::NewSequence)
                ->Event("PlaySequence", &EditorLayerTrackViewRequestBus::Events::PlaySequence)
                ->Event("SetCurrentSequence", &EditorLayerTrackViewRequestBus::Events::SetCurrentSequence)
                ->Event("SetRecording", &EditorLayerTrackViewRequestBus::Events::SetRecording)
                ->Event("SetSequenceTimeRange", &EditorLayerTrackViewRequestBus::Events::SetSequenceTimeRange)
                ->Event("SetTime", &EditorLayerTrackViewRequestBus::Events::SetSequenceTime)
                ->Event("StopSequence", &EditorLayerTrackViewRequestBus::Events::StopSequence)
                ;
        }
    }

    void TrackViewComponent::Activate()
    {
        EditorLayerTrackViewRequestBus::Handler::BusConnect(GetEntityId());
    }

    void TrackViewComponent::Deactivate()
    {
        EditorLayerTrackViewRequestBus::Handler::BusDisconnect();
    }

    int TrackViewComponent::GetNumSequences()
    {
        return PyTrackViewGetNumSequences();
    }

    void TrackViewComponent::NewSequence(const char* name, int sequenceType)
    {
        return PyTrackViewNewSequence(name, sequenceType);
    }

    void TrackViewComponent::PlaySequence()
    {
        return PyTrackViewPlaySequence();
    }

    void TrackViewComponent::StopSequence()
    {
        return PyTrackViewStopSequence();
    }

    void TrackViewComponent::SetSequenceTime(float time)
    {
        return PyTrackViewSetSequenceTime(time);
    }

    void TrackViewComponent::AddSelectedEntities()
    {
        return PyTrackViewAddSelectedEntities();
    }

    void TrackViewComponent::AddLayerNode()
    {
        return PyTrackViewAddLayerNode();
    }

    void TrackViewComponent::AddTrack(const char* paramName, const char* nodeName, const char* parentDirectorName)
    {
        return PyTrackViewAddTrack(paramName, nodeName, parentDirectorName);
    }

    void TrackViewComponent::DeleteTrack(const char* paramName, uint32 index, const char* nodeName, const char* parentDirectorName)
    {
        return PyTrackViewDeleteTrack(paramName, index, nodeName, parentDirectorName);
    }

    int TrackViewComponent::GetNumTrackKeys(const char* paramName, int trackIndex, const char* nodeName, const char* parentDirectorName)
    {
        return PyTrackViewGetNumTrackKeys(paramName, trackIndex, nodeName, parentDirectorName);
    }

    void TrackViewComponent::SetRecording(bool bRecording)
    {
        return PyTrackViewSetRecording(bRecording);
    }

    void TrackViewComponent::DeleteSequence(const char* name)
    {
        return PyTrackViewDeleteSequence(name);
    }

    void TrackViewComponent::SetCurrentSequence(const char* name)
    {
        return PyTrackViewSetCurrentSequence(name);
    }

    AZStd::string TrackViewComponent::GetSequenceName(unsigned int index)
    {
        return PyTrackViewGetSequenceName(index);
    }

    Range TrackViewComponent::GetSequenceTimeRange(const char* name)
    {
        return PyTrackViewGetSequenceTimeRange(name);
    }

    void TrackViewComponent::AddNode(const char* nodeTypeString, const char* nodeName)
    {
        return PyTrackViewAddNode(nodeTypeString, nodeName);
    }

    void TrackViewComponent::DeleteNode(AZStd::string_view nodeName, AZStd::string_view parentDirectorName)
    {
        return PyTrackViewDeleteNode(nodeName, parentDirectorName);
    }

    int TrackViewComponent::GetNumNodes(AZStd::string_view parentDirectorName)
    {
        return PyTrackViewGetNumNodes(parentDirectorName);
    }

    AZStd::string TrackViewComponent::GetNodeName(int index, AZStd::string_view parentDirectorName)
    {
        return PyTrackViewGetNodeName(index, parentDirectorName);
    }

    AZStd::any TrackViewComponent::GetKeyValue(const char* paramName, int trackIndex, int keyIndex, const char* nodeName, const char* parentDirectorName)
    {
        return PyTrackViewGetKeyValue(paramName, trackIndex, keyIndex, nodeName, parentDirectorName);
    }

    AZStd::any TrackViewComponent::GetInterpolatedValue(const char* paramName, int trackIndex, float time, const char* nodeName, const char* parentDirectorName)
    {
        return PyTrackViewGetInterpolatedValue(paramName, trackIndex, time, nodeName, parentDirectorName);
    }

    void TrackViewComponent::SetSequenceTimeRange(const char* name, float start, float end)
    {
        return PyTrackViewSetSequenceTimeRange(name, start, end);
    }
}

namespace AzToolsFramework
{
    void TrackViewFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Range>("CryRange")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "legacy.trackview")
                ->Property("start", BehaviorValueProperty(&Range::start))
                ->Property("end", BehaviorValueProperty(&Range::end))
                ;

            // this will put these methods into the 'azlmbr.legacy.trackview' module
            auto addLegacyTrackview = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/TrackView")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.trackview");
            };
            addLegacyTrackview(behaviorContext->Method("set_recording", PyTrackViewSetRecording, nullptr, "Activates/deactivates TrackView recording mode."));

            addLegacyTrackview(behaviorContext->Method("new_sequence", PyTrackViewNewSequence, nullptr, "Creates a new sequence of the given type (0=Object Entity Sequence (Legacy), 1=Component Entity Sequence (PREVIEW)) with the given name."));
            addLegacyTrackview(behaviorContext->Method("delete_sequence", PyTrackViewDeleteSequence, nullptr, "Deletes the specified sequence."));
            addLegacyTrackview(behaviorContext->Method("set_current_sequence", PyTrackViewSetCurrentSequence, nullptr, "Sets the specified sequence as a current one in TrackView."));
            addLegacyTrackview(behaviorContext->Method("get_num_sequences", PyTrackViewGetNumSequences, nullptr, "Gets the number of sequences."));
            addLegacyTrackview(behaviorContext->Method("get_sequence_name", PyTrackViewGetSequenceName, nullptr, "Gets the name of a sequence by its index."));

            addLegacyTrackview(behaviorContext->Method("get_sequence_time_range", PyTrackViewGetSequenceTimeRange, nullptr, "Gets the time range of a sequence as a pair."));

            addLegacyTrackview(behaviorContext->Method("set_sequence_time_range", PyTrackViewSetSequenceTimeRange, nullptr, "Sets the time range of a sequence."));
            addLegacyTrackview(behaviorContext->Method("play_sequence", PyTrackViewPlaySequence, nullptr, "Plays the current sequence in TrackView."));
            addLegacyTrackview(behaviorContext->Method("stop_sequence", PyTrackViewStopSequence, nullptr, "Stops any sequence currently playing in TrackView."));
            addLegacyTrackview(behaviorContext->Method("set_time", PyTrackViewSetSequenceTime, nullptr, "Sets the time of the sequence currently playing in TrackView."));

            addLegacyTrackview(behaviorContext->Method("add_node", PyTrackViewAddNode, nullptr, "Adds a new node with the given type & name to the current sequence."));
            addLegacyTrackview(behaviorContext->Method("add_selected_entities", PyTrackViewAddSelectedEntities, nullptr, "Adds an entity node(s) from viewport selection to the current sequence."));
            addLegacyTrackview(behaviorContext->Method("add_layer_node", PyTrackViewAddLayerNode, nullptr, "Adds a layer node from the current layer to the current sequence."));
            addLegacyTrackview(behaviorContext->Method("delete_node", PyTrackViewDeleteNode, nullptr, "Deletes the specified node from the current sequence."));
            addLegacyTrackview(behaviorContext->Method("add_track", PyTrackViewAddTrack, nullptr, "Adds a track of the given parameter ID to the node."));
            addLegacyTrackview(behaviorContext->Method("delete_track", PyTrackViewDeleteTrack, nullptr, "Deletes a track of the given parameter ID (in the given index in case of a multi-track) from the node."));
            addLegacyTrackview(behaviorContext->Method("get_num_nodes", PyTrackViewGetNumNodes, nullptr, "Gets the number of nodes."));
            addLegacyTrackview(behaviorContext->Method("get_node_name", PyTrackViewGetNodeName, nullptr, "Gets the name of a sequence by its index."));

            addLegacyTrackview(behaviorContext->Method("get_num_track_keys", PyTrackViewGetNumTrackKeys, nullptr, "Gets number of keys of the specified track."));

            addLegacyTrackview(behaviorContext->Method("get_key_value", PyTrackViewGetKeyValue, nullptr, "Gets the value of the specified key."));
            addLegacyTrackview(behaviorContext->Method("get_interpolated_value", PyTrackViewGetInterpolatedValue, nullptr, "Gets the interpolated value of a track at the specified time."));
        }
    }
}
