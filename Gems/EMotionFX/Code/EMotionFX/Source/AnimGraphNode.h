/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/tuple.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include <EMotionFX/Source/TriggerActionSetup.h>
#include "EMotionFXConfig.h"
#include <MCore/Source/StringIdPool.h>
#include "AnimGraphAttributeTypes.h"
#include "BlendTreeConnection.h"
#include "AnimGraphObject.h"
#include "AnimGraphInstance.h"
#include "AnimGraphNodeData.h"

#include <AzCore/std/functional.h>
#include <AzCore/Math/Color.h>

namespace EMotionFX
{
    // forward declarations
    class AnimGraph;
    class Pose;
    class MotionInstance;
    class ActorInstance;
    class AnimGraphStateMachine;
    class BlendTree;
    class AnimGraphStateTransition;
    class AnimGraphTriggerAction;
    class AnimGraphRefCountedData;
    class AnimGraph;
    class AnimGraphTransitionCondition;

    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphNode
        : public AnimGraphObject
    {
    public:
        AZ_RTTI(AnimGraphNode, "{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}", AnimGraphObject)
        AZ_CLASS_ALLOCATOR_DECL

        struct EMFX_API Port
        {
            AZ_RTTI(AnimGraphNode::Port, "{F66CF090-468F-419A-9518-97988304FEB6}")
            AZ_CLASS_ALLOCATOR_DECL

            BlendTreeConnection*    m_connection;            // the connection plugged in this port
            uint32                  m_compatibleTypes[4];    // four possible compatible types
            uint32                  m_portId;                // the unique port ID (unique inside the node input or output port lists)
            uint32                  m_nameId;                // the name of the port (using the StringIdPool)
            size_t                  m_attributeIndex;        // the index into the animgraph instance global attributes array

            MCORE_INLINE const char* GetName() const                    { return MCore::GetStringIdPool().GetName(m_nameId).c_str(); }
            MCORE_INLINE const AZStd::string& GetNameString() const     { return MCore::GetStringIdPool().GetName(m_nameId); }

            // copy settings from another port (always makes the m_connection and m_value nullptr though)
            void InitFrom(const Port& other)
            {
                for (uint32 i = 0; i < 4; ++i)
                {
                    m_compatibleTypes[i] = other.m_compatibleTypes[i];
                }

                m_portId         = other.m_portId;
                m_nameId         = other.m_nameId;
                m_connection     = nullptr;
                m_attributeIndex = other.m_attributeIndex;
            }

            void SetCompatibleTypes(const AZStd::vector<uint32>& compatibleTypes)
            {
                for (uint32 i = 0; i < 4 && i < compatibleTypes.size(); ++i)
                {
                    m_compatibleTypes[i] = compatibleTypes[i];
                }
            }

            // get the attribute value
            MCORE_INLINE MCore::Attribute* GetAttribute(AnimGraphInstance* animGraphInstance) const
            {
                return animGraphInstance->GetInternalAttribute(m_attributeIndex);
            }

            // port connection compatibility check
            bool CheckIfIsCompatibleWith(const Port& otherPort) const
            {
                // check the data types
                for (uint32 compatibleType : m_compatibleTypes)
                {
                    // If there aren't any more compatibility types and we haven't found a compatible one so far, return false
                    if (compatibleType == 0)
                    {
                        return false;
                    }
                    for (uint32 otherCompatibleTypeIndex : otherPort.m_compatibleTypes)
                    {
                        if (otherCompatibleTypeIndex == compatibleType)
                        {
                            return true;
                        }

                        // If there aren't any more compatibility types and we haven't found a compatible one so far, return false
                        if (otherCompatibleTypeIndex == 0)
                        {
                            break;
                        }
                    }
                }

                // not compatible
                return false;
            }

            // clear compatibility types
            void ClearCompatibleTypes()
            {
                m_compatibleTypes[0] = 0;
                m_compatibleTypes[1] = 0;
                m_compatibleTypes[2] = 0;
                m_compatibleTypes[3] = 0;
            }

            void Clear()
            {
                ClearCompatibleTypes();
            }

            Port()
                : m_connection(nullptr)
                , m_portId(MCORE_INVALIDINDEX32)
                , m_nameId(MCORE_INVALIDINDEX32)
                , m_attributeIndex(InvalidIndex)  { ClearCompatibleTypes(); }
            virtual ~Port() { }
        };

        AnimGraphNode();
        AnimGraphNode(AnimGraph* animGraph, const char* name);
        virtual ~AnimGraphNode();

        virtual void RecursiveReinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;
        void InitTriggerActions();

        virtual bool GetSupportsVisualization() const { return false; }
        virtual bool GetSupportsPreviewMotion() const { return false; }
        virtual bool GetSupportsDisable() const { return false; }
        virtual bool GetHasVisualOutputPorts() const { return true; }
        virtual bool GetCanHaveOnlyOneInsideParent() const { return false; }
        virtual bool GetIsDeletable() const { return true; }
        virtual bool GetIsLastInstanceDeletable() const { return true; }
        virtual bool GetCanActAsState() const { return false; }
        virtual bool GetHasVisualGraph() const { return false; }
        virtual bool GetCanHaveChildren() const { return false; }
        virtual bool GetHasOutputPose() const { return false; }
        virtual bool GetCanBeInsideStateMachineOnly() const { return false; }
        virtual bool GetCanBeInsideChildStateMachineOnly() const{ return false; }
        virtual bool GetNeedsNetTimeSync() const                { return false; }
        virtual bool GetCanBeEntryNode() const                  { return true; }
        virtual AZ::Color GetVisualColor() const                { return AZ::Color(0.28f, 0.24f, 0.93f, 1.0f); }
        virtual AZ::Color GetHasChildIndicatorColor() const     { return AZ::Color(1.0f, 1.0f, 0, 1.0f); }

        void InitInternalAttributes(AnimGraphInstance* animGraphInstance) override;
        void RemoveInternalAttributesForAllInstances() override;
        void DecreaseInternalAttributeIndices(size_t decreaseEverythingHigherThan) override;

        void OutputAllIncomingNodes(AnimGraphInstance* animGraphInstance);
        void UpdateAllIncomingNodes(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);
        void UpdateIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* node, float timePassedInSeconds);

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew AnimGraphNodeData(this, animGraphInstance); }

        virtual void RecursiveResetUniqueDatas(AnimGraphInstance* animGraphInstance);

        void InvalidateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void RecursiveInvalidateUniqueDatas(AnimGraphInstance* animGraphInstance) override;

        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;

        void PerformOutput(AnimGraphInstance* animGraphInstance);
        void PerformTopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);
        void PerformUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);
        void PerformPostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);

        /**
         * Inform a node that PostUpdate will not be called for the current evaluation cycle
         *
         * Some node types allocate data in Update and expect to release that
         * data in PostUpdate. However, PostUpdate is not always called (like
         * when transitioning out of a node). This method allows the node to
         * perform the necessary cleanup.
         */
        virtual void SkipPostUpdate([[maybe_unused]] AnimGraphInstance* animGraphInstance) {}

        /**
         * Inform a node that Output will not be called for the current evaluation cycle
         *
         * Some node types allocate data in Update and expect to release that
         * data in Output. However, Output is not always called (like when a
         * character is not visible). This method allows the node to perform
         * the necessary cleanup.
         */
        virtual void SkipOutput([[maybe_unused]] AnimGraphInstance* animGraphInstance) {}

        float GetDuration(AnimGraphInstance* animGraphInstance) const                 { return FindOrCreateUniqueNodeData(animGraphInstance)->GetDuration(); }
        virtual void SetCurrentPlayTime(AnimGraphInstance* animGraphInstance, float timeInSeconds) { FindOrCreateUniqueNodeData(animGraphInstance)->SetCurrentPlayTime(timeInSeconds); }
        virtual float GetCurrentPlayTime(AnimGraphInstance* animGraphInstance) const               { return FindOrCreateUniqueNodeData(animGraphInstance)->GetCurrentPlayTime(); }

        size_t GetSyncIndex(AnimGraphInstance* animGraphInstance) const               { return FindOrCreateUniqueNodeData(animGraphInstance)->GetSyncIndex(); }
        void SetSyncIndex(AnimGraphInstance* animGraphInstance, size_t syncIndex)     { FindOrCreateUniqueNodeData(animGraphInstance)->SetSyncIndex(syncIndex); }

        virtual void SetPlaySpeed(AnimGraphInstance* animGraphInstance, float speedFactor)         { FindOrCreateUniqueNodeData(animGraphInstance)->SetPlaySpeed(speedFactor); }
        virtual float GetPlaySpeed(AnimGraphInstance* animGraphInstance) const                     { return FindOrCreateUniqueNodeData(animGraphInstance)->GetPlaySpeed(); }
        virtual void SetCurrentPlayTimeNormalized(AnimGraphInstance* animGraphInstance, float normalizedTime);
        virtual void Rewind(AnimGraphInstance* animGraphInstance);

        void AutoSync(AnimGraphInstance* animGraphInstance, AnimGraphNode* leaderNode, float weight, ESyncMode syncMode, bool resync);
        void SyncFullNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* leaderNode, float weight, bool modifyLeaderSpeed = true);
        void SyncPlayTime(AnimGraphInstance* animGraphInstance, AnimGraphNode* leaderNode);
        void SyncUsingSyncTracks(AnimGraphInstance* animGraphInstance, AnimGraphNode* syncWithNode, const AnimGraphSyncTrack* syncTrackA, const AnimGraphSyncTrack* syncTrackB, float weight, bool resync, bool modifyLeaderSpeed = true);
        static AZStd::tuple<float, float, float> SyncPlaySpeeds(float playSpeedA, float durationA, float playSpeedB, float durationB, float weight);
        void SyncPlaySpeeds(AnimGraphInstance* animGraphInstance, AnimGraphNode* leaderNode, float weight, bool modifyLeaderSpeed = true);
        virtual void HierarchicalSyncInputNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* inputNode, AnimGraphNodeData* uniqueDataOfThisNode);
        void HierarchicalSyncAllInputNodes(AnimGraphInstance* animGraphInstance, AnimGraphNodeData* uniqueDataOfThisNode);

        static void CalcSyncFactors(AnimGraphInstance* animGraphInstance, const AnimGraphNode* leaderNode, const AnimGraphNode* followerNode, ESyncMode syncMode, float weight, float* outLeaderFactor, float* outFollowerFactor, float* outPlaySpeed);
        static void CalcSyncFactors(float leaderPlaySpeed, const AnimGraphSyncTrack* leaderSyncTrack, size_t leaderSyncTrackIndex, float leaderDuration,
            float followerPlaySpeed, const AnimGraphSyncTrack* followerSyncTrack, size_t followerSyncTrackIndex, float followerDuration,
            ESyncMode syncMode, float weight, float* outLeaderFactor, float* outFollowerFactor, float* outPlaySpeed);

        void RequestPoses(AnimGraphInstance* animGraphInstance);
        void FreeIncomingPoses(AnimGraphInstance* animGraphInstance);
        void IncreaseInputRefCounts(AnimGraphInstance* animGraphInstance);
        void DecreaseRef(AnimGraphInstance* animGraphInstance);

        void RequestRefDatas(AnimGraphInstance* animGraphInstance);
        void FreeIncomingRefDatas(AnimGraphInstance* animGraphInstance);
        void IncreaseInputRefDataRefCounts(AnimGraphInstance* animGraphInstance);
        void DecreaseRefDataRef(AnimGraphInstance* animGraphInstance);

        /**
         * Get a pointer to the custom data you stored.
         * Custom data can for example link a game or engine object. The pointer that you specify will not be deleted when the object is being destructed.
         * @result A void pointer to the custom data you have specified.
         */
        void* GetCustomData() const;

        /**
         * Set a pointer to the custom data you stored.
         * Custom data can for example link a game or engine object. The pointer that you specify will not be deleted when the object is being destructed.
         * @param dataPointer A void pointer to the custom data, which could for example be your engine or game object.
         */
        void SetCustomData(void* dataPointer);

        virtual AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const  { MCORE_UNUSED(animGraphInstance); MCORE_ASSERT(false); return nullptr; }

        virtual void RecursiveCollectActiveNodes(AnimGraphInstance* animGraphInstance, AZStd::vector<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType = AZ::TypeId::CreateNull()) const;
        virtual void RecursiveCollectActiveNetTimeSyncNodes(AnimGraphInstance* animGraphInstance, AZStd::vector<AnimGraphNode*>* outNodes) const;

        virtual bool RecursiveDetectCycles(AZStd::unordered_set<const AnimGraphNode*>& nodes) const;

        void CollectChildNodesOfType(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>* outNodes) const; // note: outNodes is NOT cleared internally, nodes are added to the array

        /**
         * Collect child nodes of the given type. This will only iterate through the child nodes and isn't a recursive process.
         * @param[in] nodeType The rtti type id of the node to check for.
         * @return [out] outNodes Pointers to all child nodes of the given type. The array will not be cleared upfront.
         */
        void CollectChildNodesOfType(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>& outNodes) const;

        void RecursiveCollectNodesOfType(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>* outNodes) const; // note: outNodes is NOT cleared internally, nodes are added to the array
        void RecursiveCollectTransitionConditionsOfType(const AZ::TypeId& conditionType, AZStd::vector<AnimGraphTransitionCondition*>* outConditions) const; // note: outNodes is NOT cleared internally, nodes are added to the array

        virtual void RecursiveCollectObjectsOfType(const AZ::TypeId& objectType, AZStd::vector<AnimGraphObject*>& outObjects) const;

        virtual void RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<AnimGraphObject*>& outObjects) const;

        virtual void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* previousState, AnimGraphStateTransition* usedTransition)
        {
            MCORE_UNUSED(animGraphInstance);
            MCORE_UNUSED(previousState);
            MCORE_UNUSED(usedTransition);
            //MCore::LogInfo("OnStateEntering '%s', with transition 0x%x, prev state='%s'", GetName(), usedTransition, (previousState!=nullptr) ? previousState->GetName() : "");
        }

        virtual void OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* previousState, AnimGraphStateTransition* usedTransition);

        virtual void OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* targetState, AnimGraphStateTransition* usedTransition)
        {
            MCORE_UNUSED(animGraphInstance);
            MCORE_UNUSED(targetState);
            MCORE_UNUSED(usedTransition);
            //MCore::LogInfo("OnStateExit '%s', with transition 0x%x, target state='%s'", GetName(), usedTransition, (targetState!=nullptr) ? targetState->GetName() : "");
        }

        virtual void OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* newState, AnimGraphStateTransition* usedTransition);

        void RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet) override;

        const char* GetName() const;
        const AZStd::string& GetNameString() const;
        void SetName(const char* name);

        MCORE_INLINE AnimGraphNodeId GetId() const                          { return m_id; }
        void SetId(AnimGraphNodeId id) { m_id = id; }

        const MCore::Attribute* GetInputValue(AnimGraphInstance* instance, size_t inputPort) const;

        size_t FindInputPortByID(uint32 portID) const;
        size_t FindOutputPortByID(uint32 portID) const;

        Port* FindInputPortByName(const AZStd::string& portName);
        Port* FindOutputPortByName(const AZStd::string& portName);

        // this is about incoming connections
        bool ValidateConnections() const;
        BlendTreeConnection* AddConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort);

        // The AddConnection method requires the sourceNode to be initialized (to have its output ports ready). This method
        // will add the connection in this node in an unitiliazed way. When this node is initialized, it will initialize
        // the connections as well.
        BlendTreeConnection* AddUnitializedConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort);

        void RemoveConnection(BlendTreeConnection* connection, bool delFromMem = true);
        void RemoveConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort);
        bool RemoveConnectionById(AnimGraphConnectionId connectionId, bool delFromMem = true);
        void RemoveAllConnections();

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        TriggerActionSetup& GetTriggerActionSetup() { return m_actionSetup; }
        const TriggerActionSetup& GetTriggerActionSetup() const { return m_actionSetup; }

        /**
         * Collect all outgoing connections.
         * As the nodes only store the incoming connections getting access to the outgoing connections is a bit harder. For that we need to process all nodes in the graph where
         * our node is located, iterate over all connections and check if they are coming from our node. Don't call this function at runtime.
         * @param[out] outConnections This will hold all output connections of our node. The second attribute of the pair is the target
         *             node of the outgoing connection. The BlendTreeConnection itself contains the pointer to the source node. The
         *             vector will be cleared upfront.
         */
        void CollectOutgoingConnections(AZStd::vector<AZStd::pair<BlendTreeConnection*, AnimGraphNode*>>& outConnections) const;

        /**
         * Collect outgoing connections that are connected to port `portIndex`.
         * As the nodes only store the incoming connections getting access to the outgoing connections is a bit harder. For that we need to process all nodes in the graph where
         * our node is located, iterate over all connections and check if they are coming from our node. Don't call this function at runtime.
         * @param[out] outConnections This will hold all output connections of our node. The second attribute of the pair is the target
         *             node of the outgoing connection. The BlendTreeConnection itself contains the pointer to the source node. The
         *             vector will be cleared upfront.
         */
        void CollectOutgoingConnections(AZStd::vector<AZStd::pair<BlendTreeConnection*, AnimGraphNode*>>& outConnections, const size_t portIndex) const;

        MCORE_INLINE bool GetInputNumberAsBool(AnimGraphInstance* animGraphInstance, size_t inputPortNr) const
        {
            const MCore::Attribute* attribute = GetInputAttribute(animGraphInstance, inputPortNr);
            if (attribute == nullptr)
            {
                return false;
            }
            switch (attribute->GetType())
            {
            case MCore::AttributeFloat::TYPE_ID:
                return !MCore::Math::IsFloatZero(static_cast<const MCore::AttributeFloat*>(attribute)->GetValue());
            case MCore::AttributeBool::TYPE_ID:
                return static_cast<const MCore::AttributeBool*>(attribute)->GetValue();
            case MCore::AttributeInt32::TYPE_ID:
                return static_cast<const MCore::AttributeInt32*>(attribute)->GetValue() != 0;
            default:
                AZ_Assert(false, "Unhandled type");
            }
            return false;
        }

        MCORE_INLINE float GetInputNumberAsFloat(AnimGraphInstance* animGraphInstance, size_t inputPortNr) const
        {
            const MCore::Attribute* attribute = GetInputAttribute(animGraphInstance, inputPortNr);
            if (attribute == nullptr)
            {
                return 0.0f;
            }
            switch (attribute->GetType())
            {
            case MCore::AttributeFloat::TYPE_ID:
                return static_cast<const MCore::AttributeFloat*>(attribute)->GetValue();
            case MCore::AttributeBool::TYPE_ID:
                return static_cast<const MCore::AttributeBool*>(attribute)->GetValue();
            case MCore::AttributeInt32::TYPE_ID:
                return static_cast<float>(static_cast<const MCore::AttributeInt32*>(attribute)->GetValue());
            default:
                AZ_Assert(false, "Unhandled type");
            }
            return 0.0f;
        }

        MCORE_INLINE int32 GetInputNumberAsInt32(AnimGraphInstance* animGraphInstance, size_t inputPortNr) const
        {
            const MCore::Attribute* attribute = GetInputAttribute(animGraphInstance, inputPortNr);
            if (attribute == nullptr)
            {
                return 0;
            }
            switch (attribute->GetType())
            {
            case MCore::AttributeFloat::TYPE_ID:
                return static_cast<int32>(static_cast<const MCore::AttributeFloat*>(attribute)->GetValue());
            case MCore::AttributeBool::TYPE_ID:
                return static_cast<const MCore::AttributeBool*>(attribute)->GetValue();
            case MCore::AttributeInt32::TYPE_ID:
                return static_cast<const MCore::AttributeInt32*>(attribute)->GetValue();
            default:
                AZ_Assert(false, "Unhandled type");
            }
            return 0;
        }

        MCORE_INLINE uint32 GetInputNumberAsUint32(AnimGraphInstance* animGraphInstance, size_t inputPortNr) const
        {
            const MCore::Attribute* attribute = GetInputAttribute(animGraphInstance, inputPortNr);
            if (attribute == nullptr)
            {
                return 0;
            }
            switch (attribute->GetType())
            {
            case MCore::AttributeFloat::TYPE_ID:
                return static_cast<uint32>(static_cast<const MCore::AttributeFloat*>(attribute)->GetValue());
            case MCore::AttributeBool::TYPE_ID:
                return static_cast<const MCore::AttributeBool*>(attribute)->GetValue();
            case MCore::AttributeInt32::TYPE_ID:
                return static_cast<const MCore::AttributeInt32*>(attribute)->GetValue();
            default:
                AZ_Assert(false, "Unhandled type");
            }
            return 0;
        }

        MCORE_INLINE AnimGraphNode* GetInputNode(size_t portNr)
        {
            const BlendTreeConnection* con = m_inputPorts[portNr].m_connection;
            if (con == nullptr)
            {
                return nullptr;
            }
            return con->GetSourceNode();
        }

        MCORE_INLINE MCore::Attribute* GetInputAttribute(AnimGraphInstance* animGraphInstance, size_t portNr) const
        {
            const BlendTreeConnection* con = m_inputPorts[portNr].m_connection;
            if (con == nullptr)
            {
                return nullptr;
            }
            return con->GetSourceNode()->GetOutputValue(animGraphInstance, con->GetSourcePort());
        }

        MCORE_INLINE MCore::AttributeFloat* GetInputFloat(AnimGraphInstance* animGraphInstance, size_t portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeFloat::TYPE_ID);
            return static_cast<MCore::AttributeFloat*>(attrib);
        }

        MCORE_INLINE MCore::AttributeInt32* GetInputInt32(AnimGraphInstance* animGraphInstance, size_t portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeInt32::TYPE_ID);
            return static_cast<MCore::AttributeInt32*>(attrib);
        }

        MCORE_INLINE MCore::AttributeString* GetInputString(AnimGraphInstance* animGraphInstance, size_t portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeString::TYPE_ID);
            return static_cast<MCore::AttributeString*>(attrib);
        }

        MCORE_INLINE MCore::AttributeBool* GetInputBool(AnimGraphInstance* animGraphInstance, size_t portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeBool::TYPE_ID);
            return static_cast<MCore::AttributeBool*>(attrib);
        }

        MCORE_INLINE bool TryGetInputVector4(AnimGraphInstance* animGraphInstance, size_t portNr, AZ::Vector4& outResult) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return false;
            }
            switch (attrib->GetType())
            {
            case MCore::AttributeVector4::TYPE_ID:
            {
                outResult = (static_cast<MCore::AttributeVector4*>(attrib))->GetValue();
                return true;
            }
            case MCore::AttributeVector3::TYPE_ID:
            {
                const AZ::Vector3& vec3 = (static_cast<MCore::AttributeVector3*>(attrib))->GetValue();
                outResult.SetX(vec3.GetX());
                outResult.SetY(vec3.GetY());
                outResult.SetZ(vec3.GetZ());
                outResult.SetW(0.0f);
                return true;
            }
            default:
                AZ_Assert(false, "Unexpected attribute type");
                break;
            }
            return false;
        }

        MCORE_INLINE bool TryGetInputVector2(AnimGraphInstance* animGraphInstance, size_t portNr, AZ::Vector2& outResult) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return false;
            }
            switch (attrib->GetType())
            {
            case MCore::AttributeVector2::TYPE_ID:
            {
                outResult = (static_cast<MCore::AttributeVector2*>(attrib))->GetValue();
                return true;
            }
            case MCore::AttributeVector3::TYPE_ID:
            {
                const AZ::Vector3& vec3 = (static_cast<MCore::AttributeVector3*>(attrib))->GetValue();
                outResult.SetX(vec3.GetX());
                outResult.SetY(vec3.GetY());
                return true;
            }
            default:
                AZ_Assert(false, "Unexpected attribute type");
                break;
            }
            return false;
        }

        MCORE_INLINE bool TryGetInputVector3(AnimGraphInstance* animGraphInstance, size_t portNr, AZ::Vector3& outResult) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return false;
            }
            switch (attrib->GetType())
            {
            case MCore::AttributeVector3::TYPE_ID:
            {
                const AZ::Vector3& vec3AttrValue = (static_cast<MCore::AttributeVector3*>(attrib))->GetValue();
                outResult = vec3AttrValue;
                return true;
            }
            case MCore::AttributeVector2::TYPE_ID:
            {
                const AZ::Vector2& vec2Value = (static_cast<MCore::AttributeVector2*>(attrib))->GetValue();
                outResult.SetX(vec2Value.GetX());
                outResult.SetY(vec2Value.GetY());
                outResult.SetZ(0.0f);
                return true;
            }
            case MCore::AttributeVector4::TYPE_ID:
            {
                const AZ::Vector4& vec4AttrValue = (static_cast<MCore::AttributeVector4*>(attrib))->GetValue();
                outResult.SetX(vec4AttrValue.GetX());
                outResult.SetY(vec4AttrValue.GetY());
                outResult.SetZ(vec4AttrValue.GetZ());
                return true;
            }
            default:
                AZ_Assert(false, "Unexpected attribute type");
                break;
            }
            return false;
        }

        MCORE_INLINE MCore::AttributeQuaternion* GetInputQuaternion(AnimGraphInstance* animGraphInstance, size_t portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeQuaternion::TYPE_ID);
            return static_cast<MCore::AttributeQuaternion*>(attrib);
        }

        MCORE_INLINE MCore::AttributeColor* GetInputColor(AnimGraphInstance* animGraphInstance, size_t portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeColor::TYPE_ID);
            return static_cast<MCore::AttributeColor*>(attrib);
        }
        MCORE_INLINE AttributeMotionInstance* GetInputMotionInstance(AnimGraphInstance* animGraphInstance, size_t portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == AttributeMotionInstance::TYPE_ID);
            return static_cast<AttributeMotionInstance*>(attrib);
        }
        MCORE_INLINE AttributePose* GetInputPose(AnimGraphInstance* animGraphInstance, size_t portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == AttributePose::TYPE_ID);
            return static_cast<AttributePose*>(attrib);
        }

        MCORE_INLINE MCore::Attribute*              GetOutputAttribute(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const                     { return m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance); }
        MCORE_INLINE MCore::AttributeFloat*         GetOutputNumber(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == MCore::AttributeFloat::TYPE_ID);
            return static_cast<MCore::AttributeFloat*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeFloat*         GetOutputFloat(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == MCore::AttributeFloat::TYPE_ID);
            return static_cast<MCore::AttributeFloat*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeInt32*         GetOutputInt32(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == MCore::AttributeInt32::TYPE_ID);
            return static_cast<MCore::AttributeInt32*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeString*        GetOutputString(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == MCore::AttributeString::TYPE_ID);
            return static_cast<MCore::AttributeString*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeBool*          GetOutputBool(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == MCore::AttributeBool::TYPE_ID);
            return static_cast<MCore::AttributeBool*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeVector2*       GetOutputVector2(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == MCore::AttributeVector2::TYPE_ID);
            return static_cast<MCore::AttributeVector2*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeVector3*       GetOutputVector3(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == MCore::AttributeVector3::TYPE_ID);
            return static_cast<MCore::AttributeVector3*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeVector4*       GetOutputVector4(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == MCore::AttributeVector4::TYPE_ID);
            return static_cast<MCore::AttributeVector4*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeQuaternion*    GetOutputQuaternion(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == MCore::AttributeQuaternion::TYPE_ID);
            return static_cast<MCore::AttributeQuaternion*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeColor*         GetOutputColor(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == MCore::AttributeColor::TYPE_ID);
            return static_cast<MCore::AttributeColor*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE AttributePose*                 GetOutputPose(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == AttributePose::TYPE_ID);
            return static_cast<AttributePose*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE AttributeMotionInstance*       GetOutputMotionInstance(AnimGraphInstance* animGraphInstance, size_t outputPortIndex) const
        {
            if (m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(m_outputPorts[outputPortIndex].m_compatibleTypes[0] == AttributeMotionInstance::TYPE_ID);
            return static_cast<AttributeMotionInstance*>(m_outputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }

        void SetupInputPortAsNumber(const char* name, size_t inputPortNr, uint32 portID);
        void SetupInputPortAsBool(const char* name, size_t inputPortNr, uint32 portID);
        void SetupInputPort(const char* name, size_t inputPortNr, uint32 attributeTypeID, uint32 portID);

        void SetupInputPortAsVector3(const char* name, size_t inputPortNr, uint32 portID);
        void SetupInputPortAsVector2(const char* name, size_t inputPortNr, uint32 portID);
        void SetupInputPortAsVector4(const char* name, size_t inputPortNr, uint32 portID);
        void SetupInputPort(const char* name, size_t inputPortNr, const AZStd::vector<uint32>& attributeTypeIDs, uint32 portID);

        void SetupOutputPort(const char* name, size_t portIndex, uint32 attributeTypeID, uint32 portID);
        void SetupOutputPortAsPose(const char* name, size_t outputPortNr, uint32 portID);
        void SetupOutputPortAsMotionInstance(const char* name, size_t outputPortNr, uint32 portID);

        bool GetHasConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort) const;
        BlendTreeConnection* FindConnection(const AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort) const;
        bool HasConnectionAtInputPort(AZ::u32 inputPortNr) const;

        /**
         * Find the connection at the given port.
         * Search over the incoming connections that are stored within this node and check if they are connected at the given port.
         * @param[in] port The port inside this node of connection to search for.
         * @result A pointer to the connection at the given port, nullptr in case there is nothing connected to that port.
         */
        BlendTreeConnection* FindConnection(uint16 port) const;
        BlendTreeConnection* FindConnectionById(AnimGraphConnectionId connectionId) const;

        /**
         * Check if a connection is connected to the given input port.
         * @param[in] inputPort The input port id to check.
         * @result True in case there is a connection plugged into the given input port, false if not.
         */
        bool CheckIfIsInputPortConnected(uint16 inputPort) const;

        AnimGraphNode* RecursiveFindNodeByName(const char* nodeName) const;
        bool RecursiveIsNodeNameUnique(const AZStd::string& newNameCandidate, const AnimGraphNode* forNode) const;
        AnimGraphNode* RecursiveFindNodeById(AnimGraphNodeId nodeId) const;

        virtual void RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToReset = 0xffffffff);

        const AZStd::vector<AnimGraphNode::Port>& GetInputPorts() const { return m_inputPorts; }
        const AZStd::vector<AnimGraphNode::Port>& GetOutputPorts() const { return m_outputPorts; }
        void SetInputPorts(const AZStd::vector<AnimGraphNode::Port>& inputPorts) { m_inputPorts = inputPorts; }
        void SetOutputPorts(const AZStd::vector<AnimGraphNode::Port>& outputPorts) { m_outputPorts = outputPorts; }
        void InitInputPorts(size_t numPorts);
        void InitOutputPorts(size_t numPorts);
        void SetInputPortName(size_t portIndex, const char* name);
        void SetOutputPortName(size_t portIndex, const char* name);
        size_t FindOutputPortIndex(const AZStd::string& name) const;
        size_t FindInputPortIndex(const AZStd::string& name) const;
        size_t AddOutputPort();
        size_t AddInputPort();
        virtual bool GetIsStateTransitionNode() const                               { return false; }
        MCORE_INLINE MCore::Attribute* GetOutputValue(AnimGraphInstance* animGraphInstance, size_t portIndex) const            { return animGraphInstance->GetInternalAttribute(m_outputPorts[portIndex].m_attributeIndex); }
        MCORE_INLINE Port& GetInputPort(size_t index)                               { return m_inputPorts[index]; }
        MCORE_INLINE Port& GetOutputPort(size_t index)                              { return m_outputPorts[index]; }
        MCORE_INLINE const Port& GetInputPort(size_t index) const                   { return m_inputPorts[index]; }
        MCORE_INLINE const Port& GetOutputPort(size_t index) const                  { return m_outputPorts[index]; }
        void RelinkPortConnections();

        MCORE_INLINE size_t GetNumConnections() const                               { return m_connections.size(); }
        MCORE_INLINE BlendTreeConnection* GetConnection(size_t index) const         { return m_connections[index]; }
        const AZStd::vector<BlendTreeConnection*>& GetConnections() const           { return m_connections; }

        AZ_FORCE_INLINE AnimGraphNode* GetParentNode() const                        { return m_parentNode; }
        AZ_FORCE_INLINE void SetParentNode(AnimGraphNode* node)                     { m_parentNode = node; }

        /**
         * Check if the given node is the parent or the parent of the parent etc. of the node.
         * @param[in] node The parent node we try to search.
         * @result True in case the given node is the parent or the parent of the parent etc. of the node, false in case the given node wasn't found in any of the parents.
         */
        virtual bool RecursiveIsParentNode(const AnimGraphNode* node) const;

        /**
         * Check if the given node is a child or a child of a child etc. of the node.
         * @param[in] node The child node we try to search.
         * @result True in case the given node is a child or the child of a child etc. of the node, false in case the given node wasn't found in any of the child nodes.
         */
        bool RecursiveIsChildNode(AnimGraphNode* node) const;

        /**
         * Find child node by name. This will only iterate through the child nodes and isn't a recursive process.
         * @param[in] name The name of the node to search.
         * @return A pointer to the child node with the given name in case of success, in the other case a nullptr pointer will be returned.
         */
        AnimGraphNode* FindChildNode(const char* name) const;

        /**
         * Find child node by id. This will only iterate through the child nodes and isn't a recursive process.
         * @param[in] childId The id of the node to search.
         * @return A pointer to the child node with the given id in case of success, in the other case a nullptr pointer will be returned.
         */
        AnimGraphNode* FindChildNodeById(AnimGraphNodeId childId) const;

        /**
         * Find child node index by name. This will only iterate through the child nodes and isn't a recursive process.
         * @param[in] name The name of the node to search.
         * @return The index of the child node with the given name in case of success, in the other case MCORE_INVALIDINDEX32 will be returned.
         */
        size_t FindChildNodeIndex(const char* name) const;

        /**
         * Find child node index. This will only iterate through the child nodes and isn't a recursive process.
         * @param[in] node A pointer to the node for which we want to find the child node index.
         * @return The index of the child node in case of success, in the other case MCORE_INVALIDINDEX32 will be returned.
         */
        size_t FindChildNodeIndex(AnimGraphNode* node) const;

        AnimGraphNode* FindFirstChildNodeOfType(const AZ::TypeId& nodeType) const;

        /**
         * Check if a child node of the given type exists. This will only iterate through the child nodes and isn't a recursive process.
         * @param[in] nodeType The rtti type id of the node to check for.
         * @return True in case a child node of the given type was found, false if not.
         */
        bool HasChildNodeOfType(const AZ::TypeId& nodeType) const;

        size_t RecursiveCalcNumNodes() const;
        size_t RecursiveCalcNumNodeConnections() const;

        void CopyBaseNodeTo(AnimGraphNode* node) const;

        MCORE_INLINE size_t GetNumChildNodes() const                        { return m_childNodes.size(); }
        MCORE_INLINE AnimGraphNode* GetChildNode(size_t index) const       { return m_childNodes[index]; }
        const AZStd::vector<AnimGraphNode*>& GetChildNodes() const         { return m_childNodes; }

        void SetNodeInfo(const AZStd::string& info);
        const AZStd::string& GetNodeInfo() const;

        void AddChildNode(AnimGraphNode* node);
        void ReserveChildNodes(size_t numChildNodes);

        void RemoveChildNode(size_t index, bool delFromMem = true);
        void RemoveChildNodeByPointer(AnimGraphNode* node, bool delFromMem = true);
        void RemoveAllChildNodes(bool delFromMem = true);
        bool CheckIfHasChildOfType(const AZ::TypeId& nodeType) const;    // non-recursive

        void MarkConnectionVisited(AnimGraphNode* sourceNode);
        void OutputIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeToOutput);

        MCORE_INLINE AnimGraphNodeData* FindOrCreateUniqueNodeData(AnimGraphInstance* animGraphInstance) const { return animGraphInstance->FindOrCreateUniqueNodeData(this); }

        bool GetIsEnabled() const;
        void SetIsEnabled(bool enabled);
        bool GetIsCollapsed() const;
        void SetIsCollapsed(bool collapsed);
        void SetVisualizeColor(const AZ::Color& color);
        const AZ::Color& GetVisualizeColor() const;
        void SetVisualPos(int32 x, int32 y);
        int32 GetVisualPosX() const;
        int32 GetVisualPosY() const;
        bool GetIsVisualizationEnabled() const;
        void SetVisualization(bool enabled);

        bool HierarchicalHasError(AnimGraphObjectData* uniqueData, bool onlyCheckChildNodes = false) const;
        void SetHasError(AnimGraphObjectData* uniqueData, bool hasError);

        // collect internal objects
        void RecursiveCollectObjects(AZStd::vector<AnimGraphObject*>& outObjects) const override;
        virtual void RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled);

        void FilterEvents(AnimGraphInstance* animGraphInstance, EEventMode eventMode, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float localWeight, AnimGraphRefCountedData* refData);
        void FilterEvents(AnimGraphInstance* animGraphInstance, EEventMode eventMode, AnimGraphRefCountedData* refDataNodeA, AnimGraphNode* nodeB, float localWeight, AnimGraphRefCountedData* refData);

        bool GetCanVisualize(AnimGraphInstance* animGraphInstance) const;

        MCORE_INLINE size_t GetNodeIndex() const                                            { return m_nodeIndex; }
        MCORE_INLINE void SetNodeIndex(size_t index)                                        { m_nodeIndex = index; }

        void ResetPoseRefCount(AnimGraphInstance* animGraphInstance);
        MCORE_INLINE void IncreasePoseRefCount(AnimGraphInstance* animGraphInstance)           { FindOrCreateUniqueNodeData(animGraphInstance)->IncreasePoseRefCount(); }
        MCORE_INLINE void DecreasePoseRefCount(AnimGraphInstance* animGraphInstance)           { FindOrCreateUniqueNodeData(animGraphInstance)->DecreasePoseRefCount(); }
        MCORE_INLINE uint32 GetPoseRefCount(AnimGraphInstance* animGraphInstance) const        { return animGraphInstance->FindOrCreateUniqueNodeData(this)->GetPoseRefCount(); }

        void ResetRefDataRefCount(AnimGraphInstance* animGraphInstance);
        MCORE_INLINE void IncreaseRefDataRefCount(AnimGraphInstance* animGraphInstance)        { FindOrCreateUniqueNodeData(animGraphInstance)->IncreaseRefDataRefCount(); }
        MCORE_INLINE void DecreaseRefDataRefCount(AnimGraphInstance* animGraphInstance)        { FindOrCreateUniqueNodeData(animGraphInstance)->DecreaseRefDataRefCount(); }
        MCORE_INLINE uint32 GetRefDataRefCount(AnimGraphInstance* animGraphInstance) const     { return FindOrCreateUniqueNodeData(animGraphInstance)->GetRefDataRefCount(); }

        // Returns an attribute string (MCore::CommandLine formatted) if this condition is affected by a convertion of
        // node ids. The method will return the attribute string that will be used to patch this condition on a command
        virtual void GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        size_t                                      m_nodeIndex;
        AZ::u64                                     m_id;
        AZStd::vector<BlendTreeConnection*>         m_connections;
        AZStd::vector<Port>                         m_inputPorts;
        AZStd::vector<Port>                         m_outputPorts;
        AZStd::vector<AnimGraphNode*>               m_childNodes;
        TriggerActionSetup                          m_actionSetup;
        AnimGraphNode*                              m_parentNode;
        void*                                       m_customData;
        AZ::Color                                   m_visualizeColor;
        AZStd::string                               m_name;
        AZStd::string                               m_nodeInfo;
        int32                                       m_posX;
        int32                                       m_posY;
        bool                                        m_disabled;
        bool                                        m_visEnabled;
        bool                                        m_isCollapsed;

        virtual void Output(AnimGraphInstance* animGraphInstance);
        virtual void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);
        virtual void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        void RecursiveCountChildNodes(size_t& numNodes) const;
        void RecursiveCountNodeConnections(size_t& numConnections) const;
    };
} // namespace EMotionFX
