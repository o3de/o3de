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

#include <SceneAPI/FbxSDKWrapper/FbxVertexBitangentWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxLayerElementUtilities.h>
#include <SceneAPI/FbxSDKWrapper/FbxTypeConverter.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxVertexBitangentWrapper::FbxVertexBitangentWrapper(FbxGeometryElementBinormal* fbxBitangent)
            : m_fbxBitangent(fbxBitangent)
        {
            AZ_Assert(fbxBitangent, "Invalid FbxGeometryElementBinormal to initialize FbxVertexBitangentWrapper")
        }


        FbxVertexBitangentWrapper::~FbxVertexBitangentWrapper()
        {
            m_fbxBitangent = nullptr;
        }


        const char* FbxVertexBitangentWrapper::GetName() const
        {
            if (m_fbxBitangent)
            {
                return m_fbxBitangent->GetName();
            }
            return "";
        }


        Vector3 FbxVertexBitangentWrapper::GetElementAt(int polygonIndex, int polygonVertexIndex, int controlPointIndex) const
        {
            FbxVector4 bitangent;
            FbxLayerElementUtilities::GetGeometryElement(bitangent, m_fbxBitangent, polygonIndex, polygonVertexIndex, controlPointIndex);
            return FbxTypeConverter::ToVector3(bitangent);
        }


        bool FbxVertexBitangentWrapper::IsValid() const
        {
            return m_fbxBitangent != nullptr;
        }

    } // namespace FbxSDKWrapper
} // namespace AZ
