/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/FileIO.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/XML/rapidxml.h>

namespace Audio
{
    /*!
     * FindFilesInPath
     */
    static AZStd::vector<AZStd::string> FindFilesInPath(const AZStd::string_view folderPath, const char* filter)
    {
        AZStd::vector<AZStd::string> foundFiles;
        AZ::IO::FileIOBase::FindFilesCallbackType findFilesCallback = [&foundFiles](const char* file) -> bool
        {
            foundFiles.emplace_back(file);
            return true;
        };

        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO)
        {
            AZ::IO::Result result = fileIO->FindFiles(folderPath.data(), filter, findFilesCallback);
        }

        return foundFiles;
    }

    /*!
     * ScopedXmlLoader
     * Create this object on the stack and it will load an xml file contents to an internal buffer
     * and allow you to get the first Xml node.
     */
    class ScopedXmlLoader
    {
    public:
        ScopedXmlLoader(const AZStd::string_view filePath)
        {
            auto fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO != nullptr, "Audio::ScopedXmlLoader - FileIOBase instance is null!");

            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            AZ::IO::Result result = fileIO->Open(filePath.data(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeText, fileHandle);
            if (!result)
            {
                m_hasError = true;
            }

            if (!m_hasError)
            {
                result = fileIO->Size(fileHandle, m_fileSize);
                if (!result)
                {
                    m_hasError = true;
                }
            }

            if (!m_hasError)
            {
                m_fileBuffer.resize_no_construct(m_fileSize + 1);
                m_fileBuffer[m_fileSize] = '\0';
                result = fileIO->Read(fileHandle, m_fileBuffer.data(), m_fileSize);
                if (!result)
                {
                    m_hasError = true;
                }
            }

            if (!m_hasError)
            {
                if (!m_xmlDoc.parse<AZ::rapidxml::parse_no_data_nodes>(m_fileBuffer.data()))
                {
                    m_hasError = true;
                }
            }

            fileIO->Close(fileHandle);
        }

        ~ScopedXmlLoader()
        {
        }

        bool HasError() const
        {
            return m_hasError;
        }

        const AZ::rapidxml::xml_node<char>* GetRootNode() const
        {
            if (!HasError())
            {
                return m_xmlDoc.first_node();
            }

            return nullptr;
        }

    private:
        AZStd::vector<char> m_fileBuffer;
        AZ::rapidxml::xml_document<char> m_xmlDoc;
        AZ::u64 m_fileSize = 0;
        bool m_hasError = false;
    };

} // namespace Audio
