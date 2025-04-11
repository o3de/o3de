/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneData/Rules/TagRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(TagRule, AZ::SystemAllocator, 0)

            const AZStd::vector<AZStd::string>& TagRule::GetTags() const
            {
                return m_tags;
            }

            void TagRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<TagRule, DataTypes::ITagRule>()
                    ->Version(1)
                    ->Field("tags", &TagRule::m_tags);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<TagRule>("Tags", "Add tags.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &TagRule::m_tags, "", "Text for the comment.");
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
