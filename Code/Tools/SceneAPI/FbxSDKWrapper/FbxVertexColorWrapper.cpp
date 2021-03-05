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
#include <SceneAPI/FbxSDKWrapper/FbxVertexColorWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxLayerElementUtilities.h>
#include <SceneAPI/FbxSDKWrapper/FbxTypeConverter.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxColorWrapper::FbxColorWrapper(FbxColor& fbxColor)
            : m_fbxColor(fbxColor)
        {
        }

        float FbxColorWrapper::GetR() const
        {
            return aznumeric_caster(m_fbxColor.mRed);
        }

        float FbxColorWrapper::GetG() const
        {
            return aznumeric_caster(m_fbxColor.mGreen);
        }

        float FbxColorWrapper::GetB() const
        {
            return aznumeric_caster(m_fbxColor.mBlue);
        }

        float FbxColorWrapper::GetAlpha() const
        {
            return aznumeric_caster(m_fbxColor.mAlpha);
        }

        FbxVertexColorWrapper::FbxVertexColorWrapper(FbxGeometryElementVertexColor* fbxVertexColor)
            : m_fbxVertexColor(fbxVertexColor)
        {
            AZ_Assert(m_fbxVertexColor, "Invalid FbxGeometryElementVertexColor to initialie FbxVertexColorWrapper");
        }

        FbxVertexColorWrapper::~FbxVertexColorWrapper()
        {
            m_fbxVertexColor = nullptr;
        }

        const char* FbxVertexColorWrapper::GetName() const
        {
            if (m_fbxVertexColor)
            {
                return m_fbxVertexColor->GetName();
            }
            return nullptr;
        }

        FbxColorWrapper FbxVertexColorWrapper::GetElementAt(int polygonIndex, int polygonVertexIndex, int controlPointIndex) const
        {
            FbxColor color;
            FbxLayerElementUtilities::GetGeometryElement(color, m_fbxVertexColor, polygonIndex, polygonVertexIndex, controlPointIndex);
            return FbxColorWrapper(color);
        }

        bool FbxVertexColorWrapper::IsValid() const
        {
            return m_fbxVertexColor != nullptr;
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
