/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxColliderConfiguration.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(WhiteBoxColliderConfiguration, AZ::SystemAllocator)

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
