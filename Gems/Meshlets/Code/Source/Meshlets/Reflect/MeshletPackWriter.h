/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>

namespace AZ::Meshlets
{
    //! Builds an in-memory .azmeshletpack byte buffer from sequentially-added
    //! sections. Sections appear in ToC-order, each at a 16-byte aligned offset.
    //! Used by both ingestion paths (SceneAPI export module + JSON sidecar).
    class MeshletPackWriter
    {
    public:
        MeshletPackWriter();

        //! Reset state and begin a new pack. Must be called before AddSection.
        void BeginPack();

        //! Pack format version written into the FileHeader. Defaults to PackVersion (2);
        //! DAG-enabled packs pass PackVersionDag (3). Reset to the default by BeginPack.
        void SetVersion(AZ::u32 version) { m_version = version; }

        //! Append a section to the pack. Data is copied into an internal staging buffer.
        //! Caller may reuse the source buffer immediately after this returns.
        void AddSection(SectionKind kind, const void* data, AZ::u64 size);

        //! Emit the assembled pack bytes into outBytes. Returns false on internal error
        //! (currently never; reserved for future structural validation).
        //! After End, the writer is in a finalized state -- call BeginPack again to reuse.
        bool End(AZStd::vector<AZ::u8>& outBytes);

    private:
        struct StagedSection
        {
            SectionKind m_kind;
            AZStd::vector<AZ::u8> m_data;
        };

        AZStd::vector<StagedSection> m_sections;
        AZ::u32 m_version = PackVersion;
        bool m_began = false;
    };

} // namespace AZ::Meshlets
