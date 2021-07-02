#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <vector>
#include <unordered_map>
#include <AzCore/Math/Vector3.h>
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxSkinWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        struct TestFbxPolygon
        {
            size_t m_startVertexIndex;
            size_t m_vertexCount;

            TestFbxPolygon(size_t startVertexIndex, size_t vertexCount)
                : m_startVertexIndex(startVertexIndex)
                , m_vertexCount(vertexCount)
            {
            }
        };

        // TestFbxMesh
        //      FbxMesh Test Data creation
        class TestFbxMesh
            : public FbxMeshWrapper
        {
        public:
            TestFbxMesh();
            ~TestFbxMesh() override = default;

            int GetDeformerCount() const override;
            AZStd::shared_ptr<const FbxSkinWrapper> GetSkin(int index) const override;
            bool GetMaterialIndices(FbxLayerElementArrayTemplate<int>** lockableArray) const override;

            int GetControlPointsCount() const;
            AZStd::vector<Vector3> GetControlPoints() const override;

            int GetPolygonCount() const override;
            int GetPolygonSize(int polygonIndex) const override;
            int* GetPolygonVertices() const override;
            int GetPolygonVertexIndex(int polygonIndex) const;

            FbxUVWrapper GetElementUV(int index = 0) override;
            int GetElementUVCount() const override;
            FbxVertexColorWrapper GetElementVertexColor(int index = 0) override;
            int GetElementVertexColorCount() const override;

            bool GetPolygonVertexNormal(int polyIndex, int vertexIndex, Vector3& normal) const override;

            // Create test data APIs
            void CreateMesh(std::vector<AZ::Vector3>& points, std::vector<std::vector<int> >& polygonVertexIndices);
            void CreateExpectMeshInfo(std::vector<std::vector<int> >& expectedFaceVertexIndices);
            void SetSkin(const AZStd::shared_ptr<FbxSkinWrapper>& skin);

            size_t GetExpectedVetexCount() const;
            size_t GetExpectedFaceCount() const;
            AZ::Vector3 GetExpectedFaceVertexPosition(unsigned int faceIndex, unsigned int vertexIndex) const;

        protected:
            AZStd::vector<Vector3> m_vertexControlPoints; // vertex positions
            size_t m_vertexCount;
            int* m_polygonVertexIndices; // store all polygons' vertex indices in sequence. Each index maps to a control point.
            FbxLayerElementArrayTemplate<int>* m_materialIndices;
            std::unordered_map<int, TestFbxPolygon> m_polygonInfo;

            FbxUVWrapper m_uvElements;
            FbxVertexColorWrapper m_vertexColorElements;
            AZStd::shared_ptr<FbxSkinWrapper> m_skin;

            // Expected converted data
            unsigned int m_expectedVertexCount;
            std::vector<std::vector<int> > m_expectedFaceVertexIndices;
        };
    }
}
