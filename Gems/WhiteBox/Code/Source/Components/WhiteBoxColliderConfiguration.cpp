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

#include "WhiteBox_precompiled.h"

#include "WhiteBoxColliderConfiguration.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(WhiteBoxColliderConfiguration, AZ::SystemAllocator, 0)

    void WhiteBoxColliderConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxColliderConfiguration>()->Version(1)->Field(
                "BodyType", &WhiteBoxColliderConfiguration::m_bodyType);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<WhiteBoxColliderConfiguration>(
                        "White Box Collider Configuration", "White Box collider configuration properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &WhiteBoxColliderConfiguration::m_bodyType, "Body Type",
                        "Set if the White Box Collider will be treated as static or kinematic at runtime.")
                    ->EnumAttribute(WhiteBoxBodyType::Static, "Static")
                    ->EnumAttribute(WhiteBoxBodyType::Kinematic, "Kinematic");
            }
        }
    }
} // namespace WhiteBox
