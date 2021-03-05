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

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace Physics
{
    class WorldBody;

    namespace ReflectionUtils
    {
        void ReflectPhysicsApi(AZ::ReflectContext* context);
    }

    namespace Utils
    {
        /// Reusable unordered set of string names.
        using NameSet = AZStd::unordered_set<AZStd::string>;

        /// Helper routine for certain physics engines that don't directly expose this property on rigid bodies.
        AZ_INLINE AZ::Matrix3x3 InverseInertiaLocalToWorld(const AZ::Vector3& diag, const AZ::Matrix3x3& rotationToWorld)
        {
            return rotationToWorld * AZ::Matrix3x3::CreateDiagonal(diag) * rotationToWorld.GetTranspose();
        }

        /// Makes the input string unique for the input set
        void MakeUniqueString(const AZStd::unordered_set<AZStd::string>& stringSet
            , AZStd::string& stringInOut
            , AZ::u64 maxStringLength);

        /// Defers the deletion of the body until after the next world update.
        /// The body is first removed from the world, and then deleted.
        /// This ensures trigger exit events are raised correctly on deleted
        /// objects.
        void DeferDelete(AZStd::unique_ptr<Physics::WorldBody> body);

        //! Returns true if the tag matches the filter tag, or the filter tag is empty
        bool FilterTag(AZ::Crc32 tag, AZ::Crc32 filter);
    }
}