/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Meshlets/Reflect/MeshletPackWriter.h>
#include <cstring>

namespace AZ::Meshlets
{
    namespace
    {
        AZ::u64 AlignUp(AZ::u64 value, AZ::u64 alignment)
        {
            return (value + alignment - 1) & ~(alignment - 1);
        }
    }

    MeshletPackWriter::MeshletPackWriter() = default;

    void MeshletPackWriter::BeginPack()
    {
        m_sections.clear();
        m_version = PackVersion;
        m_began = true;
    }

    void MeshletPackWriter::AddSection(SectionKind kind, const void* data, AZ::u64 size)
    {
        AZ_Assert(m_began, "MeshletPackWriter::AddSection called without BeginPack");
        StagedSection s;
        s.m_kind = kind;
        s.m_data.resize(static_cast<size_t>(size));
        if (size > 0 && data != nullptr)
        {
            std::memcpy(s.m_data.data(), data, static_cast<size_t>(size));
        }
        m_sections.push_back(AZStd::move(s));
    }

    bool MeshletPackWriter::End(AZStd::vector<AZ::u8>& outBytes)
    {
        if (!m_began)
        {
            return false;
        }
        m_began = false;

        const AZ::u32 tocCount = static_cast<AZ::u32>(m_sections.size());
        const AZ::u64 headerSize = sizeof(FileHeader);
        const AZ::u64 tocSize = sizeof(SectionTocEntry) * tocCount;

        // Compute section offsets (16-byte aligned, starting after header + ToC).
        AZStd::vector<AZ::u64> sectionOffsets(tocCount);
        AZ::u64 cursor = AlignUp(headerSize + tocSize, SectionAlignment);
        for (AZ::u32 i = 0; i < tocCount; ++i)
        {
            sectionOffsets[i] = cursor;
            cursor = AlignUp(cursor + m_sections[i].m_data.size(), SectionAlignment);
        }

        const AZ::u64 totalSize = cursor;
        outBytes.assign(static_cast<size_t>(totalSize), 0);

        // Write header.
        FileHeader header{};
        header.m_magic = PackMagic;
        header.m_version = m_version;
        header.m_tocCount = static_cast<AZ::u16>(tocCount);
        header.m_flags = 0;
        header.m_reserved = 0;
        std::memcpy(outBytes.data(), &header, sizeof(header));

        // Write ToC.
        SectionTocEntry* tocPtr = reinterpret_cast<SectionTocEntry*>(outBytes.data() + headerSize);
        for (AZ::u32 i = 0; i < tocCount; ++i)
        {
            SectionTocEntry e{};
            e.m_kind = static_cast<AZ::u32>(m_sections[i].m_kind);
            e.m_flags = 0;
            e.m_offset = sectionOffsets[i];
            e.m_size = m_sections[i].m_data.size();
            e.m_reserved = 0;
            tocPtr[i] = e;
        }

        // Write section data.
        for (AZ::u32 i = 0; i < tocCount; ++i)
        {
            if (!m_sections[i].m_data.empty())
            {
                std::memcpy(outBytes.data() + sectionOffsets[i],
                            m_sections[i].m_data.data(),
                            m_sections[i].m_data.size());
            }
        }

        m_sections.clear();
        return true;
    }

} // namespace AZ::Meshlets
