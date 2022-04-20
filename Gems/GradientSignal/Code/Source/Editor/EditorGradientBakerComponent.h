/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/SectorDataRequestBus.h>
#include <GradientSignal/Editor/EditorGradientTypeIds.h>
#include <GradientSignal/GradientSampler.h>

#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>


namespace GradientSignal
{
    class GradientBakerConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientBakerConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(GradientBakerConfig, "{C43366FC-6789-4154-848D-DF0F39BAA4E6}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        GradientSampler m_gradientSampler;
        AZ::EntityId m_inputBounds;
    };

    class EditorGradientBakerComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private GradientRequestBus::Handler
        , private GradientPreviewContextRequestBus::Handler
        , private LmbrCentral::DependencyNotificationBus::Handler
        , private SectorDataNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorGradientBakerComponent, EditorGradientBakerComponentTypeId, AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorGradientBakerComponent() = default;

        //! Component overrides ...
        void Activate() override;
        void Deactivate() override;

        //! GradientRequestBus overrides ...
        float GetValue(const GradientSampleParams& sampleParams) const override;
        void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;
        bool IsEntityInHierarchy(const AZ::EntityId& entityId) const override;

        //! LmbrCentral::DependencyNotificationBus overrides ...
        void OnCompositionChanged() override;

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Gradient Baker";
        static constexpr const char* const s_componentDescription = "Bakes out an inbound gradient signal to a streaming image asset";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientBaker.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientBaker.svg";
        static constexpr const char* const s_helpUrl = "";

    protected:
        //! SectorDataNotificationBus overrides ...
        void OnSectorDataConfigurationUpdated() const override;

        //! AzToolsFramework::EntitySelectionEvents overrides ...
        void OnSelected() override;
        void OnDeselected() override;

        //! GradientPreviewContextRequestBus overrides ...
        AZ::EntityId GetPreviewEntity() const override;
        AZ::Aabb GetPreviewBounds() const override;

        void OnConfigurationChanged();

        // This is used by the preview so we can pass an invalid entity Id if our component is disabled
        AZ::EntityId GetGradientEntityId() const;

        void UpdatePreviewSettings() const;
        AzToolsFramework::EntityIdList CancelPreviewRendering() const;

    private:
        GradientBakerConfig m_configuration;
        AZ::EntityId m_gradientEntityId;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}
