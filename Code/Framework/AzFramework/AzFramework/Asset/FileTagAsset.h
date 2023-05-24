/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/string.h>


namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    namespace FileTag
    {
        enum class FilePatternType : AZ::u8
        {
            //! The pattern is an eaxct match
            Exact = 0,
            //! The pattern is a file wildcard pattern (glob)
            Wildcard,
            //! The pattern is a regular expression pattern
            Regex,
        };

        //! File Tag Data stores all the information related to the FileTagAsset.
        struct FileTagData
        {
            AZ_TYPE_INFO(FileTagData, "{5F66E43B-548B-4AA8-8CD8-F6924F6031E6}");
            FileTagData(AZStd::set<AZStd::string> fileTags, FilePatternType filePatternType = FilePatternType::Exact, const AZStd::string& comment = "");
            FileTagData() = default;
            static void Reflect(AZ::ReflectContext* context);

            FilePatternType m_filePatternType = FilePatternType::Exact;
            AZStd::set<AZStd::string> m_fileTags;
            AZStd::string m_comment;
        };

        using FileTagMap = AZStd::map<AZStd::string, AzFramework::FileTag::FileTagData>;

        class FileTagAsset
            : public AZ::Data::AssetData
        {
        public:
            AZ_RTTI(FileTagAsset, "{F3BE5CAB-85B7-44B7-9495-863863F6B267}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(FileTagAsset, AZ::SystemAllocator);

            static const char* GetDisplayName();
            static const char* GetGroup();
            static const char* Extension();

            static void Reflect(AZ::ReflectContext* context);

            FileTagMap m_fileTagMap;
        };
    }
}
