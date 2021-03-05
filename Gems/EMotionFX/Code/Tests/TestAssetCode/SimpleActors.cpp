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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MeshBuilder.h>
#include <EMotionFX/Source/Node.h>
#include <MCore/Source/MemoryObject.h>
#include <Tests/TestAssetCode/SimpleActors.h>

namespace EMotionFX
{
    SimpleJointChainActor::SimpleJointChainActor(size_t jointCount, const char* name)
        : Actor(name)
    {
        if (jointCount)
        {
            AddNode(0, "rootJoint");
            GetBindPose()->SetLocalSpaceTransform(0, Transform::CreateIdentity());
        }

        for (uint32 i = 1; i < jointCount; ++i)
        {
            AddNode(i, ("joint" + AZStd::to_string(i)).c_str(), i - 1);

            Transform transform = Transform::CreateIdentity();
            transform.mPosition = AZ::Vector3(static_cast<float>(i), 0.0f, 0.0f);
            GetBindPose()->SetLocalSpaceTransform(i, transform);
        }
    }

    AllRootJointsActor::AllRootJointsActor(size_t jointCount, const char* name)
        : Actor(name)
    {
        for (uint32 i = 0; i < jointCount; ++i)
        {
            AddNode(i, ("rootJoint" + AZStd::to_string(i)).c_str());

            Transform transform = Transform::CreateIdentity();
            transform.mPosition = AZ::Vector3(static_cast<float>(i), 0.0f, 0.0f);
            GetBindPose()->SetLocalSpaceTransform(i, transform);
        }
    }

    PlaneActor::PlaneActor(const char* name)
        : SimpleJointChainActor(1, name)
    {
        SetMesh(0, 0, CreatePlane(0, {
            AZ::Vector3(-1.0f, -1.0f, 0.0f),
            AZ::Vector3(1.0f, -1.0f, 0.0f),
            AZ::Vector3(-1.0f,  1.0f, 0.0f),

            AZ::Vector3(1.0f, -1.0f, 0.0f),
            AZ::Vector3(-1.0f,  1.0f, 0.0f),
            AZ::Vector3(1.0f,  1.0f, 0.0f)
        }));
    }

    Mesh* PlaneActor::CreatePlane(uint32 nodeIndex, const AZStd::vector<AZ::Vector3>& points) const
    {
        const uint32 vertCount = static_cast<uint32>(points.size());
        const uint32 faceCount = vertCount / 3;

        AZStd::unique_ptr<EMotionFX::MeshBuilder> meshBuilder(aznew MeshBuilder(nodeIndex, vertCount));

        // Original vertex numbers
        MeshBuilderVertexAttributeLayerUInt32* orgVtxLayer = MeshBuilderVertexAttributeLayerUInt32::Create(
                vertCount,
                EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS,
                false,
                false
                );
        meshBuilder->AddLayer(orgVtxLayer);

        // The positions layer
        MeshBuilderVertexAttributeLayerVector3* posLayer = MeshBuilderVertexAttributeLayerVector3::Create(
                vertCount,
                EMotionFX::Mesh::ATTRIB_POSITIONS,
                false,
                true
                );
        meshBuilder->AddLayer(posLayer);

        // The normals layer
        MeshBuilderVertexAttributeLayerVector3* normalsLayer = MeshBuilderVertexAttributeLayerVector3::Create(
                vertCount,
                EMotionFX::Mesh::ATTRIB_NORMALS,
                false,
                true
                );
        meshBuilder->AddLayer(normalsLayer);

        const int materialId = 0;
        const AZ::Vector3 normalVector(0.0f, 0.0f, 1.0f);
        for (uint32 faceNum = 0; faceNum < faceCount; ++faceNum)
        {
            meshBuilder->BeginPolygon(materialId);
            for (uint32 vertexOfFace = 0; vertexOfFace < 3; ++vertexOfFace)
            {
                uint32 vertexNum = faceNum * 3 + vertexOfFace;
                orgVtxLayer->SetCurrentVertexValue(&vertexNum);
                posLayer->SetCurrentVertexValue(&points[vertexNum]);
                normalsLayer->SetCurrentVertexValue(&normalVector);

                meshBuilder->AddPolygonVertex(vertexNum);
            }
            meshBuilder->EndPolygon();
        }

        return Mesh::CreateFromMeshBuilder(meshBuilder.get());
    }

    PlaneActorWithJoints::PlaneActorWithJoints(size_t jointCount, const char* name)
        : PlaneActor(name)
    {
        for (uint32 i = 1; i < jointCount; ++i)
        {
            AddNode(i, ("joint" + AZStd::to_string(i)).c_str(), i - 1);

            Transform transform = Transform::CreateIdentity();
            transform.mPosition = AZ::Vector3(static_cast<float>(i), 0.0f, 0.0f);
            GetBindPose()->SetLocalSpaceTransform(i, transform);
        }
    }

} // namespace EMotionFX
