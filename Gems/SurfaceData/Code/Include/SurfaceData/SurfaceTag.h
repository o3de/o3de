/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Component/Component.h>
#include <SurfaceData/SurfaceDataConstants.h>

namespace SurfaceData
{
    /**
    * Represents a tag value to match with surface materials and/or masks
    */
    class SurfaceTag final
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceTag, AZ::SystemAllocator, 0);
        AZ_RTTI(SurfaceTag, "{67C8C6ED-F32A-443E-A777-1CAE48B22CD7}");
        static void Reflect(AZ::ReflectContext* context);

        SurfaceTag()
            : m_surfaceTagCrc(Constants::s_unassignedTagCrc)
        {
        }

        SurfaceTag(const AZStd::string& value)
            : m_surfaceTagCrc(AZ::Crc32(value.data()))
        {
        }

        SurfaceTag(const AZ::Crc32& value)
            : m_surfaceTagCrc(value)
        {
        }

        AZ_INLINE bool operator==(const SurfaceTag& other) const;
        AZ_INLINE bool operator<(const SurfaceTag& other) const;
        AZ_INLINE operator AZ::Crc32() const;
        AZ_INLINE operator AZ::u32() const;

        void SetTag(const AZStd::string& value)
        {
            m_surfaceTagCrc = AZ::Crc32(value.data());
        }

        static AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> GetRegisteredTags();

    private:
        bool FindDisplayName(const AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>>& selectableTags, AZStd::string& name) const;

        AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> BuildSelectableTagList() const;

        AZStd::string GetDisplayName() const;

        AZ::u32 m_surfaceTagCrc;
    };

    AZ_INLINE bool SurfaceTag::operator==(const SurfaceTag& other) const
    {
        return other.m_surfaceTagCrc == m_surfaceTagCrc;
    }

    AZ_INLINE bool SurfaceTag::operator<(const SurfaceTag& other) const
    {
        return other.m_surfaceTagCrc < m_surfaceTagCrc;
    }

    AZ_INLINE SurfaceTag::operator AZ::Crc32() const
    {
        return m_surfaceTagCrc;
    }

    AZ_INLINE SurfaceTag::operator AZ::u32() const
    {
        return static_cast<AZ::u32>(m_surfaceTagCrc);
    }
}