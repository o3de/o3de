/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    //! @class ViewBookmark
    //! @brief struct that store viewport camera properties that can be serialized and loaded
    struct ViewBookmark
    {
        AZ_CLASS_ALLOCATOR(ViewBookmark, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ViewBookmark, "{522A38D9-6FFF-4B96-BECF-B4D0F7ABCD25}");

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

        //! Choose storage mode for ViewBookmarks
        //! Shared are stored in the prefab.
        //! Local are stored in the Settings Registry.
        enum class StorageMode : int
        {
            Shared = 0,
            Local = 1,
            Invalid = -1
        };

        virtual bool SaveBookmark(ViewBookmark bookmark, const StorageMode mode) = 0;
        virtual bool ModifyBookmarkAtIndex(ViewBookmark bookmark, int index, const StorageMode mode) = 0;
        virtual bool SaveLastKnownLocation(ViewBookmark bookmark) = 0;
        virtual bool LoadViewBookmarks() = 0;
        virtual AZStd::optional<ViewBookmark> GetBookmarkAtIndex(int index, const StorageMode mode) = 0;
        virtual AZStd::optional<ViewBookmark> LoadLastKnownLocation() const = 0;
        virtual bool RemoveBookmarkAtIndex(int index, const StorageMode mode) = 0;

    private:
        virtual void SaveBookmarkSettingsFile() = 0;
    };
} // namespace AzToolsFramework
