/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace EMotionFX
{
    class Actor;
    class SubMesh;
}

namespace EMStudio
{
    class SubMeshInfo final
    {
    public:
        AZ_RTTI(SubMeshInfo, "{D5A1FACF-8905-4A5C-86A0-2175CEB843F7}")
        AZ_CLASS_ALLOCATOR_DECL

        SubMeshInfo() {}
        SubMeshInfo(EMotionFX::Actor* actor, size_t lodLevel, EMotionFX::SubMesh* subMesh);
        ~SubMeshInfo() = default;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string   m_materialName;
        unsigned int    m_verticesCount;
        unsigned int    m_indicesCount;
        unsigned int    m_polygonsCount;
        size_t m_bonesCount;
        
    };
} // namespace EMStudio
