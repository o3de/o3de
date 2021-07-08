/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
