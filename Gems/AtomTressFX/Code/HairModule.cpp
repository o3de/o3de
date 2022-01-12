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
#if defined (ATOMTRESSFX_EDITOR)
    #include <Components/EditorHairComponent.h>
#endif // ATOMTRESSFX_EDITOR

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
#if defined (ATOMTRESSFX_EDITOR)
                        // Prevent adding editor component when editor and tools are not built
                        EditorHairComponent::CreateDescriptor(),
#endif // ATOMTRESSFX_EDITOR
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
