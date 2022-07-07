/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/ObjectAffectedByParameterChanges.h>
#include <Integration/Assets/AnimGraphAsset.h>
#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/System/SystemCommon.h>


namespace EMotionFX
{
    class AnimGraphModelFixture;

    class EMFX_API AnimGraphReferenceNode
        : public AnimGraphNode
        , private AZ::Data::AssetBus::MultiHandler
        , public ObjectAffectedByParameterChanges
    {
    public:
        AZ_RTTI(AnimGraphReferenceNode, "{61B29119-CF2F-4CC6-8872-CC6BB5A198FD}", AnimGraphNode, ObjectAffectedByParameterChanges)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* parentAnimGraphInstance);
            ~UniqueData();

            void Update() override;

            void OnReferenceAnimGraphAssetChanged();
            AnimGraphInstance* m_referencedAnimGraphInstance = nullptr;

            // Cache the mappings.
            // During Update, parameter values that are not coming from an upstream connections
            // are going to taken from parent anim graphs (if name and type matches). This information
            // is cached with the following structure.
            struct ValueParameterMappingCacheEntry
            {
                ValueParameterMappingCacheEntry(AnimGraphInstance* sourceAnimGraphInstance,
                    uint32 sourceValueParameterIndex,
                    uint32 targetValueParameterIndex)
                    : m_sourceAnimGraphInstance(sourceAnimGraphInstance)
                    , m_sourceValueParameterIndex(sourceValueParameterIndex)
                    , m_targetValueParameterIndex(targetValueParameterIndex)
                {}

                AnimGraphInstance*       m_sourceAnimGraphInstance;
                uint32                   m_sourceValueParameterIndex;
                uint32                   m_targetValueParameterIndex;
            };
            AZStd::vector<ValueParameterMappingCacheEntry> m_parameterMappingCache;
            bool m_parameterMappingCacheDirty = true;
        };


        AnimGraphReferenceNode();
        ~AnimGraphReferenceNode();

        void Reinit() override;
        void RecursiveReinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        void RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet) override;
        void Rewind(AnimGraphInstance* animGraphInstance) override;

        void RecursiveInvalidateUniqueDatas(AnimGraphInstance* animGraphInstance) override;
        void RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToDisable = 0xffffffff) override;

        void RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled) override;

        void RecursiveCollectActiveNodes(AnimGraphInstance* animGraphInstance, AZStd::vector<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType) const override;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override;
        void RecursiveCollectObjects(AZStd::vector<AnimGraphObject*>& outObjects) const override;
        void RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<AnimGraphObject*>& outObjects) const override;

        bool RecursiveDetectCycles(AZStd::unordered_set<const AnimGraphNode*>& nodes) const override;

        AZ::Color GetVisualColor() const override                   { return AZ::Color(0.64f, 0.42f, 0.58f, 1.0f); }
        bool GetCanActAsState() const override                      { return true; }
        bool GetSupportsVisualization() const override              { return true; }
        bool GetHasOutputPose() const override                      { return true; }
        bool GetHasVisualOutputPorts() const override               { return true; }
        bool GetCanHaveOnlyOneInsideParent() const override         { return false; }
        bool GetHasVisualGraph() const override;
        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;
        bool GetHasCycles() const                                   { return m_hasCycles; }

        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::MultiHandler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        void SetAnimGraphAsset(AZ::Data::Asset<Integration::AnimGraphAsset> asset);

        AnimGraph* GetReferencedAnimGraph() const;
        MotionSet* GetMotionSet() const;
        AZ::Data::Asset<Integration::AnimGraphAsset> GetReferencedAnimGraphAsset() const;
        AZ::Data::Asset<Integration::MotionSetAsset> GetReferencedMotionSetAsset() const;
        AnimGraphInstance* GetReferencedAnimGraphInstance(AnimGraphInstance* animGraphInstance) const;

        // ObjectAffectedByParameterChanges
        AZStd::vector<AZStd::string> GetParameters() const override;
        AnimGraph* GetParameterAnimGraph() const override;
        void ParameterMaskChanged(const AZStd::vector<AZStd::string>& newParameterMask) override;
        void AddRequiredParameters(AZStd::vector<AZStd::string>& parameterNames) const override;
        void ParameterAdded(const AZStd::string& newParameterName) override;
        void ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName) override;
        void ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange) override;
        void ParameterRemoved(const AZStd::string& oldParameterName) override;

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        
        AZ::Data::Asset<Integration::MotionSetAsset>* GetMotionSetAsset() { return &m_motionSetAsset; }
        bool HasMotionSetAsset() const { return static_cast<bool>(m_motionSetAsset); }

        void ReleaseAnimGraphInstances();

        // Callbacks from the Reflected Property Editor
        void OnAnimGraphAssetChanged();
        void OnMotionSetAssetChanged();
        void OnMotionSetChanged();
        void OnMaskedParametersChanged();

        void LoadAnimGraphAsset();
        void LoadMotionSetAsset();
        
        void OnAnimGraphAssetReady();
        void OnMotionSetAssetReady();
        
        void ReinitMaskedParameters();
        void ReinitInputPorts();
        
        void UpdateParameterMappingCache(AnimGraphInstance* animGraphInstance);

        AZ::Data::Asset<Integration::AnimGraphAsset> m_animGraphAsset;
        AZ::Data::Asset<Integration::MotionSetAsset> m_motionSetAsset;
        AZStd::string                                m_activeMotionSetName;

        // Since changing the anim graph asset could trigger its destructor (since
        // it could be the last anim graph being used) and produce anim graph instances
        // to be destroyed, invalidating the data we have in unique data, we are going
        // to cache a second anim graph asset and update it after we processed the 
        // unique data
        uint32 m_lastProcessedAnimGraphId = MCORE_INVALIDINDEX32;

        // When a different anim graph is set, we are going to select all 
        // the parameters that can't be mapped automatically. Only parameters
        // not in this list are going to be tried to be mapped. If the user changes
        // the parameters names, then those excluded will try to be mapped.
        // Parameters in this list will have ports and will allow to be connected
        // If a parameter is in this list it means that it wasn't mapped
        AZStd::vector<AZStd::string>                 m_maskedParameterNames;

        bool                                         m_hasCycles;
        bool                                         m_reinitMaskedParameters;

        // Cache the value parameter indices that are selected to be inputs
        // The index in the vector is the portid. This is used during Update to get 
        // the upstream values faster
        AZStd::vector<uint32>                        m_parameterIndexByPortIndex;
    };

}   // namespace EMotionFX
