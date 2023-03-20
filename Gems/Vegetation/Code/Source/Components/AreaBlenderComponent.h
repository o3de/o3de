/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <Vegetation/Ebuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/AreaBlenderRequestBus.h>
#include <Vegetation/AreaComponentBase.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class AreaBlenderConfig
        : public AreaConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(AreaBlenderConfig, AZ::SystemAllocator);
        AZ_RTTI(AreaBlenderConfig, "{ED57731E-2821-4AA6-9BD6-9203ED0B6AB0}", AreaConfig);
        static void Reflect(AZ::ReflectContext* context);
        bool m_inheritBehavior = true;
        bool m_propagateBehavior = true;
        AZStd::vector<AZ::EntityId> m_vegetationAreaIds;

        size_t GetNumAreas() const;
        AZ::EntityId GetAreaEntityId(int index) const;
        void RemoveAreaEntityId(int index);
        void AddAreaEntityId(AZ::EntityId entityId);
    };

    inline constexpr AZ::TypeId AreaBlenderComponentTypeId{ "{899AA751-BC3F-45D8-9D66-07CE72FDC86D}" };

    /**
    * Placement logic for combined vegetation areas
    */
    class AreaBlenderComponent
        : public AreaComponentBase
        , private AreaBlenderRequestBus::Handler        
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(AreaBlenderComponent, AreaBlenderComponentTypeId, AreaComponentBase);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        AreaBlenderComponent(const AreaBlenderConfig& configuration);
        AreaBlenderComponent() = default;
        ~AreaBlenderComponent() = default;

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

        // AreaInfoBus
        AZ::Aabb GetEncompassingAabb() const override;
        AZ::u32 GetProductCount() const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // AreaBlenderRequestBus
        AZ::u32 GetAreaPriority() const override;
        void SetAreaPriority(AZ::u32 priority) override;
        AZ::u32 GetAreaLayer() const override;
        void SetAreaLayer(AZ::u32 layer) override;
        AZ::u32 GetAreaProductCount() const override;
        bool GetInheritBehavior() const override;
        void SetInheritBehavior(bool value) override;
        bool GetPropagateBehavior() const override;
        void SetPropagateBehavior(bool value) override;
        size_t GetNumAreas() const override;
        AZ::EntityId GetAreaEntityId(int index) const override;
        void RemoveAreaEntityId(int index) override;
        void AddAreaEntityId(AZ::EntityId entityId) override;

    private:
        AreaBlenderConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;

        void SetupDependencies();
    };
}
