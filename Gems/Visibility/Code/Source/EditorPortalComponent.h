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

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include "PortalComponent.h"
#include "EditorPortalComponentBus.h"

namespace Visibility
{
    class EditorPortalComponent;
    struct PortalQuadVertices;

    class EditorPortalConfiguration
        : public PortalConfiguration
    {
    public:
        AZ_TYPE_INFO_LEGACY(EditorPortalConfiguration, "{C9F99449-7A77-50C4-9ED3-D69B923BFDBD}", PortalConfiguration);
        AZ_CLASS_ALLOCATOR(EditorPortalConfiguration, AZ::SystemAllocator,0);

        static void Reflect(AZ::ReflectContext* context);

        void OnChange() override;
        void OnVerticesChange() override;

        void SetEntityId(AZ::EntityId entityId);

    private:
        AZ::EntityId m_entityId;
    };

    class EditorPortalComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private EditorPortalRequestBus::Handler
        , private AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
    {
        friend class EditorPortalConfiguration;

        using Base = AzToolsFramework::Components::EditorComponentBase;

    public:
        AZ_COMPONENT(EditorPortalComponent, "{64525CDD-7DD4-5CEF-B545-559127DC834E}", AzToolsFramework::Components::EditorComponentBase);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        static void Reflect(AZ::ReflectContext* context);

        EditorPortalComponent() = default;
        EditorPortalComponent(const EditorPortalComponent&) = delete;
        EditorPortalComponent& operator=(const EditorPortalComponent&) = delete;
        ~EditorPortalComponent() override;

        // EditorComponentBase overrides ...
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

        // PortalRequestBus overrides ...
        void SetHeight(float height) override;
        float GetHeight() override;
        void SetDisplayFilled(bool filled) override;
        bool GetDisplayFilled() override;
        void SetAffectedBySun(bool affectedBySun) override;
        bool GetAffectedBySun() override;
        void SetViewDistRatio(float viewDistRatio) override;
        float GetViewDistRatio() override;
        void SetSkyOnly(bool skyOnly) override;
        bool GetSkyOnly() override;
        void SetOceanIsVisible(bool oceanVisible) override;
        bool GetOceanIsVisible() override;
        void SetUseDeepness(bool useDeepness) override;
        bool GetUseDeepness() override;
        void SetDoubleSide(bool doubleSided) override;
        bool GetDoubleSide() override;
        void SetLightBlending(bool lightBending) override;
        bool GetLightBlending() override;
        void SetLightBlendValue(float lightBendAmount) override;
        float GetLightBlendValue() override;
        bool GetVertex(size_t index, AZ::Vector3& vertex) const override;
        bool UpdateVertex(size_t index, const AZ::Vector3& vertex) override;
        size_t Size() const override { return m_config.m_vertices.size(); }
        void UpdatePortalObject() override;

        // EntityDebugDisplayBus overrides ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

    private:
        enum class VertTranslation
        {
            Keep,
            Remove
        };

        PortalQuadVertices CalculatePortalQuadVertices(VertTranslation vertTranslation);

        // Reflected members
        EditorPortalConfiguration m_config;

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                       ///< and creating a concrete ComponentMode.

        AZ::Transform m_AZCachedWorldTransform;
        Matrix44 m_cryCachedWorldTransform;
        IVisArea* m_area = nullptr;
    };
} // namespace Visibility
