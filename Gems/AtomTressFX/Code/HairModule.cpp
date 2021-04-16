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
