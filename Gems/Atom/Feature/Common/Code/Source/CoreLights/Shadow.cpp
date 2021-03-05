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

#include <CoreLights/Shadow.h>

namespace AZ
{
    namespace Render
    {
        namespace Shadow
        {
            static Matrix4x4 s_clipToShadowmapTextureMatrix = Matrix4x4::CreateZero();

            const Matrix4x4& GetClipToShadowmapTextureMatrix()
            {
                if (AZ::IsClose(s_clipToShadowmapTextureMatrix.GetElement(0, 0), 0.0f, AZ::Constants::FloatEpsilon))
                {
                    static const float clipToTextureMatrixValues[] =
                    {
                        0.5f, 0.0f, 0.0f, 0.5f,  // X: [-1, 1] -> [0, 1]
                        0.0f,-0.5f, 0.0f, 0.5f,  // Y: [ 1,-1] -> [0, 1]
                        0.0f, 0.0f, 1.0f, 0.0f,  // Z: [ 0, 1] -> [0, 1]
                        0.0f, 0.0f, 0.0f, 1.0f
                    };
                    s_clipToShadowmapTextureMatrix = Matrix4x4::CreateFromRowMajorFloat16(clipToTextureMatrixValues);
                }
                return s_clipToShadowmapTextureMatrix;
            }

        } // namespace Shadow
    } // namespace Render
} // namespace AZ
