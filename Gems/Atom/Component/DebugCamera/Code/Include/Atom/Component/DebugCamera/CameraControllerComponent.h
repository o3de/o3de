/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <Atom/Component/DebugCamera/CameraControllerBus.h>

namespace AZ
{
    namespace Debug
    {
        /** 
         * CameraControllerComponent is the base class of any camera controller component 
         * which can modify camera's transformation or frustum. 
         * A camera controller usually processes input events and then utilizes the data to modify the entity's transformation.
         * It's allowed to have more than one camera controller component on the same camera entity but there will 
         * be only one controller activated at same time. 
         * The derived class should disable its event handling and updating accordingly when the controller is disabled.
        */
        class CameraControllerComponent
            : public Component
            , public CameraControllerRequestBus::Handler
        {
        public:
            AZ_COMPONENT(CameraControllerComponent, "{A3503719-6DE2-46D0-A54B-922155F4537F}", AZ::Component);

            CameraControllerComponent() = default;
            virtual ~CameraControllerComponent() = default;
            
            static void Reflect(AZ::ReflectContext* context);

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            // CameraControllerRequestBus::Handler overrides
            void Enable(TypeId typeId) override final;
            void Reset() override final;
            void Disable() override final;

            // AZ::Component overrides
            void Activate() override final;
            void Deactivate() override final;
            
        private:
            // The derived components implements these functions for when the controller is enabled/disabled
            virtual void OnEnabled() {}
            virtual void OnDisabled() {}
            
            bool m_enabled = false;
        };

    } // namespace Debug
} // namespace AZ
