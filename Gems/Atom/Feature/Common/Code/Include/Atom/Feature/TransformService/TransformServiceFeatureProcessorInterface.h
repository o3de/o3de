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

#include <AzCore/Math/Transform.h>
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

            //! Sets the Matrix3x4 for a given id. Id must be one reserved earlier.
            virtual void SetMatrix3x4ForId(ObjectId id, const AZ::Matrix3x4& transform) = 0;
            //! Gets the Matrix3x4 for a given id. Id must be one reserved earlier.
            virtual AZ::Matrix3x4 GetMatrix3x4ForId(ObjectId) const = 0;

        };
    }
}
