/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/FileTagAsset.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AzFramework
{
    namespace FileTag
    {
        FileTagData::FileTagData(AZStd::set<AZStd::string> fileTags, FilePatternType filePatternType, const AZStd::string& comment)
            : m_filePatternType(filePatternType)
            , m_fileTags(fileTags)
            , m_comment(AZStd::move(comment))
        {
        }

        void FileTagData::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FileTagData>()
                    ->Version(2)
                    ->Field("FilePatternType", &FileTagData::m_filePatternType)
                    ->Field("FileTags", &FileTagData::m_fileTags)
                    ->Field("Comment", &FileTagData::m_comment);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<FileTagData>("Definition", "Files/Patterns and their associated tags.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FileTagData::m_filePatternType,
                            "File Pattern", "File Pattern can either be a regex or a wildcard.")
                        ->Attribute(AZ::Edit::Attributes::EnumValues,
                            AZStd::vector<AZ::Edit::EnumConstant<FilePatternType>>
                    {
                        AZ::Edit::EnumConstant<FilePatternType>(FilePatternType::Exact, "Exact"),
                        AZ::Edit::EnumConstant<FilePatternType>(FilePatternType::Wildcard, "Wildcard"),
                        AZ::Edit::EnumConstant<FilePatternType>(FilePatternType::Regex, "Regex")
                    })
                        ->DataElement(AZ::Edit::UIHandlers::Default, &FileTagData::m_fileTags, "File Tags", "List of tags associated with the file/pattern.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &FileTagData::m_comment, "Comment", "Comment for the file tag definition");
                }
            }
        }

        void FileTagAsset::Reflect(AZ::ReflectContext* context)
        {
            FileTagData::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FileTagAsset>()
                    ->Version(1)
                    ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                    ->Field("FileTagMap", &FileTagAsset::m_fileTagMap);
                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<FileTagAsset>("Definition", "Asset storing all the file/pattern tagging information.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &FileTagAsset::m_fileTagMap, "File Tag Map", "Container for storing file tagging information.");
                }
            }
        }

        const char* FileTagAsset::GetDisplayName()
        {
            return "File Tag";
        }

        const char* FileTagAsset::GetGroup()
        {
            return "FileTag";
        }
        const char* FileTagAsset::Extension()
        {
            return "filetag";
        }
    }
}
