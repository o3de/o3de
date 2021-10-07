/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>
#include <GradientSignal/Ebuses/GradientPreviewRequestBus.h>
#include <GradientSignal/Ebuses/GradientPreviewRequestBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <Vegetation/Ebuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/AreaInfoBus.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    /* Base class for editor vegetation area components.
    * Allows area editor components to drive gradient preview settings.
    */

    template<typename TComponent, typename TConfiguration>
    class EditorAreaComponentBase
        : public EditorVegetationComponentBase<TComponent, TConfiguration>
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , protected GradientSignal::GradientPreviewContextRequestBus::Handler
        , protected LmbrCentral::DependencyNotificationBus::Handler
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<TComponent, TConfiguration>;

        AZ_RTTI((EditorAreaComponentBase, "{403D99B0-68E9-4FA2-B7AE-D2B6DDD9130E}", TComponent, TConfiguration), BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        void Activate() override;
        void Deactivate() override;

        using BaseClassType::GetEntityId;
        using BaseClassType::SetDirty;

        using WrappedComponentType = TComponent;
        using WrappedConfigType = TConfiguration;

    protected:

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus
        void OnCompositionChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // GradientPreviewContextRequestBus::Handler
        AZ::EntityId GetPreviewEntity() const override;
        AZ::Aabb GetPreviewBounds() const override;
        bool GetConstrainToShape() const override;

        GradientSignal::GradientPreviewContextPriority GetPreviewContextPriority() const override;

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EntitySelectionEvents::Bus::Handler
        void OnSelected() override;
        void OnDeselected() override;

        AZ::u32 ConfigurationChanged() override;

    private:
        AZ::u32 GetPreviewGroupVisibility() const;
        AZ::u32 GetPreviewPositionVisibility() const;
        AZ::u32 GetPreviewSizeVisibility() const;
        AZ::u32 GetPreviewConstrainToShapeVisibility() const;
        AZ::u32 PreviewSettingsAndSettingsVisibilityChanged() const;
        void UpdatePreviewSettings() const;
        bool m_overridePreviewSettings = false;
        AZ::EntityId m_previewEntityId;
        AZ::Vector3 m_previewPosition = AZ::Vector3(0.0f);
        AZ::Vector3 m_previewSize = AZ::Vector3(1.0f); //1m sq preview...arbitrary default box size in meters chosen by design 
        bool m_constrainToShape = false;
    };

} // namespace Vegetation

#include "EditorAreaComponentBase.inl"
