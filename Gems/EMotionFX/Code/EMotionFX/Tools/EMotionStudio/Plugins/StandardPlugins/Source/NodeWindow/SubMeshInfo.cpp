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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Material.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/SubMeshInfo.h>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(SubMeshInfo, EMStudio::UIAllocator, 0)

    SubMeshInfo::SubMeshInfo(EMotionFX::Actor* actor, unsigned int lodLevel, EMotionFX::SubMesh* subMesh)
    {
        // In EMFX studio, we are not using the subMesh index - they all uses the default material.
        m_materialName = actor->GetMaterial(lodLevel, 0)->GetNameString();
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
            ->Field("materialName", &SubMeshInfo::m_materialName)
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
            ->DataElement(AZ::Edit::UIHandlers::Default, &SubMeshInfo::m_materialName, "Material", "")
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
