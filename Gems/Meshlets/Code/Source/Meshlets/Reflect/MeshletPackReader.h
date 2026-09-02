/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/span.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>

namespace AZ::Meshlets
{
    //! Validates and indexes a .azmeshletpack byte buffer in place. Does not own
    //! the buffer -- caller keeps it alive for the reader's lifetime.
    //!
    //! Forward compatibility: unknown SectionKind values from future sub-projects
    //! are exposed by HasSection / GetSection but should be ignored by SP1
    //! consumers. The reader rejects bad magic, unsupported version, or
    //! truncated/out-of-bounds section ranges.
    class MeshletPackReader
    {
    public:
        MeshletPackReader();

        //! Validate the buffer, populate the section index. Returns false on
        //! malformed input (caller should treat the pack as unloadable).
        bool Parse(const AZ::u8* bytes, AZ::u64 sizeBytes);

        AZ::u32 GetSectionCount() const { return m_sectionCount; }

        bool HasSection(SectionKind kind) const;

        //! Returns a span over the section's bytes. Returns an empty span if the
        //! section is absent.
        AZStd::span<const AZ::u8> GetSection(SectionKind kind) const;

    private:
        struct SectionRange
        {
            const AZ::u8* m_data = nullptr;
            AZ::u64 m_size = 0;
        };

        AZStd::unordered_map<AZ::u32 /*kind*/, SectionRange> m_sections;
        AZ::u32 m_sectionCount = 0;
    };

} // namespace AZ::Meshlets
