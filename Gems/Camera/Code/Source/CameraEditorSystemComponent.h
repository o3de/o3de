/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorCameraBus.h>

#include "CameraViewRegistrationBus.h"

namespace Camera
{
    class CameraEditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private EditorCameraSystemRequestBus::Handler
        , private CameraViewRegistrationRequestsBus::Handler
        , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(CameraEditorSystemComponent, "{769802EB-722A-4F89-A475-DA396DA1FDCC}");
        static void Reflect(AZ::ReflectContext* context);

        CameraEditorSystemComponent() = default;
        ~CameraEditorSystemComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEvents
        void NotifyRegisterViews() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorCameraRequestBus::Handler
        void CreateCameraEntityFromViewport() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // CameraViewRegistrationRequestsBus::Handler
        void SetViewForEntity(const AZ::EntityId& id, AZ::RPI::ViewPtr view) override;
        AZ::RPI::ViewPtr GetViewForEntity(const AZ::EntityId& id) override;
        //////////////////////////////////////////////////////////////////////////

        // ActionManagerRegistrationNotificationBus overrides ...
        void OnActionRegistrationHook() override;
        void OnMenuBindingHook() override;

        AZStd::map<AZ::EntityId, AZStd::weak_ptr<AZ::RPI::View>> m_entityViewMap;
    };
}
