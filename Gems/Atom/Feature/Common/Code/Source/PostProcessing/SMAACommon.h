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

#include <AzCore/Math/Vector4.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>

namespace AZ
{
    namespace Render
    {
        // Path to texture resources used by SMAA.
        static const char* const PathToSMAAAreaTexture{ "textures/postprocessing/AreaTex.dds.streamingimage" };
        static const char* const PathToSMAASearchTexture{ "textures/postprocessing/SearchTex.dds.streamingimage" };

        // Option shader variable names for SMAA shader.
        static const char* const EnableDiagonalDetectionFeatureOptionName{ "o_enableDiagonalDetectionFeature" };
        static const char* const EnableCornerDetectionFeatureOptionName{ "o_enableCornerDetectionFeature" };
        static const char* const EnablePredicationFeatureOptionName{ "o_enablePredicationFeature" };
        static const char* const EdgeDetectionModeOptionName{ "o_edgeDetectionMode" };
        static const char* const BlendingOutputModeOptionName{ "o_blendingOutputMode" };
    } // namespace Render
} // namespace AZ
