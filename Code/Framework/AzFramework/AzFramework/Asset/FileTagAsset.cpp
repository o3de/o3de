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
#include <AzFramework/Translation/TranslationDef.h>

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
                    editContext->Class<FileTagData>(
                        QT_TRANSLATE_NOOP("AzFramework", "Definition"),
                        QT_TRANSLATE_NOOP("AzFramework", "Files/Patterns and their associated tags."))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FileTagData::m_filePatternType,
                            QT_TRANSLATE_NOOP("AzFramework", "File Pattern"),
                            QT_TRANSLATE_NOOP("AzFramework", "File Pattern can either be an exact value, a regex or a wildcard."))
                        ->Attribute(AZ::Edit::Attributes::EnumValues,
                            AZStd::vector<AZ::Edit::EnumConstant<FilePatternType>>
                    {
                        AZ::Edit::EnumConstant<FilePatternType>(FilePatternType::Exact,
                            QT_TRANSLATE_NOOP("AzFramework", "Exact")),
                        AZ::Edit::EnumConstant<FilePatternType>(FilePatternType::Wildcard,
                            QT_TRANSLATE_NOOP("AzFramework", "Wildcard")),
                        AZ::Edit::EnumConstant<FilePatternType>(FilePatternType::Regex,
                            QT_TRANSLATE_NOOP("AzFramework", "Regex"))
                    })
                        ->DataElement(AZ::Edit::UIHandlers::Default, &FileTagData::m_fileTags,
                            QT_TRANSLATE_NOOP("AzFramework", "File Tags"),
                            QT_TRANSLATE_NOOP("AzFramework", "List of tags associated with the file/pattern."))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &FileTagData::m_comment,
                            QT_TRANSLATE_NOOP("AzFramework", "Comment"),
                            QT_TRANSLATE_NOOP("AzFramework", "Comment for the file tag definition"));
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
                    editContext->Class<FileTagAsset>(
                        QT_TRANSLATE_NOOP("AzFramework", "Definition"),
                        QT_TRANSLATE_NOOP("AzFramework", "Asset storing all the file/pattern tagging information."))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &FileTagAsset::m_fileTagMap,
                            QT_TRANSLATE_NOOP("AzFramework", "File Tag Map"),
                            QT_TRANSLATE_NOOP("AzFramework", "Container for storing file tagging information."));
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
