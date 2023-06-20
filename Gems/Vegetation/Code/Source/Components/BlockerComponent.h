/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Vegetation/Ebuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/BlockerRequestBus.h>
#include <Vegetation/AreaComponentBase.h>

#define VEG_BLOCKER_ENABLE_CACHING 0

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class BlockerConfig
        : public AreaConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(BlockerConfig, AZ::SystemAllocator);
        AZ_RTTI(BlockerConfig, "{01F6E6C5-707E-42EC-91BB-F674B9F51A40}", AreaConfig);
        static void Reflect(AZ::ReflectContext* context);

        BlockerConfig() : AreaConfig() { m_priority = AreaConstants::s_priorityMax; m_layer = AreaConstants::s_foregroundLayer; }
        bool m_inheritBehavior = true;
    };

    inline constexpr AZ::TypeId BlockerComponentTypeId{ "{C8A7AAEB-C315-44CE-919D-F304B53ACA4A}" };

    /**
    * Blocking claim logic for vegetation in an area
    */
    class BlockerComponent
        : public AreaComponentBase
        , private BlockerRequestBus::Handler
    {
    public:
        friend class EditorBlockerComponent;
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(BlockerComponent, BlockerComponentTypeId, AreaComponentBase);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        BlockerComponent(const BlockerConfig& configuration);
        BlockerComponent() = default;
        ~BlockerComponent() = default;

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
        AZ::Aabb GetEncompassingAabb() const override;
        AZ::u32 GetProductCount() const override;

        //////////////////////////////////////////////////////////////////////////
        // AreaNotificationBus
        void OnCompositionChanged() override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // BlockerRequestBus
        AZ::u32 GetAreaPriority() const override;
        void SetAreaPriority(AZ::u32 priority) override;
        AZ::u32 GetAreaLayer() const override;
        void SetAreaLayer(AZ::u32 layer) override;
        AZ::u32 GetAreaProductCount() const override;
        bool GetInheritBehavior() const override;
        void SetInheritBehavior(bool value) override;

    private:
        bool ClaimPosition(EntityIdStack& processedIds, const ClaimPoint& point, InstanceData& instanceData);
        BlockerConfig m_configuration;

#if VEG_BLOCKER_ENABLE_CACHING
        // cached data
        AZStd::recursive_mutex m_cacheMutex;
        using ClaimInstanceMapping = AZStd::unordered_map<ClaimHandle, bool>;
        ClaimInstanceMapping m_claimCacheMapping;
#endif
    };
}
