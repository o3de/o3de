/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommentComponent.h"
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    void EditorCommentComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorCommentComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Configuration", &EditorCommentComponent::m_comment)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorCommentComponent>(
                    "Comment", "The Comment component allows you to add long-form text comments for component entities")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Comment.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Comment.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level"), AZ_CRC_CE("Game"), AZ_CRC_CE("Layer") }))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/editor/comment/")
                    ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &EditorCommentComponent::m_comment,"", "Comment")
                        ->Attribute(AZ::Edit::Attributes::PlaceholderText, "Add comment text here");
            }
        }
    }
} // namespace LmbrCentral
