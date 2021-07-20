/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SurfaceData_precompiled.h"
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>

namespace SurfaceData
{
    bool GetMeshRayIntersection(
        const AZ::RPI::ModelAsset& meshAsset, const AZ::Transform& meshTransform,
        const AZ::Transform& meshTransformInverse, const AZ::Vector3& nonUniformScale,
        const AZ::Vector3& rayStart, const AZ::Vector3& rayEnd, 
        AZ::Vector3& outPosition, AZ::Vector3& outNormal)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        const AZ::Vector3 clampedScale = nonUniformScale.GetMax(AZ::Vector3(AZ::MinTransformScale));

        // Transform everything into model space
        const AZ::Vector3 rayStartLocal = meshTransformInverse.TransformPoint(rayStart) / clampedScale;
        const AZ::Vector3 rayEndLocal = meshTransformInverse.TransformPoint(rayEnd) / clampedScale;
        const AZ::Vector3 rayDirectionLocal = (rayEndLocal - rayStartLocal).GetNormalized();
        float distance = rayEndLocal.GetDistance(rayStartLocal);

        AZ::Vector3 normalLocal;

        constexpr bool AllowBruteForce = true;
        if (meshAsset.LocalRayIntersectionAgainstModel(rayStartLocal, rayDirectionLocal, AllowBruteForce, distance, normalLocal))
        {
            // Transform everything back to world space
            outPosition = meshTransform.TransformPoint((rayStartLocal + (rayDirectionLocal * distance)) * clampedScale);
            outNormal = meshTransform.TransformVector(normalLocal * clampedScale);
            return true;
        }

        return false;
    }

}
