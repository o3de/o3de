/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AZTestShared/Math/MathTestHelpers.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>

#include <NvCloth/Types.h>

// NvCloth library includes
#include <NvCloth/Range.h>
#include <foundation/PxVec3.h>
#include <foundation/PxVec4.h>
#include <foundation/PxQuat.h>
#include <NvClothExt/ClothFabricCooker.h>

namespace UnitTest
{
    static constexpr float Tolerance = 1e-4f;
    static constexpr float ToleranceU8 = 1.0f / 255.0f;

    void ExpectEq(const AZ::Vector3& azVec, const physx::PxVec3& pxVec, const float tolerance = Tolerance);

    void ExpectEq(const AZ::Vector4& azVec, const physx::PxVec4& pxVec, const float tolerance = Tolerance);

    void ExpectEq(const AZ::Quaternion& azQuat, const physx::PxQuat& pxQuat, const float tolerance = Tolerance);

    void ExpectEq(const physx::PxVec4& pxVecA, const physx::PxVec4& pxVecB, const float tolerance = Tolerance);

    void ExpectEq(const NvCloth::FabricCookedData::InternalCookedData& azCookedData, const nv::cloth::CookedData& nvCookedData, const float tolerance = Tolerance);

    void ExpectEq(const NvCloth::FabricCookedData::InternalCookedData& azCookedDataA, const NvCloth::FabricCookedData::InternalCookedData& azCookedDataB, const float tolerance = Tolerance);

    void ExpectEq(const NvCloth::FabricCookedData& fabricCookedDataA, const NvCloth::FabricCookedData& fabricCookedDataB, const float tolerance = Tolerance);

    void ExpectEq(const AZStd::vector<float>& azVecA, const AZStd::vector<float> azVecB, const float tolerance = Tolerance);

    void ExpectEq(const AZStd::vector<AZ::s32>& azVector, const nv::cloth::Range<const AZ::s32>& nvRange);

    void ExpectEq(const AZStd::vector<AZ::u32>& azVector, const nv::cloth::Range<const AZ::u32>& nvRange);

    void ExpectEq(const AZStd::vector<float>& azVector, const nv::cloth::Range<const float>& nvRange, const float tolerance = Tolerance);

    void ExpectEq(const AZStd::vector<AZ::Vector4>& azVector, const nv::cloth::Range<const physx::PxVec4>& nvRange, const float tolerance = Tolerance);

    NvCloth::FabricCookedData CreateTestFabricCookedData();
} // namespace UnitTest
