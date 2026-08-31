/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Meshlets/Reflect/MeshletPackAsset.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <cstring>

namespace AZ::Meshlets
{
    void MeshletPackAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MeshletPackAsset, AZ::Data::AssetData>()->Version(1);
        }
    }

    bool MeshletPackAsset::LoadFromBuffer(AZStd::vector<AZ::u8>&& bytes)
    {
        m_bytes = AZStd::move(bytes);
        if (!m_reader.Parse(m_bytes.data(), m_bytes.size()))
        {
            m_bytes.clear();
            return false;
        }
        return true;
    }

    const PackHeaderRecord* MeshletPackAsset::GetPackHeader() const
    {
        auto bytes = m_reader.GetSection(SectionKind::PackHeader);
        if (bytes.size() < sizeof(PackHeaderRecord))
        {
            return nullptr;
        }
        return reinterpret_cast<const PackHeaderRecord*>(bytes.data());
    }

} // namespace AZ::Meshlets
