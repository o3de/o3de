/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

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
    //! @brief Interface for saving/loading ViewBookmarks.
    class ViewBookmarkInterface
    {
    public:
        AZ_RTTI(ViewBookmarkInterface, "{71E7E178-4107-4975-A6E6-1C4B005C981A}")

        virtual bool SaveBookmarkAtIndex(const ViewBookmark& bookmark, int index) = 0;
        virtual bool SaveLastKnownLocation(const ViewBookmark& bookmark) = 0;
        virtual AZStd::optional<ViewBookmark> LoadBookmarkAtIndex(int index) = 0;
        virtual AZStd::optional<ViewBookmark> LoadLastKnownLocation() = 0;
        virtual bool RemoveBookmarkAtIndex(int index) = 0;
    };

    //! Provides the ability to override how the SettingsRegistry data is persisted.
    class ViewBookmarkPersistInterface
    {
    public:
        AZ_RTTI(ViewBookmarkPersistInterface, "{16D3997B-DE3E-42FB-8F0B-39DF0ED8FA24}")

        //! Writable stream interface
        //! Accepts a filename and contents buffer, it will pass the buffer to the third parameter and output to the stream provided.
        using StreamWriteFn = AZStd::function<bool(
            const AZStd::string& localBookmarksFileName,
            const AZStd::string& stringBuffer,
            AZStd::function<bool(AZ::IO::GenericStream& genericStream, const AZStd::string& stringBuffer)>)>;
        //! Readable interface
        //! Will load the file name provided (using project for full path) and return the contents of the file.
        using StreamReadFn = AZStd::function<AZStd::vector<char>(const AZStd::string& localBookmarksFileName)>;
        // Interface to determine if a file (using project for full path) with the name provided exists already.
        using FileExistsFn = AZStd::function<bool(const AZStd::string& localBookmarksFileName)>;

        //! Overrides the behavior of writing to a stream.
        //! @note By default this will write to a file on disk.
        virtual void OverrideStreamWriteFn(StreamWriteFn streamWriteFn) = 0;
        //! Overrides the behavior of reading from a stream.
        //! @note By default this will read from a file on disk.
        virtual void OverrideStreamReadFn(StreamReadFn streamReadFn) = 0;
        //! Overrides the check for if the persistent View Bookmark Settings Registry exists or not.
        //! @note By default this will check for a file on disk.
        virtual void OverrideFileExistsFn(FileExistsFn fileExistsFn) = 0;
    };
} // namespace AzToolsFramework
