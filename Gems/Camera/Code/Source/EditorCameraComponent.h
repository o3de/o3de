/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Components/CameraBus.h>

#include <AzFramework/Components/EditorEntityEvents.h>
#include "CameraComponent.h"
#include "CameraComponentController.h"

#include <Atom/RPI.Public/Base.h>

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// The CameraComponent holds all of the data necessary for a camera.
    /// Get and set data through the CameraRequestBus or TransformBus
    //////////////////////////////////////////////////////////////////////////
    using EditorCameraComponentBase = AzToolsFramework::Components::EditorComponentAdapter<CameraComponentController, CameraComponent, CameraComponentConfig>;
    class EditorCameraComponent
        : public EditorCameraComponentBase
        , public EditorCameraViewRequestBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorCameraComponent, EditorCameraComponentTypeId, AzToolsFramework::Components::EditorComponentBase);
        virtual ~EditorCameraComponent() = default;

        static void Reflect(AZ::ReflectContext* reflection);

        // AZ::Component interface
        void Activate() override;
        void Deactivate() override;
        AZ::u32 OnConfigurationChanged() override;

        // AzFramework::DebugDisplayRequestBus::Handler interface
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        /// EditorCameraViewRequestBus::Handler interface
        void ToggleCameraAsActiveView() override { OnPossessCameraButtonClicked(); }
        bool GetCameraState(AzFramework::CameraState& cameraState) override;

    protected:
        void EditorDisplay(AzFramework::DebugDisplayRequests& displayInterface, const AZ::Transform& world);
        AZ::Crc32 OnPossessCameraButtonClicked();
        AZStd::string GetCameraViewButtonText() const;

        float m_frustumViewPercentLength = 1.f;
        AZ::Color m_frustumDrawColor = AzFramework::ViewportColors::HoverColor;
    };
} // Camera
