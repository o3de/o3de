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
        SubMeshInfo(EMotionFX::Actor* actor, unsigned int lodLevel, EMotionFX::SubMesh* subMesh);
        ~SubMeshInfo() = default;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string   m_materialName;
        unsigned int    m_verticesCount;
        unsigned int    m_indicesCount;
        unsigned int    m_polygonsCount;
        unsigned int    m_bonesCount;
        
    };
} // namespace EMStudio
