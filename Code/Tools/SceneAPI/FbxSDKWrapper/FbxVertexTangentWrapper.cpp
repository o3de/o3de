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

#include <SceneAPI/FbxSDKWrapper/FbxVertexTangentWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxLayerElementUtilities.h>
#include <SceneAPI/FbxSDKWrapper/FbxTypeConverter.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {

        FbxVertexTangentWrapper::FbxVertexTangentWrapper(FbxGeometryElementTangent* fbxTangent)
            : m_fbxTangent(fbxTangent)
        {
            AZ_Assert(fbxTangent, "Invalid FbxGeometryElementTangent to initialize FbxVertexTangentWrapper")
        }


        FbxVertexTangentWrapper::~FbxVertexTangentWrapper()
        {
            m_fbxTangent = nullptr;
        }


        const char* FbxVertexTangentWrapper::GetName() const
        {
            if (m_fbxTangent)
            {
                return m_fbxTangent->GetName();
            }
            return "";
        }


        Vector3 FbxVertexTangentWrapper::GetElementAt(int polygonIndex, int polygonVertexIndex, int controlPointIndex) const
        {
            FbxVector4 tangent;
            FbxLayerElementUtilities::GetGeometryElement(tangent, m_fbxTangent, polygonIndex, polygonVertexIndex, controlPointIndex);
            return FbxTypeConverter::ToVector3(tangent);
        }


        bool FbxVertexTangentWrapper::IsValid() const
        {
            return m_fbxTangent != nullptr;
        }

    } // namespace FbxSDKWrapper
} // namespace AZ
