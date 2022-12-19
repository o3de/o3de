/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneData/Rules/ReadOnlyRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            void ReadOnlyRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<ReadOnlyRule, DataTypes::IReadOnlyRule>()->Version(1);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<ReadOnlyRule>("ReadOnly", "This rule marks the container as read-only.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "");
                }
            }

            bool ReadOnlyRule::ModifyTooltip(AZStd::string& tooltip)
            {
                tooltip = AZStd::string::format("This group is read only. %.*s", AZ_STRING_ARG(tooltip));
                return true;
            }
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
