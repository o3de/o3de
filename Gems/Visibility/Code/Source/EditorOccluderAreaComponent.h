/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include "OccluderAreaComponent.h"
#include "EditorOccluderAreaComponentBus.h"

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Visibility
{
    class EditorOccluderAreaComponent;

    class EditorOccluderAreaConfiguration
        : public OccluderAreaConfiguration
    {
    public:
        AZ_TYPE_INFO_LEGACY(EditorOccluderAreaConfiguration, "{032F466F-25CB-5460-AC2F-B04236C87878}", OccluderAreaConfiguration);
        AZ_CLASS_ALLOCATOR(EditorOccluderAreaConfiguration, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        void OnChange() override;
        void OnVerticesChange() override;

        void SetEntityId(AZ::EntityId entityId);

    private:
        AZ::EntityId m_entityId;
    };

    class EditorOccluderAreaComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private EditorOccluderAreaRequestBus::Handler
        , private AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
    {
        friend class EditorOccluderAreaConfiguration;

        using Base = AzToolsFramework::Components::EditorComponentBase;

    public:
        AZ_COMPONENT(EditorOccluderAreaComponent, "{1A209C7C-6C06-5AE6-AD60-22CD8D0DAEE3}", AzToolsFramework::Components::EditorComponentBase);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        static void Reflect(AZ::ReflectContext* context);

        EditorOccluderAreaComponent() = default;
        EditorOccluderAreaComponent(const EditorOccluderAreaComponent&) = delete;
        EditorOccluderAreaComponent& operator=(const EditorOccluderAreaComponent&) = delete;
        ~EditorOccluderAreaComponent();

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // TransformNotificationBus overrides ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorComponentSelectionRequestsBus overrides ...
        AZ::Aabb GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src,
            const AZ::Vector3& dir, float& distance) override;
        bool SupportsEditorRayIntersect() override { return true; }

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

        /// EditorOccluderAreaRequestBus overrides ...
        void SetDisplayFilled(bool value) override;
        bool GetDisplayFilled() override;
        void SetCullDistRatio(float value) override;
        float GetCullDistRatio() override;
        void SetUseInIndoors(bool value) override;
        bool GetUseInIndoors() override;
        void SetDoubleSide(bool value) override;
        bool GetDoubleSide() override;
        bool GetVertex(size_t index, AZ::Vector3& vertex) const override;
        bool UpdateVertex(size_t index, const AZ::Vector3& vertex) override;
        size_t Size() const override { return m_config.m_vertices.size(); }
        void UpdateOccluderAreaObject() override;

    private:
        friend EditorOccluderAreaConfiguration;

        // AzFramework::EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // Reflected members
        EditorOccluderAreaConfiguration m_config;

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                       ///< and creating a concrete ComponentMode.

        // Unreflected members
        IVisArea* m_area = nullptr;
    };
} // namespace Visibility
