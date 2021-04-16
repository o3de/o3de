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
#include "SurfaceData_precompiled.h"
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>

namespace SurfaceData
{
    bool GetMeshRayIntersection(
        const AZ::RPI::ModelAsset& meshAsset, const AZ::Transform& meshTransform, const AZ::Transform& meshTransformInverse,
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, AZ::Vector3& outPosition, AZ::Vector3& outNormal)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        // Transform everything into model space
        const AZ::Vector3 rayOriginLocal = meshTransformInverse.TransformPoint(rayOrigin);
        const AZ::Vector3 rayDirectionLocal = meshTransformInverse.TransformVector(rayDirection).GetNormalized();
        float distance = FLT_MAX;

        AZ::Vector3 normalLocal;

        if (meshAsset.LocalRayIntersectionAgainstModel(rayOriginLocal, rayDirectionLocal, distance, normalLocal))
        {
            // Transform everything back to world space
            outPosition = meshTransform.TransformPoint(rayOriginLocal + (rayDirectionLocal * distance));
            outNormal = meshTransform.TransformVector(normalLocal);
            return true;
        }

        return false;
    }

}
