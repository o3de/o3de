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
#include <AzCore/Math/Vector2.h>
#include <SceneAPI/FbxSDKWrapper/FbxLayerElementUtilities.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxUVWrapper
        {
        public:
            FbxUVWrapper(FbxGeometryElementUV* fbxUV);
            ~FbxUVWrapper();

            Vector2 GetElementAt(int polygonIndex, int polygonVertexIndex, int controlPointIndex) const;
            const char* GetName() const;
            bool IsValid() const;

        protected:
            FbxGeometryElementUV* m_fbxUV;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
