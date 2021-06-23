/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
