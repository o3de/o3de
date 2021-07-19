/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

#include <Atom/Component/DebugCamera/ArcBallControllerComponent.h>
#include <Atom/Component/DebugCamera/CameraComponent.h>
#include <Atom/Component/DebugCamera/NoClipControllerComponent.h>

namespace AZ
{
    namespace Debug
    {
        class CameraModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(DebugCameraModule, "{C4F5D301-5C7F-42C2-8326-08F685B2D7A3}", AZ::Module);
            
            CameraModule()
            {
                m_descriptors.insert(m_descriptors.end(), {
                    CameraControllerComponent::CreateDescriptor(),
                    ArcBallControllerComponent::CreateDescriptor(),
                    CameraComponent::CreateDescriptor(),
                    NoClipControllerComponent::CreateDescriptor(),
                });
            }

            /**
            * Add required SystemComponents to the SystemEntity.
            */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                AZ::ComponentTypeList required;
                return required;
            }
        };
    } // namespace Debug
} // namespace AZ

  // DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
  // The first parameter should be GemName_GemIdLower
  // The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_Component_DebugCamera, AZ::Debug::CameraModule)
