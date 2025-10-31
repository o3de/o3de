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
#include <SceneAPI/SceneData/Rules/CommentRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(CommentRule, AZ::SystemAllocator)

            const AZStd::string& CommentRule::GetComment() const
            {
                return m_comment;
            }

            void CommentRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<CommentRule, DataTypes::ICommentRule>()->Version(1)
                    ->Field("comment", &CommentRule::m_comment);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<CommentRule>("Comment", "Add an optional comment to the asset's properties.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement("MultiLineEdit", &CommentRule::m_comment, "", "Text for the comment.")
                            ->Attribute("PlaceholderText", "Add comment text here");
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
