/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builders/HairBuilderComponent.h>
#include <Builders/HairBuilderModule.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            HairBuilderModule::HairBuilderModule()
                : AZ::Module()
            {
                m_descriptors.push_back(HairBuilderComponent::CreateDescriptor());
            }

            AZ::ComponentTypeList HairBuilderModule::GetRequiredSystemComponents() const
            {
                return AZ::ComponentTypeList{azrtti_typeid<HairBuilderComponent>()};
            }

        } // namespace Hair
    } // namespace Render
} // namespace AZ
