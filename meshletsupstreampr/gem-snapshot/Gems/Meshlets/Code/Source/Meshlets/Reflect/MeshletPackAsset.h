/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>
#include <Meshlets/Reflect/MeshletPackReader.h>

namespace AZ::Meshlets
{
    //! Runtime asset wrapping a parsed .azmeshletpack file. Owns the byte
    //! buffer; exposes section views via the embedded reader.
    class MeshletPackAsset : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(MeshletPackAsset, "{2D7E4B91-8C3F-41A6-9D5B-7E2C1F4A8B30}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(MeshletPackAsset, AZ::SystemAllocator);

        static constexpr const char* DisplayName = "Meshlet Pack";
        static constexpr const char* Group       = "Meshlets";
        static constexpr const char* Extension   = "azmeshletpack";

        static void Reflect(AZ::ReflectContext* context);

        //! Take ownership of the byte buffer and parse it. Returns false on
        //! malformed input.
        bool LoadFromBuffer(AZStd::vector<AZ::u8>&& bytes);

        const MeshletPackReader& GetReader() const { return m_reader; }

        //! Convenience: read the PackHeader record. Returns nullptr if absent
        //! (which would indicate a malformed pack — Parse should have rejected).
        const PackHeaderRecord* GetPackHeader() const;

    private:
        AZStd::vector<AZ::u8> m_bytes;  //!< Owns the in-memory pack.
        MeshletPackReader m_reader;
    };

    // ! Dummy class to anchor the .lib for the Gem.
    class AZ_DLL_EXPORT MeshletsReflectClass
    {
        MeshletsReflectClass() = default;
        virtual ~MeshletsReflectClass() = default;
    };
} // namespace AZ::Meshlets
