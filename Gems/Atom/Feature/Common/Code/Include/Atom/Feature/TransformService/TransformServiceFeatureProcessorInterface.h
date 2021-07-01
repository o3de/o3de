/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        //! This feature processor handles static and dynamic non-skinned meshes.
        class TransformServiceFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:

            AZ_RTTI(AZ::Render::TransformServiceFeatureProcessorInterface, "{A9099337-AA0F-4F47-8E47-6E7FBA8998D0}", AZ::RPI::FeatureProcessor);

            using ObjectId = RHI::Handle<uint32_t, TransformServiceFeatureProcessorInterface>;

            //! Reserves an object ID that can later be sent transform updates
            virtual ObjectId ReserveObjectId() = 0;
            //! Releases an object ID to be used by others. The passed in handle is invalidated.
            virtual void ReleaseObjectId(ObjectId& id) = 0;

            //! Sets the transform (and optionally non-uniform scale) for a given id. Id must be one reserved earlier.
            virtual void SetTransformForId(ObjectId id, const AZ::Transform& transform,
                const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne()) = 0;
            //! Gets the transform for a given id. Id must be one reserved earlier.
            virtual AZ::Transform GetTransformForId(ObjectId) const = 0;
            //! Gets the non-uniform scale for a given id. Id must be one reserved earlier.
            virtual AZ::Vector3 GetNonUniformScaleForId(ObjectId id) const = 0;
        };
    }
}
