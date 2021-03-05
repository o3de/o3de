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

#include <SceneAPI/FbxSDKWrapper/FbxUVWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxLayerElementUtilities.h>
#include <SceneAPI/FbxSDKWrapper/FbxTypeConverter.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxUVWrapper::FbxUVWrapper(FbxGeometryElementUV* fbxUV)
            : m_fbxUV(fbxUV)
        {
            AZ_Assert(fbxUV, "Invalid FbxGeometryElementUV to initialize FbxUVWrapper")
        }

        FbxUVWrapper::~FbxUVWrapper()
        {
            m_fbxUV = nullptr;
        }

        const char* FbxUVWrapper::GetName() const
        {
            if (m_fbxUV)
            {
                return m_fbxUV->GetName();
            }
            return nullptr;
        }

        Vector2 FbxUVWrapper::GetElementAt(int polygonIndex, int polygonVertexIndex, int controlPointIndex) const
        {
            FbxVector2 uv;
            FbxLayerElementUtilities::GetGeometryElement(uv, m_fbxUV, polygonIndex, polygonVertexIndex, controlPointIndex);
            return FbxTypeConverter::ToVector2(uv);
        }

        bool FbxUVWrapper::IsValid() const
        {
            return m_fbxUV != nullptr;
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
