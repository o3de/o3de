/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Meshlets/Reflect/MeshletPackReader.h>
#include <cstring>

namespace AZ::Meshlets
{
    MeshletPackReader::MeshletPackReader() = default;

    bool MeshletPackReader::Parse(const AZ::u8* bytes, AZ::u64 sizeBytes)
    {
        m_sections.clear();
        m_sectionCount = 0;

        if (bytes == nullptr || sizeBytes < sizeof(FileHeader))
        {
            return false;
        }

        FileHeader header;
        std::memcpy(&header, bytes, sizeof(header));
        if (header.m_magic != PackMagic)
        {
            return false;
        }
        // v2/v3/v4 are strictly additive.
        if (header.m_version != PackVersion && header.m_version != PackVersionDag &&
            header.m_version != PackVersionPaged)
        {
            return false;
        }

        const AZ::u64 tocSize = static_cast<AZ::u64>(header.m_tocCount) * sizeof(SectionTocEntry);
        if (sizeof(FileHeader) + tocSize > sizeBytes)
        {
            return false;
        }

        const SectionTocEntry* toc = reinterpret_cast<const SectionTocEntry*>(bytes + sizeof(FileHeader));
        for (AZ::u32 i = 0; i < header.m_tocCount; ++i)
        {
            const SectionTocEntry& e = toc[i];
            // Bounds check: section range must lie inside the buffer.
            if (e.m_offset > sizeBytes || e.m_size > sizeBytes - e.m_offset)
            {
                return false;
            }
            SectionRange r;
            r.m_data = bytes + e.m_offset;
            r.m_size = e.m_size;
            m_sections[e.m_kind] = r;
        }

        m_sectionCount = header.m_tocCount;
        return true;
    }

    bool MeshletPackReader::HasSection(SectionKind kind) const
    {
        return m_sections.find(static_cast<AZ::u32>(kind)) != m_sections.end();
    }

    AZStd::span<const AZ::u8> MeshletPackReader::GetSection(SectionKind kind) const
    {
        auto it = m_sections.find(static_cast<AZ::u32>(kind));
        if (it == m_sections.end())
        {
            return {};
        }
        return AZStd::span<const AZ::u8>(it->second.m_data, static_cast<size_t>(it->second.m_size));
    }

} // namespace AZ::Meshlets
