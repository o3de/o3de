/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <HairModule.h>
#include <Components/HairSystemComponent.h>
#include <Components/HairComponent.h>
#include <Components/EditorHairComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            HairModule::HairModule()
                : AZ::Module()
            {
                m_descriptors.insert(
                    m_descriptors.end(),
                    {
                        HairSystemComponent::CreateDescriptor(),
                        HairComponent::CreateDescriptor(),
//#ifdef ATOMLYINTEGRATION_FEATURE_COMMON_EDITOR
                        EditorHairComponent::CreateDescriptor(),
//#endif
                    });
            }

            AZ::ComponentTypeList HairModule::GetRequiredSystemComponents() const
            {
                // add required SystemComponents to the SystemEntity
                return AZ::ComponentTypeList{
                    azrtti_typeid<HairSystemComponent>()
                };
            }

        } // namespace Hair
    } // namespace Render
} // namespace AZ
