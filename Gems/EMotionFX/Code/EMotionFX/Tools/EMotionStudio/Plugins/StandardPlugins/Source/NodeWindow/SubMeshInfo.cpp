/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/SubMeshInfo.h>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(SubMeshInfo, EMStudio::UIAllocator)

    SubMeshInfo::SubMeshInfo([[maybe_unused]] EMotionFX::Actor* actor, [[maybe_unused]] size_t lodLevel, EMotionFX::SubMesh* subMesh)
    {
        m_verticesCount = subMesh->GetNumVertices();
        m_indicesCount = subMesh->GetNumIndices();
        m_polygonsCount = subMesh->GetNumPolygons();
        m_bonesCount = subMesh->GetNumBones();
    }

    void SubMeshInfo::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<SubMeshInfo>()
            ->Version(1)
            ->Field("verticesCount", &SubMeshInfo::m_verticesCount)
            ->Field("indicesCount", &SubMeshInfo::m_indicesCount)
            ->Field("polygonsCount", &SubMeshInfo::m_polygonsCount)
            ->Field("bonesCount", &SubMeshInfo::m_bonesCount)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<SubMeshInfo>("Submesh info", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &SubMeshInfo::m_verticesCount, "Vertices", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &SubMeshInfo::m_indicesCount, "Indices", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &SubMeshInfo::m_polygonsCount, "Polygons", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &SubMeshInfo::m_bonesCount, "Bones", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ;
    }

} // namespace EMStudio
