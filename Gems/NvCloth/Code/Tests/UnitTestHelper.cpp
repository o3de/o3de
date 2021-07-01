/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <UnitTestHelper.h>

#include <AzCore/Interface/Interface.h>

#include <TriangleInputHelper.h>

#include <NvCloth/IFabricCooker.h>

namespace UnitTest
{
    void ExpectEq(const AZ::Vector3& azVec, const physx::PxVec3& pxVec, const float tolerance)
    {
        EXPECT_NEAR(azVec.GetX(), pxVec.x, tolerance);
        EXPECT_NEAR(azVec.GetY(), pxVec.y, tolerance);
        EXPECT_NEAR(azVec.GetZ(), pxVec.z, tolerance);
    }

    void ExpectEq(const AZ::Vector4& azVec, const physx::PxVec4& pxVec, const float tolerance)
    {
        EXPECT_NEAR(azVec.GetX(), pxVec.x, tolerance);
        EXPECT_NEAR(azVec.GetY(), pxVec.y, tolerance);
        EXPECT_NEAR(azVec.GetZ(), pxVec.z, tolerance);
        EXPECT_NEAR(azVec.GetW(), pxVec.w, tolerance);
    }

    void ExpectEq(const AZ::Quaternion& azQuat, const physx::PxQuat& pxQuat, const float tolerance)
    {
        EXPECT_NEAR(azQuat.GetX(), pxQuat.x, tolerance);
        EXPECT_NEAR(azQuat.GetY(), pxQuat.y, tolerance);
        EXPECT_NEAR(azQuat.GetZ(), pxQuat.z, tolerance);
        EXPECT_NEAR(azQuat.GetW(), pxQuat.w, tolerance);
    }

    void ExpectEq(const physx::PxVec4& pxVecA, const physx::PxVec4& pxVecB, const float tolerance)
    {
        EXPECT_NEAR(pxVecA.x, pxVecB.x, tolerance);
        EXPECT_NEAR(pxVecA.y, pxVecB.y, tolerance);
        EXPECT_NEAR(pxVecA.z, pxVecB.z, tolerance);
        EXPECT_NEAR(pxVecA.w, pxVecB.w, tolerance);
    }

    void ExpectEq(const NvCloth::FabricCookedData::InternalCookedData& azCookedData, const nv::cloth::CookedData& nvCookedData, const float tolerance)
    {
        EXPECT_EQ(azCookedData.m_numParticles, nvCookedData.mNumParticles);
        ExpectEq(azCookedData.m_phaseIndices, nvCookedData.mPhaseIndices);
        ExpectEq(azCookedData.m_phaseTypes, nvCookedData.mPhaseTypes);
        ExpectEq(azCookedData.m_sets, nvCookedData.mSets);
        ExpectEq(azCookedData.m_restValues, nvCookedData.mRestvalues, tolerance);
        ExpectEq(azCookedData.m_stiffnessValues, nvCookedData.mStiffnessValues, tolerance);
        ExpectEq(azCookedData.m_indices, nvCookedData.mIndices);
        ExpectEq(azCookedData.m_anchors, nvCookedData.mAnchors);
        ExpectEq(azCookedData.m_tetherLengths, nvCookedData.mTetherLengths, tolerance);
        ExpectEq(azCookedData.m_triangles, nvCookedData.mTriangles);
    }

    void ExpectEq(const NvCloth::FabricCookedData::InternalCookedData& azCookedDataA, const NvCloth::FabricCookedData::InternalCookedData& azCookedDataB, const float tolerance)
    {
        EXPECT_EQ(azCookedDataA.m_numParticles, azCookedDataB.m_numParticles);
        EXPECT_EQ(azCookedDataA.m_phaseIndices, azCookedDataB.m_phaseIndices);
        EXPECT_EQ(azCookedDataA.m_phaseTypes, azCookedDataB.m_phaseTypes);
        EXPECT_EQ(azCookedDataA.m_sets, azCookedDataB.m_sets);
        ExpectEq(azCookedDataA.m_restValues, azCookedDataB.m_restValues, tolerance);
        ExpectEq(azCookedDataA.m_stiffnessValues, azCookedDataB.m_stiffnessValues, tolerance);
        EXPECT_EQ(azCookedDataA.m_indices, azCookedDataB.m_indices);
        EXPECT_EQ(azCookedDataA.m_anchors, azCookedDataB.m_anchors);
        ExpectEq(azCookedDataA.m_tetherLengths, azCookedDataB.m_tetherLengths, tolerance);
        EXPECT_EQ(azCookedDataA.m_triangles, azCookedDataB.m_triangles);
    }

    void ExpectEq(const NvCloth::FabricCookedData& fabricCookedDataA, const NvCloth::FabricCookedData& fabricCookedDataB, const float tolerance)
    {
        EXPECT_EQ(fabricCookedDataA.m_id, fabricCookedDataB.m_id);
        EXPECT_THAT(fabricCookedDataA.m_particles, ::testing::Pointwise(ContainerIsCloseTolerance(tolerance), fabricCookedDataB.m_particles));
        EXPECT_EQ(fabricCookedDataA.m_indices, fabricCookedDataB.m_indices);
        EXPECT_THAT(fabricCookedDataA.m_gravity, IsCloseTolerance(fabricCookedDataB.m_gravity, tolerance));
        EXPECT_EQ(fabricCookedDataA.m_useGeodesicTether, fabricCookedDataB.m_useGeodesicTether);
        ExpectEq(fabricCookedDataA.m_internalData, fabricCookedDataB.m_internalData, tolerance);
    }

    void ExpectEq(const AZStd::vector<float>& azVectorA, const AZStd::vector<float> azVectorB, const float tolerance)
    {
        EXPECT_EQ(azVectorA.size(), azVectorB.size());
        if (azVectorA.size() == azVectorB.size())
        {
            for (size_t i = 0; i < azVectorA.size(); ++i)
            {
                EXPECT_NEAR(azVectorA[i], azVectorB[i], tolerance);
            }
        }
    }

    void ExpectEq(const AZStd::vector<AZ::s32>& azVector, const nv::cloth::Range<const AZ::s32>& nvRange)
    {
        EXPECT_EQ(azVector.size(), nvRange.size());
        if (azVector.size() == nvRange.size())
        {
            for (AZ::u32 i = 0; i < nvRange.size(); ++i)
            {
                EXPECT_EQ(azVector[i], nvRange[i]);
            }
        }
    }

    void ExpectEq(const AZStd::vector<AZ::u32>& azVector, const nv::cloth::Range<const AZ::u32>& nvRange)
    {
        EXPECT_EQ(azVector.size(), nvRange.size());
        if (azVector.size() == nvRange.size())
        {
            for (AZ::u32 i = 0; i < nvRange.size(); ++i)
            {
                EXPECT_EQ(azVector[i], nvRange[i]);
            }
        }
    }

    void ExpectEq(const AZStd::vector<float>& azVector, const nv::cloth::Range<const float>& nvRange, const float tolerance)
    {
        EXPECT_EQ(azVector.size(), nvRange.size());
        if (azVector.size() == nvRange.size())
        {
            for (AZ::u32 i = 0; i < nvRange.size(); ++i)
            {
                EXPECT_NEAR(azVector[i], nvRange[i], tolerance);
            }
        }
    }

    void ExpectEq(const AZStd::vector<AZ::Vector4>& azVector, const nv::cloth::Range<const physx::PxVec4>& nvRange, const float tolerance)
    {
        EXPECT_EQ(azVector.size(), nvRange.size());
        if (azVector.size() == nvRange.size())
        {
            for (AZ::u32 i = 0; i < nvRange.size(); ++i)
            {
                ExpectEq(azVector[i], nvRange[i], tolerance);
            }
        }
    }

    NvCloth::FabricCookedData CreateTestFabricCookedData()
    {
        const float width = 1.0f;
        const float height = 1.0f;
        const AZ::u32 segmentsX = 5;
        const AZ::u32 segmentsY = 5;

        const TriangleInput planeXY = CreatePlane(width, height, segmentsX, segmentsY);

        const AZStd::optional<const NvCloth::FabricCookedData> fabricCookedData =
            AZ::Interface<NvCloth::IFabricCooker>::Get()->CookFabric(
                planeXY.m_vertices, planeXY.m_indices);
        EXPECT_TRUE(fabricCookedData.has_value());

        return *fabricCookedData;
    }
} // namespace UnitTest
