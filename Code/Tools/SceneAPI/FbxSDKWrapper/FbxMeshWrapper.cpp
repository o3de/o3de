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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxSkinWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxVertexTangentWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxVertexBitangentWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxBlendShapeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxTypeConverter.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxMeshWrapper::FbxMeshWrapper(FbxMesh* fbxMesh)
            : m_fbxMesh(fbxMesh)
        {
            AZ_Assert(fbxMesh, "Invalid FbxMesh input to initialize FbxMeshWrapper");
        }

        FbxMeshWrapper::~FbxMeshWrapper()
        {
            m_fbxMesh = nullptr;
        }

        const char* FbxMeshWrapper::GetName() const
        {
            return m_fbxMesh->GetName();
        }

        int FbxMeshWrapper::GetDeformerCount() const
        {
            return m_fbxMesh->GetDeformerCount();
        }

        int FbxMeshWrapper::GetDeformerCount(int type) const
        {
            return m_fbxMesh->GetDeformerCount(static_cast<FbxDeformer::EDeformerType>(type));
        }

        int FbxMeshWrapper::GetControlPointsCount() const
        {
            return m_fbxMesh->GetControlPointsCount();
        }

        AZStd::vector<Vector3> FbxMeshWrapper::GetControlPoints() const
        {
            FbxVector4* controlPoints = m_fbxMesh->GetControlPoints();
            AZStd::vector<Vector3> azControlPoints;
            azControlPoints.reserve(aznumeric_caster(GetControlPointsCount()));
            for (int i = 0; i < GetControlPointsCount(); ++i)
            {
                azControlPoints.push_back(FbxTypeConverter::ToVector3(controlPoints[i]));
            }
            return azControlPoints;
        }

        AZStd::shared_ptr<const FbxSkinWrapper> FbxMeshWrapper::GetSkin(int index) const
        {
            FbxSkin* skin = static_cast<FbxSkin*>(m_fbxMesh->GetDeformer(index, FbxDeformer::eSkin));
            return skin ? AZStd::make_shared<const FbxSkinWrapper>(skin) : nullptr;
        }

        AZStd::shared_ptr<const FbxBlendShapeWrapper> FbxMeshWrapper::GetBlendShape(int index) const
        {
            int deformerCount = m_fbxMesh->GetDeformerCount();
            int blendshapeCount = m_fbxMesh->GetDeformerCount(FbxDeformer::eBlendShape);
            FbxBlendShape* blendShape = static_cast<FbxBlendShape*>(m_fbxMesh->GetDeformer(index, FbxDeformer::eBlendShape));
            return blendShape ? AZStd::make_shared<const FbxBlendShapeWrapper>(blendShape) : nullptr;
        }

        int FbxMeshWrapper::GetPolygonCount() const
        {
            return m_fbxMesh->GetPolygonCount();
        }

        int FbxMeshWrapper::GetPolygonSize(int polygonIndex) const
        {
            return m_fbxMesh->GetPolygonSize(polygonIndex);
        }

        int* FbxMeshWrapper::GetPolygonVertices() const
        {
            return m_fbxMesh->GetPolygonVertices();
        }

        int FbxMeshWrapper::GetPolygonVertexIndex(int polygonIndex) const
        {
            return m_fbxMesh->GetPolygonVertexIndex(polygonIndex);
        }

        bool FbxMeshWrapper::GetMaterialIndices(FbxLayerElementArrayTemplate<int>** lockableArray) const
        {
            return m_fbxMesh->GetMaterialIndices(lockableArray);
        }

        FbxUVWrapper FbxMeshWrapper::GetElementUV(int index)
        {
            return FbxUVWrapper(m_fbxMesh->GetElementUV(index));
        }

        FbxVertexTangentWrapper FbxMeshWrapper::GetElementTangent(int index)
        {
            return FbxVertexTangentWrapper(m_fbxMesh->GetElementTangent(index));
        }

        FbxVertexBitangentWrapper FbxMeshWrapper::GetElementBitangent(int index)
        {
            return FbxVertexBitangentWrapper(m_fbxMesh->GetElementBinormal(index));
        }

        int FbxMeshWrapper::GetElementUVCount() const
        {
            return m_fbxMesh->GetElementUVCount();
        }

        int FbxMeshWrapper::GetElementTangentCount() const
        {
            return m_fbxMesh->GetElementTangentCount();
        }

        int FbxMeshWrapper::GetElementBitangentCount() const
        {
            return m_fbxMesh->GetElementBinormalCount();
        }

        FbxVertexColorWrapper FbxMeshWrapper::GetElementVertexColor(int index)
        {
            return FbxVertexColorWrapper(m_fbxMesh->GetElementVertexColor(index));
        }

        int FbxMeshWrapper::GetElementVertexColorCount() const
        {
            return m_fbxMesh->GetElementVertexColorCount();
        }

        bool FbxMeshWrapper::GetPolygonVertexNormal(int polyIndex, int vertexIndex, Vector3& normal) const
        {
            FbxVector4 fbxNormal;
            bool hasVertexNormal = m_fbxMesh->GetPolygonVertexNormal(polyIndex, vertexIndex, fbxNormal);
            normal = FbxTypeConverter::ToVector3(fbxNormal);
            return hasVertexNormal;
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
