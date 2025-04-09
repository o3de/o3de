/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Component/DebugCamera/CameraControllerComponent.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Debug
    {
        void CameraControllerComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<CameraControllerComponent, AZ::Component>()
                    ->Version(1);
            }
        }

        void CameraControllerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
            required.push_back(AZ_CRC_CE("CameraService"));
        }

        void CameraControllerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("CameraControllerService"));
        }

        void CameraControllerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        void CameraControllerComponent::Enable(TypeId typeId)
        {
            // Enable this controller if type id matches, otherwise disable this controller
            if (typeId == RTTI_GetType())
            {
                if (!m_enabled)
                {
                    m_enabled = true;
                    OnEnabled();
                }
            }
            else
            {
                if (m_enabled)
                {
                    m_enabled = false;
                    OnDisabled();
                }
            }
        }

        void CameraControllerComponent::Disable()
        {
            if (m_enabled)
            {
                m_enabled = false;
                OnDisabled();
            }
        }

        void CameraControllerComponent::Reset()
        {
            if (m_enabled)
            {
                OnDisabled();
                OnEnabled();
            }
        }
        
        void CameraControllerComponent::Activate()
        {
            CameraControllerRequestBus::Handler::BusConnect(GetEntityId());
        }

        void CameraControllerComponent::Deactivate()
        {
            if (m_enabled)
            {
                m_enabled = false;
                OnDisabled();
            }
            CameraControllerRequestBus::Handler::BusDisconnect();
        }
        
    } // namespace Debug
} // namespace AZ
