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

#include <fbxsdk.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/FbxSDKWrapper/FbxVertexColorWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxUVWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxSkinWrapper;
        class FbxBlendShapeWrapper;
        class FbxVertexTangentWrapper;
        class FbxVertexBitangentWrapper;

        class FbxMeshWrapper
        {
        public:
            FbxMeshWrapper(FbxMesh* fbxMesh);
            virtual ~FbxMeshWrapper();

            virtual const char* GetName() const;

            virtual int GetDeformerCount() const;
            virtual int GetDeformerCount(int type) const;
            virtual AZStd::shared_ptr<const FbxSkinWrapper> GetSkin(int index) const;
            virtual AZStd::shared_ptr<const FbxBlendShapeWrapper> GetBlendShape(int index) const;
            virtual bool GetMaterialIndices(FbxLayerElementArrayTemplate<int>** lockableArray) const;

            // Vertex positions access
            // Get the control points number
            virtual int GetControlPointsCount() const;

            // Get the array of control points
            virtual AZStd::vector<Vector3> GetControlPoints() const;

            // Polygon data accesses
            // Get the polygon count of this mesh
            virtual int GetPolygonCount() const;

            // Get the number of polygon vertices in a polygon
            virtual int GetPolygonSize(int polygonIndex) const;

            // Get the array of polygon vertices (i.e: indices to the control points)
            virtual int* GetPolygonVertices() const;

            // Gets the start index into the array returned by GetPolygonVertices() for the given polygon
            virtual int GetPolygonVertexIndex(int polygonIndex) const;

            // Vertex colors and UV textures accesses
            // Returns this geometry's UV element
            virtual FbxUVWrapper GetElementUV(int index = 0);
            
            virtual int GetElementUVCount() const;

            virtual int GetElementTangentCount() const;
            virtual int GetElementBitangentCount() const;
            virtual FbxVertexTangentWrapper GetElementTangent(int index = 0);
            virtual FbxVertexBitangentWrapper GetElementBitangent(int index = 0);
            
            // Returns this geometry's vertex color element
            virtual FbxVertexColorWrapper GetElementVertexColor(int index = 0);

            virtual int GetElementVertexColorCount() const;

            // Get the normal associated with the specified polygon vertex
            virtual bool GetPolygonVertexNormal(int polyIndex, int vertexIndex, Vector3& normal) const;

        protected:
            FbxMeshWrapper() = default;
            FbxMesh* m_fbxMesh;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ