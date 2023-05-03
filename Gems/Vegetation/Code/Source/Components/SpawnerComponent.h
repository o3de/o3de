/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <Vegetation/DescriptorListAsset.h>
#include <Vegetation/Ebuses/DescriptorSelectorRequestBus.h>
#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/Ebuses/ModifierRequestBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/Ebuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/SpawnerRequestBus.h>
#include <Vegetation/AreaComponentBase.h>
#include <SurfaceData/SurfaceDataTypes.h>

#define VEG_SPAWNER_ENABLE_CACHING 0
#define VEG_SPAWNER_ENABLE_RELATIVE 0

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class SpawnerConfig
        : public AreaConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SpawnerConfig, AZ::SystemAllocator);
        AZ_RTTI(SpawnerConfig, "{98A6B0CE-FAD0-4108-B019-6B01931E649F}", AreaConfig);
        static void Reflect(AZ::ReflectContext* context);
        bool m_inheritBehavior = true;
        bool m_allowEmptyMeshes = true;
        FilterStage m_filterStage = FilterStage::PreProcess;
    };

    inline constexpr AZ::TypeId SpawnerComponentTypeId{ "{14BD176C-2E44-4BA6-849A-258674179237}" };

    /**
    * Default placement logic for vegetation in an area
    */
    class SpawnerComponent
        : public AreaComponentBase
        , private SpawnerRequestBus::Handler
    {
    public:
        friend class EditorSpawnerComponent;
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SpawnerComponent, SpawnerComponentTypeId, AreaComponentBase);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SpawnerComponent(const SpawnerConfig& configuration);
        SpawnerComponent() = default;
        ~SpawnerComponent() = default;

        //////////////////////////////////////////////////////////////////////////
       // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // AreaRequestBus
        bool PrepareToClaim(EntityIdStack& stackIds) override;
        void ClaimPositions(EntityIdStack& stackIds, ClaimContext& context) override;
        void UnclaimPosition(const ClaimHandle handle) override;

        //////////////////////////////////////////////////////////////////////////
        // AreaInfoBus
        AZ::Aabb GetEncompassingAabb() const override;
        AZ::u32 GetProductCount() const override;

        //////////////////////////////////////////////////////////////////////////
        // AreaNotificationBus
        void OnCompositionChanged() override;


    protected:
        //////////////////////////////////////////////////////////////////////////
        // AreaComponentBase
        void OnRegisterArea() override;
        void OnUnregisterArea() override;

        //////////////////////////////////////////////////////////////////////////
        // SpawnerRequestBus

        AZ::u32 GetAreaPriority() const override;
        void SetAreaPriority(AZ::u32 priority) override;
        AZ::u32 GetAreaLayer() const override;
        void SetAreaLayer(AZ::u32 type) override;
        AZ::u32 GetAreaProductCount() const override;
        bool GetInheritBehavior() const override;
        void SetInheritBehavior(bool value) override;
        bool GetAllowEmptyMeshes() const override;
        void SetAllowEmptyMeshes(bool value) override;
        FilterStage GetFilterStage() const override;
        void SetFilterStage(FilterStage filterStage) override;

    private:
        void ClearSelectableDescriptors();
        bool CreateInstance(const ClaimPoint &point, InstanceData& instanceData);
        bool EvaluateFilters(EntityIdStack& processedIds, InstanceData& instanceData, const FilterStage intendedStage) const;
        bool ProcessInstance(EntityIdStack& processedIds, const ClaimPoint& point, InstanceData& instanceData, DescriptorPtr descriptorPtr);
        bool ClaimPosition(EntityIdStack& processedIds, const ClaimPoint& point, InstanceData& instanceData);
        void DestroyAllInstances();
        void CalcInstanceDebugColor(const EntityIdStack& processedIds);

        SpawnerConfig m_configuration;

        //caching vector for reuse per point
        AZStd::recursive_mutex m_selectableDescriptorMutex;
        DescriptorPtrVec m_selectableDescriptorCache;
        DescriptorPtrVec m_selectedDescriptors;

        using ClaimInstanceMapping = AZStd::unordered_map<ClaimHandle, InstanceId>;
        ClaimInstanceMapping m_claimInstanceMapping;
        mutable AZStd::recursive_mutex m_claimInstanceMappingMutex;

#if VEG_SPAWNER_ENABLE_CACHING
        // cached data
        AZStd::unordered_map<ClaimHandle, InstanceData> m_acceptedClaimCache;
        AZStd::unordered_set<ClaimHandle> m_rejectedClaimCache;
        mutable AZStd::recursive_mutex m_cacheMutex;
#endif

        SurfaceData::SurfaceTagVector m_inclusiveTagsToConsider;
        SurfaceData::SurfaceTagVector m_exclusiveTagsToConsider;
    };
}
