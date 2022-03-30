/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>


namespace AzToolsFramework
{
    //! @class ViewBookmark
    //! @brief struct that store viewport camera properties that can be serialized and loaded
    struct ViewBookmark
    {
        AZ_CLASS_ALLOCATOR(ViewBookmark, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ViewBookmark, "{9D6601B9-922F-4E90-BEB2-4D3D709DADD7}");

        ViewBookmark() = default;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ViewBookmark>()
                    ->Version(0)
                    ->Field("Position", &ViewBookmark::m_position)
                    ->Field("Rotation", &ViewBookmark::m_rotation);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ViewBookmark>("ViewBookmark Data", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "ViewBookmark")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Vector3, &ViewBookmark::m_position, "Position", "")
                        ->DataElement(AZ::Edit::UIHandlers::Vector3, &ViewBookmark::m_rotation, "Rotation", "");
                }
            }
        }

        bool operator==(const ViewBookmark& other) const
        {
            return m_position == other.m_position && m_rotation == other.m_rotation;
        }

        bool operator!=(const ViewBookmark& other) const
        {
            return !(*this == other);
        }

        bool IsZero() const
        {
            return m_position == AZ::Vector3::CreateZero() && m_rotation == AZ::Vector3::CreateZero();
        }

        AZ::Vector3 m_position = AZ::Vector3::CreateZero();

        //! Rotation in radians.
        AZ::Vector3 m_rotation = AZ::Vector3::CreateZero();

    };

    //! @class ViewBookmarkIntereface
    //! @brief Interface for saving/loading View Bookmarks.
    class ViewBookmarkLoaderInterface
    {
    public:
        AZ_RTTI(ViewBookmarkLoaderInterface, "{71E7E178-4107-4975-A6E6-1C4B005C981A}")

        virtual bool SaveBookmark(const ViewBookmark& bookmark) = 0;
        virtual bool ModifyBookmarkAtIndex(const ViewBookmark& bookmark, int index) = 0;
        virtual bool SaveLastKnownLocation(const ViewBookmark& bookmark) = 0;
        virtual AZStd::optional<ViewBookmark> LoadBookmarkAtIndex(int index) = 0;
        virtual AZStd::optional<ViewBookmark> LoadLastKnownLocation() const = 0;
        virtual bool RemoveBookmarkAtIndex(int index) = 0;
    };
} // namespace AzToolsFramework
