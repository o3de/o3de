/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <Framework/MultipartFormData.h>

namespace AWSCore
{
    namespace Detail
    {
        namespace
        {
            const char FIELD_HEADER_FMT[] = "--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n";
            const char FILE_HEADER_FMT[] = "--%s\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n\r\n";
            const char FOOTER_FMT[] = "--%s--\r\n";
            const char ENTRY_SEPARATOR[] = "\r\n";
        }
    }

    void MultipartFormData::AddField(AZStd::string name, AZStd::string value)
    {
        m_fields.emplace_back(Field{ std::move(name), std::move(value) });
    }

    void MultipartFormData::AddFile(AZStd::string fieldName, AZStd::string fileName, const char* path)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();

        AZ::IO::HandleType fileHandle;

        if (fileIO->Open(path, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle)) {
            m_fileFields.emplace_back(FileField{ std::move(fieldName), std::move(fileName) , AZStd::vector<char>{} });
            auto destFileBuffer = &m_fileFields.back().m_fileData;
            AZ::u64 size;
            if (fileIO->Size(path, size) && size > 0) {
                destFileBuffer->resize(size);
                fileIO->Read(fileHandle, &destFileBuffer->at(0), destFileBuffer->size());
            }
        }
        fileIO->Close(fileHandle);
    }

    void MultipartFormData::AddFileBytes(AZStd::string fieldName, AZStd::string fileName, const void* bytes, size_t length)
    {
        m_fileFields.emplace_back(FileField{ std::move(fieldName), std::move(fileName) , AZStd::vector<char>{} });
        m_fileFields.back().m_fileData.reserve(length);
        m_fileFields.back().m_fileData.assign(static_cast<const char*>(bytes), static_cast<const char*>(bytes) + length);
    }

    void MultipartFormData::SetCustomBoundary(AZStd::string boundary)
    {
        m_boundary = boundary;
    }

    void MultipartFormData::Prepare()
    {
        if (m_boundary.empty())
        {
            char buffer[33];
            AZ::Uuid::CreateRandom().ToString(buffer, sizeof(buffer), false, false);
            m_boundary = buffer;
        }
    }

    size_t MultipartFormData::EstimateBodySize() const
    {
        // Estimate the size of the final string as best we can to avoid unnecessary copies
        const size_t boundarySize = m_boundary.length();
        const size_t individualFieldBaseSize = boundarySize + sizeof(Detail::FIELD_HEADER_FMT) + sizeof(Detail::ENTRY_SEPARATOR);
        const size_t individualFileBaseSize = boundarySize + sizeof(Detail::FILE_HEADER_FMT) + sizeof(Detail::ENTRY_SEPARATOR);
        size_t estimatedSize = sizeof(Detail::FOOTER_FMT) + boundarySize;

        for (const auto& field : m_fields)
        {
            estimatedSize += individualFieldBaseSize + field.m_fieldName.length() + field.m_value.length();
        }

        for (const auto& fileField : m_fileFields)
        {
            estimatedSize += individualFileBaseSize + fileField.m_fieldName.length() + fileField.m_fileName.length() + fileField.m_fileData.size();
        }

        return estimatedSize;
    }

    MultipartFormData::ComposeResult MultipartFormData::ComposeForm()
    {
        ComposeResult result;
        Prepare();

        // Build the form body
        result.m_content.reserve(EstimateBodySize());

        for (const auto& field : m_fields)
        {
            result.m_content.append(AZStd::string::format(Detail::FIELD_HEADER_FMT, m_boundary.c_str(), field.m_fieldName.c_str()));
            result.m_content.append(field.m_value);
            result.m_content.append(Detail::ENTRY_SEPARATOR);
        }

        for (const auto& fileField : m_fileFields)
        {
            result.m_content.append(AZStd::string::format(Detail::FILE_HEADER_FMT, m_boundary.c_str(), fileField.m_fieldName.c_str(), fileField.m_fileName.c_str()));
            result.m_content.append(fileField.m_fileData.begin(), fileField.m_fileData.end());
            result.m_content.append(Detail::ENTRY_SEPARATOR);
        }

        result.m_content.append(AZStd::string::format(Detail::FOOTER_FMT, m_boundary.c_str()));

        // Populate the metadata
        result.m_contentLength = AZStd::string::format("%zu", result.m_content.length());
        result.m_contentType = AZStd::string::format("multipart/form-data; boundary=%s", m_boundary.c_str());

        return result;
    }


} // namespace AWSCore
