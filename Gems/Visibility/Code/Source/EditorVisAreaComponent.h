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

#include <AzCore/Math/Color.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include "EditorVisAreaComponentBus.h"
#include "VisAreaComponent.h"

namespace Visibility
{
    class EditorVisAreaComponent;

    class EditorVisAreaConfiguration
        : public VisAreaConfiguration
    {
    public:
        AZ_TYPE_INFO_LEGACY(EditorVisAreaConfiguration, "{C329E65C-1F34-5C80-9A7A-4B568105256B}", VisAreaConfiguration);
        AZ_CLASS_ALLOCATOR(EditorVisAreaConfiguration, AZ::SystemAllocator,0);

        static void Reflect(AZ::ReflectContext* context);

        void ChangeHeight() override;
        void ChangeDisplayFilled() override;
        void ChangeAffectedBySun() override;
        void ChangeViewDistRatio() override;
        void ChangeOceanIsVisible() override;
        void ChangeVertexContainer() override;

        void SetEntityId(AZ::EntityId entityId);

    private:
        AZ::EntityId m_entityId;
    };

    class EditorVisAreaComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private EditorVisAreaComponentRequestBus::Handler
        , private AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler
        , private AZ::VariableVerticesRequestBus<AZ::Vector3>::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EditorEntityInfoNotificationBus::Handler
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
    {
        friend class EditorVisAreaConfiguration; //So that the config can set m_vertices when the vertex container changes

        using Base = AzToolsFramework::Components::EditorComponentBase;

    public:
        AZ_COMPONENT(EditorVisAreaComponent, "{F4EC32D8-D4DD-54F7-97A8-D195497D5F2C}", AzToolsFramework::Components::EditorComponentBase);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        static void Reflect(AZ::ReflectContext* context);

        EditorVisAreaComponent() = default;
        EditorVisAreaComponent(const EditorVisAreaComponent&) = delete;
        EditorVisAreaComponent& operator=(const EditorVisAreaComponent&) = delete;
        virtual ~EditorVisAreaComponent();

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

        // VisAreaComponentRequestBus overrides ...
        void SetHeight(float height) override;
        float GetHeight() override;
        void SetDisplayFilled(bool filled) override;
        bool GetDisplayFilled() override;
        void SetAffectedBySun(bool affectedBySun) override;
        bool GetAffectedBySun() override;
        void SetViewDistRatio(float viewDistRatio) override;
        float GetViewDistRatio() override;
        void SetOceanIsVisible(bool oceanVisible) override;
        bool GetOceanIsVisible() override;
        bool GetVertex(size_t index, AZ::Vector3& vertex) const override;
        void AddVertex(const AZ::Vector3& vertex) override;
        bool UpdateVertex(size_t index, const AZ::Vector3& vertex) override;
        bool InsertVertex(size_t index, const AZ::Vector3& vertex) override;
        bool RemoveVertex(size_t index) override;
        void SetVertices(const AZStd::vector<AZ::Vector3>& vertices) override;
        void ClearVertices() override;
        size_t Size() const override;
        bool Empty() const override;
        void UpdateVisAreaObject() override;

        // AzFramework::EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

    private:
        // Reflected members
        EditorVisAreaConfiguration m_config;

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                       ///< and creating a concrete ComponentMode.

        // Unreflected members
        AZ::Transform m_currentWorldTransform;
        IVisArea* m_area = nullptr;

        static AZ::Color s_visAreaColor; ///< The orange color that all vis-areas draw with.
    };
} // namespace Visibility
