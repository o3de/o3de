/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SurfaceDataProfiler.h>
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
        SURFACE_DATA_PROFILE_FUNCTION_VERBOSE

        const AZ::Vector3 clampedScale = nonUniformScale.GetMax(AZ::Vector3(AZ::MinTransformScale));

        // Transform everything into model space
        const AZ::Vector3 rayStartLocal = meshTransformInverse.TransformPoint(rayStart) / clampedScale;
        const AZ::Vector3 rayEndLocal = meshTransformInverse.TransformPoint(rayEnd) / clampedScale;

        // LocalRayIntersectionAgainstModel expects the direction to contain both the direction and the distance of the raycast,
        // so it's important not to normalize this value.
        const AZ::Vector3 rayDirectionLocal = (rayEndLocal - rayStartLocal);

        float normalizedDistance = 0.0f;

        AZ::Vector3 normalLocal;

        constexpr bool AllowBruteForce = true;
        if (meshAsset.LocalRayIntersectionAgainstModel(rayStartLocal, rayDirectionLocal, AllowBruteForce, normalizedDistance, normalLocal))
        {
            // Transform everything back to world space
            // The distance that's returned is normalized from 0-1, so multiplying with the rayDirectionLocal gives the
            // actual collision point.
            outPosition = meshTransform.TransformPoint((rayStartLocal + (rayDirectionLocal * normalizedDistance)) * clampedScale);
            outNormal = meshTransform.TransformVector(normalLocal).GetNormalized();
            return true;
        }

        return false;
    }

}
