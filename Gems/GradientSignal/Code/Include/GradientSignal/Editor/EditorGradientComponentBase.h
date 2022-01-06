/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <GradientSignal/Editor/EditorGradientTypeIds.h>
#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientPreviewRequestBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

#include <SurfaceData/SurfaceDataSystemRequestBus.h>

namespace GradientSignal
{
    //////////////////////////////////////////////////////////////////////////
    // Set of helpers so we can set the preview entityId on configurations that have a gradient sampler
    template <typename T, typename = int>
    struct HasGradientSampler
        : AZStd::false_type {};

    template <typename T>
    struct HasGradientSampler <T, decltype((void)T::m_gradientSampler, 0)>
        : AZStd::true_type {};

    // Should be specialized to true_type for any class that needs custom handling
    template<typename T>
    struct HasCustomSetSamplerOwner
        : AZStd::false_type {};

    template<typename T>
    bool ValidateGradientEntityIds([[maybe_unused]] T& configuration, AZStd::enable_if_t<!HasGradientSampler<T>::value && !HasCustomSetSamplerOwner<T>::value>* = nullptr) { return true; }

    template<typename T>
    bool ValidateGradientEntityIds(T& configuration, AZStd::enable_if_t<HasGradientSampler<T>::value>* = nullptr)
    {
        return configuration.m_gradientSampler.ValidateGradientEntityId();
    }

    template<typename T>
    void SetSamplerOwnerEntity([[maybe_unused]] T& configuration, [[maybe_unused]] AZ::EntityId entityId, AZStd::enable_if_t<!HasGradientSampler<T>::value && !HasCustomSetSamplerOwner<T>::value>* = nullptr) {}

    template<typename T>
    void SetSamplerOwnerEntity(T& configuration, AZ::EntityId entityId, AZStd::enable_if_t<HasGradientSampler<T>::value>* = nullptr)
    {
        configuration.m_gradientSampler.m_ownerEntityId = entityId;
    }

    //////////////////////////////////////////////////////////////////////////

    template<typename TComponent, typename TConfiguration>
    class EditorGradientComponentBase
        : public LmbrCentral::EditorWrappedComponentBase<TComponent, TConfiguration>
        , protected GradientPreviewContextRequestBus::Handler
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , protected LmbrCentral::DependencyNotificationBus::Handler
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TComponent, TConfiguration>;
        using BaseClassType::GetEntityId;
        using BaseClassType::SetDirty;

        AZ_RTTI((EditorGradientComponentBase, "{7C529503-AD3F-4EAB-9AB1-E4BCF8EDA114}", TComponent, TConfiguration), BaseClassType);

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus
        void OnCompositionChanged() override;

    protected:
        using BaseClassType::m_component;
        using BaseClassType::m_configuration;

        AZ::u32 ConfigurationChanged() override;

        // This is used by the preview so we can pass an invalid entity Id if our component is disabled
        AZ::EntityId GetGradientEntityId() const;

    private:

        //////////////////////////////////////////////////////////////////////////
        // GradientPreviewContextRequestBus::Handler
        AZ::EntityId GetPreviewEntity() const override;
        AZ::Aabb GetPreviewBounds() const override;
        bool GetConstrainToShape() const override;

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EntitySelectionEvents::Bus::Handler
        void OnSelected() override;
        void OnDeselected() override;

        AZ::u32 GetPreviewPositionVisibility() const;
        AZ::u32 GetPreviewSizeVisibility() const;
        AZ::u32 GetPreviewConstrainToShapeVisibility() const;
        AZ::u32 PreviewSettingsAndSettingsVisibilityChanged() const;
        void UpdatePreviewSettings() const;
        AzToolsFramework::EntityIdList CancelPreviewRendering() const;
    private:
        AZ::EntityId m_previewEntityId;
        AZ::Vector3 m_previewPosition = AZ::Vector3(0.0f);
        AZ::Vector3 m_previewSize = AZ::Vector3(1.0f); //1m sq preview...arbitrary default box size in meters chosen by design 
        bool m_constrainToShape = false;
        AZ::EntityId m_gradientEntityId;
    };

} // namespace GradientSignal

#include "EditorGradientComponentBase.inl"
