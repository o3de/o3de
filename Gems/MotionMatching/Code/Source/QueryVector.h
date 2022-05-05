/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/EMotionFXConfig.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        //! The input query vector for the motion matching search.
        //! It contains the feature values based on the feature schema for the current character pose.
        //! The number of values should be equal to the number of columns in the feature matrix.
        class EMFX_API QueryVector
            : public AZStd::vector<float>
        {
        public:
            AZ_RTTI(QueryVector, "{1C30F00B-7ACA-460D-96F4-A8C98436623B}")
            AZ_CLASS_ALLOCATOR_DECL

            virtual ~QueryVector() = default;

            void SetVector2(const AZ::Vector2& value, size_t offset);
            void SetVector3(const AZ::Vector3& value, size_t offset);

            AZ::Vector2 GetVector2(size_t offset) const;
            AZ::Vector3 GetVector3(size_t offset) const;
        };
    } // namespace MotionMatching
} // namespace EMotionFX
