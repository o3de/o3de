/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <SceneAPI/FbxSceneBuilder/Tests/TestFbxMesh.h>
#include <fbxsdk.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        TestFbxMesh::TestFbxMesh()
            : m_vertexControlPoints(nullptr)
            , m_vertexCount(0)
            , m_polygonVertexIndices(nullptr)
            , m_materialIndices(new FbxLayerElementArrayTemplate<int>(eFbxInt))
            , m_uvElements(FbxGeometryElementUV::Create(nullptr, "TestElements_UV"))
            , m_vertexColorElements(FbxGeometryElementVertexColor::Create(nullptr, "TestElements_VertexColors"))
            , m_expectedVertexCount(0)
        {
        }

        int TestFbxMesh::GetDeformerCount() const
        {
            // For current test need, only have one skin for the mesh
            return m_skin ? 1 : 0;
        }

        AZStd::shared_ptr<const FbxSkinWrapper> TestFbxMesh::GetSkin(int index) const
        {
            // For current test need, only have one skin for the mesh
            return m_skin;
        }

        bool TestFbxMesh::GetMaterialIndices(FbxLayerElementArrayTemplate<int>** lockableArray) const
        {
            *lockableArray = m_materialIndices;
            return true;
        }

        int TestFbxMesh::GetControlPointsCount() const
        {
            return static_cast<int>(m_vertexCount);
        }

        AZStd::vector<Vector3> TestFbxMesh::GetControlPoints() const
        {
            return m_vertexControlPoints;
        }

        int TestFbxMesh::GetPolygonCount() const
        {
            return static_cast<int>(m_polygonInfo.size());
        }

        int TestFbxMesh::GetPolygonSize(int polygonIndex) const
        {
            if (m_polygonInfo.find(polygonIndex) != m_polygonInfo.end())
            {
                return aznumeric_caster(m_polygonInfo.find(polygonIndex)->second.m_vertexCount);
            }
            return -1;
        }

        int* TestFbxMesh::GetPolygonVertices() const
        {
            return m_polygonVertexIndices;
        }

        int TestFbxMesh::GetPolygonVertexIndex(int polygonIndex) const
        {
            if (m_polygonInfo.find(polygonIndex) != m_polygonInfo.end())
            {
                return aznumeric_caster(m_polygonInfo.find(polygonIndex)->second.m_startVertexIndex);
            }
            return -1;
        }

        FbxUVWrapper TestFbxMesh::GetElementUV(int index)
        {
            (void)index;
            return m_uvElements;
        }

        int TestFbxMesh::GetElementUVCount() const
        {
            return 1;
        }

        FbxVertexColorWrapper TestFbxMesh::GetElementVertexColor(int index)
        {
            (void)index;
            return m_vertexColorElements;
        }

        int TestFbxMesh::GetElementVertexColorCount() const
        {
            return 1;
        }

        bool TestFbxMesh::GetPolygonVertexNormal(int polyIndex, int vertexIndex, Vector3& normal) const
        {
            normal = Vector3(1.0f, 0.0f, 0.0f);
            return true;
        }

        void TestFbxMesh::CreateMesh(std::vector<AZ::Vector3>& points, std::vector<std::vector<int> >& polygonVertexIndices)
        {
            m_vertexControlPoints.clear();
            m_materialIndices->Clear();
            m_polygonInfo.clear();

            // Create fbx control point (position) data, and associated material index data
            m_vertexCount = aznumeric_caster(points.size());
            m_vertexControlPoints.reserve(points.size());
            for (unsigned int i = 0; i < points.size(); ++i)
            {
                m_vertexControlPoints.push_back(Vector3(points[i].GetX(), points[i].GetY(), points[i].GetZ()));
                m_materialIndices->Add(i);
            }

            // Create fbx face data
            m_expectedVertexCount = 0;
            for (const std::vector<int>& onePolygonIndices : polygonVertexIndices)
            {
                for (const int& index : onePolygonIndices)
                {
                    m_expectedVertexCount++;
                }
            }
            if (m_polygonVertexIndices)
            {
                delete m_polygonVertexIndices;
            }
            m_polygonVertexIndices = new int[m_expectedVertexCount];
            size_t i = 0;
            for (const std::vector<int>& onePolygonIndices : polygonVertexIndices)
            {
                m_polygonInfo.insert(std::make_pair<int, TestFbxPolygon>(aznumeric_caster(m_polygonInfo.size()),
                        TestFbxPolygon(i, aznumeric_caster(onePolygonIndices.size()))));
                for (const int& index : onePolygonIndices)
                {
                    m_polygonVertexIndices[i] = index;
                    i++;
                }
            }
        }

        void TestFbxMesh::SetSkin(const AZStd::shared_ptr<FbxSkinWrapper>& skin)
        {
            m_skin = skin;
        }

        void TestFbxMesh::CreateExpectMeshInfo(std::vector<std::vector<int> >& expectedFaceVertexIndices)
        {
            m_expectedFaceVertexIndices = expectedFaceVertexIndices;
        }

        size_t TestFbxMesh::GetExpectedVetexCount() const
        {
            return m_expectedVertexCount;
        }

        size_t TestFbxMesh::GetExpectedFaceCount() const
        {
            return m_expectedFaceVertexIndices.size();
        }

        AZ::Vector3 TestFbxMesh::GetExpectedFaceVertexPosition(unsigned int faceIndex, unsigned int vertexIndex) const
        {
            return m_vertexControlPoints[m_expectedFaceVertexIndices[faceIndex][vertexIndex]];
        }
    }
}
