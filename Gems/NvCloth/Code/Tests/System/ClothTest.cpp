/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>

#include <UnitTestHelper.h>
#include <TriangleInputHelper.h>

#include <System/Factory.h>
#include <System/Fabric.h>
#include <System/FabricCooker.h>
#include <System/Cloth.h>
#include <System/NvTypes.h>

 // NvCloth library includes
#include <NvCloth/Cloth.h>

namespace NvCloth
{
    namespace Internal
    {
        physx::PxVec3& AsPxVec3(AZ::Vector3& azVec);
        const physx::PxVec3& AsPxVec3(const AZ::Vector3& azVec);
        physx::PxQuat& AsPxQuat(AZ::Quaternion& azQuat);
        const physx::PxQuat& AsPxQuat(const AZ::Quaternion& azQuat);
        void FastCopy(const AZStd::vector<AZ::Vector4>& azVector, nv::cloth::Range<physx::PxVec4>& nvRange);
        void FastCopy(const nv::cloth::Range<physx::PxVec4>& nvRange, AZStd::vector<AZ::Vector4>& azVector);
        void FastMove(AZStd::vector<AZ::Vector4>&& azVector, nv::cloth::Range<physx::PxVec4>& nvRange);
        void FastMove(nv::cloth::Range<physx::PxVec4>&& nvRange, AZStd::vector<AZ::Vector4>& azVector);
    } // namespace Internal
} // namespace NvCloth

namespace UnitTest
{
    TEST(NvClothSystem, Cloth_AzVector3AsPxVec3_PxVec3ElementsAreTheSameAsAzVector3)
    {
        AZ::Vector3 zero = AZ::Vector3::CreateZero();
        AZ::Vector3 one = AZ::Vector3::CreateOne();
        AZ::Vector3 axisX = AZ::Vector3::CreateAxisX();
        AZ::Vector3 axisY = AZ::Vector3::CreateAxisY();
        AZ::Vector3 axisZ = AZ::Vector3::CreateAxisZ();
        AZ::Vector3 vec3 = AZ::Vector3(26.0f, -462.366f, 15.384f);

        physx::PxVec3& pxZero  = NvCloth::Internal::AsPxVec3(zero);
        physx::PxVec3& pxOne = NvCloth::Internal::AsPxVec3(one);
        physx::PxVec3& pxAxisX = NvCloth::Internal::AsPxVec3(axisX);
        physx::PxVec3& pxAxisY = NvCloth::Internal::AsPxVec3(axisY);
        physx::PxVec3& pxAxisZ = NvCloth::Internal::AsPxVec3(axisZ);
        physx::PxVec3& pxVec3  = NvCloth::Internal::AsPxVec3(vec3);

        ExpectEq(zero,  pxZero);
        ExpectEq(one,   pxOne);
        ExpectEq(axisX, pxAxisX);
        ExpectEq(axisY, pxAxisY);
        ExpectEq(axisZ, pxAxisZ);
        ExpectEq(vec3, pxVec3);
    }

    TEST(NvClothSystem, Cloth_AzVector3AsPxVec3Const_PxVec3ElementsAreTheSameAsAzVector3)
    {
        const AZ::Vector3 zero = AZ::Vector3::CreateZero();
        const AZ::Vector3 one = AZ::Vector3::CreateOne();
        const AZ::Vector3 axisX = AZ::Vector3::CreateAxisX();
        const AZ::Vector3 axisY = AZ::Vector3::CreateAxisY();
        const AZ::Vector3 axisZ = AZ::Vector3::CreateAxisZ();
        const AZ::Vector3 vec3 = AZ::Vector3(26.0f, -462.366f, 15.384f);

        const physx::PxVec3& pxZero = NvCloth::Internal::AsPxVec3(zero);
        const physx::PxVec3& pxOne = NvCloth::Internal::AsPxVec3(one);
        const physx::PxVec3& pxAxisX = NvCloth::Internal::AsPxVec3(axisX);
        const physx::PxVec3& pxAxisY = NvCloth::Internal::AsPxVec3(axisY);
        const physx::PxVec3& pxAxisZ = NvCloth::Internal::AsPxVec3(axisZ);
        const physx::PxVec3& pxVec3 = NvCloth::Internal::AsPxVec3(vec3);

        ExpectEq(zero, pxZero);
        ExpectEq(one, pxOne);
        ExpectEq(axisX, pxAxisX);
        ExpectEq(axisY, pxAxisY);
        ExpectEq(axisZ, pxAxisZ);
        ExpectEq(vec3, pxVec3);
    }

    TEST(NvClothSystem, Cloth_AzQuaternionAsPxQuat_QuatElementsAreTheSameAsAzQuaternion)
    {
        AZ::Quaternion zero = AZ::Quaternion::CreateZero();
        AZ::Quaternion one = AZ::Quaternion::CreateIdentity();
        AZ::Quaternion rotX = AZ::Quaternion::CreateRotationX(AZ::DegToRad(26.5f));
        AZ::Quaternion rotY = AZ::Quaternion::CreateRotationY(AZ::DegToRad(-196.5f));
        AZ::Quaternion rotZ = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(263.2f));
        AZ::Quaternion quat = AZ::Quaternion(26.0f, -62.366f, 15.384f, 5.0f);

        physx::PxQuat& pxZero = NvCloth::Internal::AsPxQuat(zero);
        physx::PxQuat& pxOne = NvCloth::Internal::AsPxQuat(one);
        physx::PxQuat& pxRotX = NvCloth::Internal::AsPxQuat(rotX);
        physx::PxQuat& pxRotY = NvCloth::Internal::AsPxQuat(rotY);
        physx::PxQuat& pxRotZ = NvCloth::Internal::AsPxQuat(rotZ);
        physx::PxQuat& pxQuat = NvCloth::Internal::AsPxQuat(quat);

        ExpectEq(zero, pxZero);
        ExpectEq(one, pxOne);
        ExpectEq(rotX, pxRotX);
        ExpectEq(rotY, pxRotY);
        ExpectEq(rotZ, pxRotZ);
        ExpectEq(quat, pxQuat);
    }

    TEST(NvClothSystem, Cloth_AzQuaternionAsPxQuatConst_QuatElementsAreTheSameAsAzQuaternion)
    {
        const AZ::Quaternion zero = AZ::Quaternion::CreateZero();
        const AZ::Quaternion one = AZ::Quaternion::CreateIdentity();
        const AZ::Quaternion rotX = AZ::Quaternion::CreateRotationX(AZ::DegToRad(26.5f));
        const AZ::Quaternion rotY = AZ::Quaternion::CreateRotationY(AZ::DegToRad(-196.5f));
        const AZ::Quaternion rotZ = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(263.2f));
        const AZ::Quaternion quat = AZ::Quaternion(26.0f, -62.366f, 15.384f, 5.0f);

        const physx::PxQuat& pxZero = NvCloth::Internal::AsPxQuat(zero);
        const physx::PxQuat& pxOne = NvCloth::Internal::AsPxQuat(one);
        const physx::PxQuat& pxRotX = NvCloth::Internal::AsPxQuat(rotX);
        const physx::PxQuat& pxRotY = NvCloth::Internal::AsPxQuat(rotY);
        const physx::PxQuat& pxRotZ = NvCloth::Internal::AsPxQuat(rotZ);
        const physx::PxQuat& pxQuat = NvCloth::Internal::AsPxQuat(quat);

        ExpectEq(zero, pxZero);
        ExpectEq(one, pxOne);
        ExpectEq(rotX, pxRotX);
        ExpectEq(rotY, pxRotY);
        ExpectEq(rotZ, pxRotZ);
        ExpectEq(quat, pxQuat);
    }

    TEST(NvClothSystem, Cloth_FastCopy_nvRangeElmenentsAreTheSameAsAZStdVector)
    {
        const AZStd::vector<AZ::Vector4> azEmpty;
        const AZStd::vector<AZ::Vector4> azValues = {{
            AZ::Vector4(15.0f, -692.0f, 65.0f, -15.0f),
            AZ::Vector4(1851.594f, 1.0f, -125.0f, 168.0f),
            AZ::Vector4(2384.05f, -692.0f, 41865.153f, 1567.0f),
            AZ::Vector4(35.02f, 2572.453f, 2465.0f, 987.0f),
            AZ::Vector4(-14.161f, 47.0f, 65.0f, -6358.52f)
        }};

        nv::cloth::Vector<physx::PxVec4>::Type nvEmpty;
        nv::cloth::Vector<physx::PxVec4>::Type nvValues(static_cast<uint32_t>(azValues.size()));

        nv::cloth::Range<physx::PxVec4> nvEmptyRange(nvEmpty.begin(), nvEmpty.end());
        nv::cloth::Range<physx::PxVec4> nvValuesRange(nvValues.begin(), nvValues.end());

        NvCloth::Internal::FastCopy(azEmpty, nvEmptyRange);
        NvCloth::Internal::FastCopy(azValues, nvValuesRange);

        ExpectEq(azEmpty, nvEmptyRange);
        ExpectEq(azValues, nvValuesRange);
    }

    TEST(NvClothSystem, Cloth_FastCopy_AZStdVectorElmenentsAreTheSameAsNvRange)
    {
        nv::cloth::Vector<physx::PxVec4>::Type nvEmpty;
        nv::cloth::Vector<physx::PxVec4>::Type nvValues;
        nvValues.pushBack(physx::PxVec4(15.0f, -692.0f, 65.0f, -15.0f));
        nvValues.pushBack(physx::PxVec4(1851.594f, 1.0f, -125.0f, 168.0f));
        nvValues.pushBack(physx::PxVec4(2384.05f, -692.0f, 41865.153f, 1567.0f));
        nvValues.pushBack(physx::PxVec4(35.02f, 2572.453f, 2465.0f, 987.0f));
        nvValues.pushBack(physx::PxVec4(-14.161f, 47.0f, 65.0f, -6358.52f));

        const nv::cloth::Range<physx::PxVec4> nvEmptyRange(nvEmpty.begin(), nvEmpty.end());
        const nv::cloth::Range<physx::PxVec4> nvValuesRange(nvValues.begin(), nvValues.end());

        AZStd::vector<AZ::Vector4> azEmpty;
        AZStd::vector<AZ::Vector4> azValues(nvValuesRange.size());

        NvCloth::Internal::FastCopy(nvEmptyRange, azEmpty);
        NvCloth::Internal::FastCopy(nvValuesRange, azValues);

        ExpectEq(azEmpty, nvEmptyRange);
        ExpectEq(azValues, nvValuesRange);
    }

    TEST(NvClothSystem, Cloth_FastMove_nvRangeElmenentsAreTheSameAsAZStdVector)
    {
        const AZStd::vector<AZ::Vector4> azEmpty;
        const AZStd::vector<AZ::Vector4> azValues = {{
            AZ::Vector4(15.0f, -692.0f, 65.0f, -15.0f),
            AZ::Vector4(1851.594f, 1.0f, -125.0f, 168.0f),
            AZ::Vector4(2384.05f, -692.0f, 41865.153f, 1567.0f),
            AZ::Vector4(35.02f, 2572.453f, 2465.0f, 987.0f),
            AZ::Vector4(-14.161f, 47.0f, 65.0f, -6358.52f)
        }};

        nv::cloth::Vector<physx::PxVec4>::Type nvEmpty;
        nv::cloth::Vector<physx::PxVec4>::Type nvValues(static_cast<uint32_t>(azValues.size()));

        nv::cloth::Range<physx::PxVec4> nvEmptyRange(nvEmpty.begin(), nvEmpty.end());
        nv::cloth::Range<physx::PxVec4> nvValuesRange(nvValues.begin(), nvValues.end());

        {
            AZStd::vector<AZ::Vector4> azEmptyCopy = azEmpty;
            AZStd::vector<AZ::Vector4> azValuesCopy = azValues;

            NvCloth::Internal::FastMove(AZStd::move(azEmptyCopy), nvEmptyRange);
            NvCloth::Internal::FastMove(AZStd::move(azValuesCopy), nvValuesRange);
        }

        ExpectEq(azEmpty, nvEmptyRange);
        ExpectEq(azValues, nvValuesRange);
    }

    TEST(NvClothSystem, Cloth_FastMove_AZStdVectorElmenentsAreTheSameAsNvRange)
    {
        nv::cloth::Vector<physx::PxVec4>::Type nvEmpty;
        nv::cloth::Vector<physx::PxVec4>::Type nvValues;
        nvValues.pushBack(physx::PxVec4(15.0f, -692.0f, 65.0f, -15.0f));
        nvValues.pushBack(physx::PxVec4(1851.594f, 1.0f, -125.0f, 168.0f));
        nvValues.pushBack(physx::PxVec4(2384.05f, -692.0f, 41865.153f, 1567.0f));
        nvValues.pushBack(physx::PxVec4(35.02f, 2572.453f, 2465.0f, 987.0f));
        nvValues.pushBack(physx::PxVec4(-14.161f, 47.0f, 65.0f, -6358.52f));

        const nv::cloth::Range<physx::PxVec4> nvEmptyRange(nvEmpty.begin(), nvEmpty.end());
        const nv::cloth::Range<physx::PxVec4> nvValuesRange(nvValues.begin(), nvValues.end());

        AZStd::vector<AZ::Vector4> azEmpty;
        AZStd::vector<AZ::Vector4> azValues(nvValuesRange.size());

        {
            nv::cloth::Vector<physx::PxVec4>::Type nvEmptyCopy = nvEmpty;
            nv::cloth::Vector<physx::PxVec4>::Type nvValuesCopy = nvValues;

            nv::cloth::Range<physx::PxVec4> nvEmptyRangeCopy(nvEmptyCopy.begin(), nvEmptyCopy.end());
            nv::cloth::Range<physx::PxVec4> nvValuesRangeCopy(nvValuesCopy.begin(), nvValuesCopy.end());

            NvCloth::Internal::FastMove(AZStd::move(nvEmptyRangeCopy), azEmpty);
            NvCloth::Internal::FastMove(AZStd::move(nvValuesRangeCopy), azValues);
        }

        ExpectEq(azEmpty, nvEmptyRange);
        ExpectEq(azValues, nvValuesRange);
    }

    //! Sets up a cloth for each test case with access to its native cloth instance.
    //! Creating cloth using direct calls to the library, instead of using Factory, to
    //! be able to keep a pointer the native cloth instance.
    class NvClothSystemCloth
        : public ::testing::Test
    {
    protected:
        // ::testing::Test overrides ...
        void SetUp() override;
        void TearDown() override;

        AZStd::unique_ptr<NvCloth::Cloth> m_cloth;
        nv::cloth::Cloth* m_nvCloth = nullptr; // Pointer to native cloth instance of m_cloth

    private:
        void CreateFabric();
        void CreateCloth();

        NvCloth::NvFactoryUniquePtr m_nvFactory;
        AZStd::unique_ptr<NvCloth::Fabric> m_fabric;
    };

    void NvClothSystemCloth::SetUp()
    {
        m_nvFactory = NvCloth::NvFactoryUniquePtr(NvClothCreateFactoryCPU());
        CreateFabric();
        CreateCloth();
    }

    void NvClothSystemCloth::TearDown()
    {
        m_nvCloth = nullptr;
        m_cloth.reset();
        m_fabric.reset();
        m_nvFactory.reset();
    }

    void NvClothSystemCloth::CreateFabric()
    {
        const NvCloth::FabricCookedData fabricCookedData = CreateTestFabricCookedData();

        NvCloth::NvFabricUniquePtr nvFabric(
            m_nvFactory->createFabric(
                fabricCookedData.m_internalData.m_numParticles,
                NvCloth::ToNvRange(fabricCookedData.m_internalData.m_phaseIndices),
                NvCloth::ToNvRange(fabricCookedData.m_internalData.m_sets),
                NvCloth::ToNvRange(fabricCookedData.m_internalData.m_restValues),
                NvCloth::ToNvRange(fabricCookedData.m_internalData.m_stiffnessValues),
                NvCloth::ToNvRange(fabricCookedData.m_internalData.m_indices),
                NvCloth::ToNvRange(fabricCookedData.m_internalData.m_anchors),
                NvCloth::ToNvRange(fabricCookedData.m_internalData.m_tetherLengths),
                NvCloth::ToNvRange(fabricCookedData.m_internalData.m_triangles)));
        EXPECT_TRUE(nvFabric.get() != nullptr);

        m_fabric = AZStd::make_unique<NvCloth::Fabric>(
            fabricCookedData,
            AZStd::move(nvFabric));
    }

    void NvClothSystemCloth::CreateCloth()
    {
        NvCloth::NvClothUniquePtr nvCloth(
            m_nvFactory->createCloth(
                NvCloth::ToPxVec4NvRange(m_fabric->m_cookedData.m_particles),
                *m_fabric->m_nvFabric.get()));
        EXPECT_TRUE(nvCloth.get() != nullptr);

        m_nvCloth = nvCloth.get();

        m_cloth = AZStd::make_unique<NvCloth::Cloth>(
            NvCloth::ClothId(1),
            m_fabric->m_cookedData.m_particles,
            m_fabric.get(),
            AZStd::move(nvCloth));
    }

    TEST_F(NvClothSystemCloth, Cloth_SetParticles_ParticlesAreSetToClothAndNativeCloth)
    {
        auto newParticles = m_cloth->GetParticles();
        for (auto& particle : newParticles)
        {
            particle *= 2.0f;
        }

        m_cloth->SetParticles(newParticles);

        EXPECT_THAT(newParticles, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), m_cloth->GetParticles()));

        const nv::cloth::MappedRange<const physx::PxVec4> nvClothCurrentParticles = nv::cloth::readCurrentParticles(*m_nvCloth);
        ExpectEq(newParticles, nvClothCurrentParticles);

        // The inverse masses (W element) should have been copied into the previous particles inside NvCloth
        // to take effect for the next simulation update.
        const nv::cloth::MappedRange<const physx::PxVec4> nvClothPreviousParticles = nv::cloth::readPreviousParticles(*m_nvCloth);
        for (size_t i = 0; i < newParticles.size(); ++i)
        {
            EXPECT_NEAR(newParticles[i].GetW(), nvClothPreviousParticles[static_cast<uint32_t>(i)].w, Tolerance);
        }
    }

    TEST_F(NvClothSystemCloth, Cloth_SetParticlesMove_ParticlesAreSetToClothAndNativeCloth)
    {
        auto newParticles = m_cloth->GetParticles();
        for (auto& particle : newParticles)
        {
            particle *= 2.0f;
        }

        {
            auto newParticlesCopy = newParticles;

            m_cloth->SetParticles(AZStd::move(newParticlesCopy));
        }

        EXPECT_THAT(newParticles, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), m_cloth->GetParticles()));

        const nv::cloth::MappedRange<const physx::PxVec4> nvClothCurrentParticles = nv::cloth::readCurrentParticles(*m_nvCloth);
        ExpectEq(newParticles, nvClothCurrentParticles);

        // The inverse masses (W element) should have been copied into the previous particles inside NvCloth
        // to take effect for the next simulation update.
        const nv::cloth::MappedRange<const physx::PxVec4> nvClothPreviousParticles = nv::cloth::readPreviousParticles(*m_nvCloth);
        for (size_t i = 0; i < newParticles.size(); ++i)
        {
            EXPECT_NEAR(newParticles[i].GetW(), nvClothPreviousParticles[static_cast<uint32_t>(i)].w, Tolerance);
        }
    }

    TEST_F(NvClothSystemCloth, Cloth_DiscardParticleDelta_NativeClothPreviousAndCurrentParticlesAreTheSame)
    {
        m_cloth->DiscardParticleDelta();

        const nv::cloth::MappedRange<const physx::PxVec4> nvClothCurrentParticles = nv::cloth::readCurrentParticles(*m_nvCloth);
        const nv::cloth::MappedRange<const physx::PxVec4> nvClothPreviousParticles = nv::cloth::readPreviousParticles(*m_nvCloth);

        EXPECT_EQ(nvClothCurrentParticles.size(), nvClothPreviousParticles.size());
        for (size_t i = 0; i < nvClothCurrentParticles.size(); ++i)
        {
            ExpectEq(nvClothCurrentParticles[static_cast<uint32_t>(i)], nvClothPreviousParticles[static_cast<uint32_t>(i)]);
        }
    }

    TEST_F(NvClothSystemCloth, Cloth_Update_SimParticlesAreUpdated)
    {
        const AZ::Vector3 movement(6.0f, 1.0f, 3.0f);
        const auto previousParticles = m_cloth->GetParticles();

        // Fake all particles have been moved during simulation.
        {
            nv::cloth::MappedRange<physx::PxVec4> nvParticles = m_nvCloth->getCurrentParticles();
            for (auto& particle : nvParticles)
            {
                if (particle.w != 0.0f)
                {
                    particle.x += movement.GetX();
                    particle.y += movement.GetY();
                    particle.z += movement.GetZ();
                }
            }
            // nvcloth particles are set once nvParticles gets out of scope
        }

        m_cloth->Update();

        const auto& particles = m_cloth->GetParticles();
        for (size_t i = 0; i < particles.size(); ++i)
        {
            if (particles[i].GetW() == 0.0f)
            {
                EXPECT_THAT(particles[i].GetAsVector3(), IsCloseTolerance(previousParticles[i].GetAsVector3(), Tolerance));
            }
            else
            {
                EXPECT_THAT(particles[i].GetAsVector3(), IsCloseTolerance(previousParticles[i].GetAsVector3() + movement, Tolerance));
            }
        }
    }

    TEST_F(NvClothSystemCloth, Cloth_UpdateInvalidParticles_SimParticlesAreNotUpdated)
    {
        const auto previousParticles = m_cloth->GetParticles();

        // Fake a particle has been set to non-finite values during simulation
        {
            nv::cloth::MappedRange<physx::PxVec4> nvParticles = m_nvCloth->getCurrentParticles();
            for (auto& particle : nvParticles)
            {
                if (particle.w != 0.0f)
                {
                    particle.x = std::numeric_limits<float>::quiet_NaN();
                    particle.y = std::numeric_limits<float>::infinity();
                    break;
                }
            }
            // nvcloth particles are set once nvParticles gets out of scope
        }

        m_cloth->Update();

        const auto& particles = m_cloth->GetParticles();
        EXPECT_THAT(particles, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), previousParticles));
    }

    TEST_F(NvClothSystemCloth, Cloth_UpdateInvalidParticles_NativeClothParticlesAreRestored)
    {
        // Fake a particle has been set to non-finite values during simulation
        {
            nv::cloth::MappedRange<physx::PxVec4> nvParticles = m_nvCloth->getCurrentParticles();
            for (auto& particle : nvParticles)
            {
                if (particle.w != 0.0f)
                {
                    particle.x = std::numeric_limits<float>::quiet_NaN();
                    particle.y = std::numeric_limits<float>::infinity();
                    break;
                }
            }
            // nvcloth particles are set once nvParticles gets out of scope
        }

        m_cloth->Update();

        const nv::cloth::MappedRange<const physx::PxVec4> nvClothCurrentParticles = nv::cloth::readCurrentParticles(*m_nvCloth);
        const nv::cloth::MappedRange<const physx::PxVec4> nvClothPreviousParticles = nv::cloth::readPreviousParticles(*m_nvCloth);

        EXPECT_EQ(nvClothCurrentParticles.size(), nvClothPreviousParticles.size());
        for (size_t i = 0; i < nvClothCurrentParticles.size(); ++i)
        {
            ExpectEq(nvClothCurrentParticles[static_cast<uint32_t>(i)], nvClothPreviousParticles[static_cast<uint32_t>(i)]);
        }
    }

    TEST_F(NvClothSystemCloth, Cloth_UpdateInvalidParticlesManyAttempts_NativeClothParticlesAreRestoredToInitialPositions)
    {
        const auto& initialParticles = m_cloth->GetInitialParticles();

        const size_t numInvalidSimulations = 30;
        for (size_t i = 0; i < numInvalidSimulations; ++i)
        {
            // Fake a particle has been set to non-finite values during simulation
            {
                nv::cloth::MappedRange<physx::PxVec4> nvParticles = m_nvCloth->getCurrentParticles();
                for (auto& particle : nvParticles)
                {
                    if (particle.w != 0.0f)
                    {
                        particle.x = std::numeric_limits<float>::quiet_NaN();
                        particle.y = std::numeric_limits<float>::infinity();
                        break;
                    }
                }
                // nvcloth particles are set once nvParticles gets out of scope
            }

            m_cloth->Update();
        }

        const nv::cloth::MappedRange<const physx::PxVec4> nvClothCurrentParticles = nv::cloth::readCurrentParticles(*m_nvCloth);
        const nv::cloth::MappedRange<const physx::PxVec4> nvClothPreviousParticles = nv::cloth::readPreviousParticles(*m_nvCloth);

        EXPECT_EQ(initialParticles.size(), nvClothCurrentParticles.size());
        EXPECT_EQ(initialParticles.size(), nvClothPreviousParticles.size());
        for (size_t i = 0; i < initialParticles.size(); ++i)
        {
            ExpectEq(initialParticles[i], nvClothCurrentParticles[static_cast<uint32_t>(i)]);
            ExpectEq(initialParticles[i], nvClothPreviousParticles[static_cast<uint32_t>(i)]);
        }
    }

    TEST_F(NvClothSystemCloth, Cloth_CollisionAffectsStaticParticles_StaticParticlesAreModifiedDuringUpdate)
    {
        const AZ::Vector3 movement(6.0f, 1.0f, 3.0f);
        const auto previousParticles = m_cloth->GetParticles();

        m_cloth->GetClothConfigurator()->SetCollisionAffectsStaticParticles(true);

        // Fake all particles have been moved during simulation, cloth contains static particles.
        {
            nv::cloth::MappedRange<physx::PxVec4> nvParticles = m_nvCloth->getCurrentParticles();
            for (auto& particle : nvParticles)
            {
                particle.x += movement.GetX();
                particle.y += movement.GetY();
                particle.z += movement.GetZ();
            }
            // nvcloth particles are set once nvParticles gets out of scope
        }

        m_cloth->Update();

        const auto& particles = m_cloth->GetParticles();
        for (size_t i = 0; i < particles.size(); ++i)
        {
            EXPECT_THAT(particles[i].GetAsVector3(), IsCloseTolerance(previousParticles[i].GetAsVector3() + movement, Tolerance));
        }
    }

    TEST_F(NvClothSystemCloth, Cloth_CollisionDoesNotAffectStaticParticles_StaticParticlesAreNotModifiedDuringUpdate)
    {
        const AZ::Vector3 movement(6.0f, 1.0f, 3.0f);
        const auto previousParticles = m_cloth->GetParticles();

        m_cloth->GetClothConfigurator()->SetCollisionAffectsStaticParticles(false);

        // Fake all particles have been moved during simulation, cloth contains static particles.
        {
            nv::cloth::MappedRange<physx::PxVec4> nvParticles = m_nvCloth->getCurrentParticles();
            for (auto& particle : nvParticles)
            {
                particle.x += movement.GetX();
                particle.y += movement.GetY();
                particle.z += movement.GetZ();
            }
            // nvcloth particles are set once nvParticles gets out of scope
        }

        m_cloth->Update();

        const auto& particles = m_cloth->GetParticles();
        for (size_t i = 0; i < particles.size(); ++i)
        {
            if (particles[i].GetW() == 0.0f)
            {
                EXPECT_THAT(particles[i].GetAsVector3(), IsCloseTolerance(previousParticles[i].GetAsVector3(), Tolerance));
            }
            else
            {
                EXPECT_THAT(particles[i].GetAsVector3(), IsCloseTolerance(previousParticles[i].GetAsVector3() + movement, Tolerance));
            }
        }
    }

    TEST_F(NvClothSystemCloth, Cloth_ClothConfigurationSetTransform_TranslationAndRotationAreAppliedToNativeCloth)
    {
        const AZ::Transform identity = AZ::Transform::CreateIdentity();
        const AZ::Transform rotationX = AZ::Transform::CreateRotationX(AZ::DegToRad(35.0f));
        const AZ::Transform rotationYAndTranslation = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationY(AZ::DegToRad(-135.0f)),
            AZ::Vector3(36.0f, 50.0f, -69.35f));

        m_cloth->GetClothConfigurator()->SetTransform(identity);

        ExpectEq(identity.GetTranslation(), m_nvCloth->getTranslation());
        ExpectEq(identity.GetRotation(), m_nvCloth->getRotation());

        m_cloth->GetClothConfigurator()->SetTransform(rotationX);

        ExpectEq(rotationX.GetTranslation(), m_nvCloth->getTranslation());
        ExpectEq(rotationX.GetRotation(), m_nvCloth->getRotation());

        m_cloth->GetClothConfigurator()->SetTransform(rotationYAndTranslation);

        ExpectEq(rotationYAndTranslation.GetTranslation(), m_nvCloth->getTranslation());
        ExpectEq(rotationYAndTranslation.GetRotation(), m_nvCloth->getRotation());
    }

    TEST_F(NvClothSystemCloth, Cloth_ClothConfigurationSetMass_MassIsAppliedToClothSimParticlesAndNativeClothPreviousParticles)
    {
        const float globalMass = 2.0f;
        const auto& initialParticles = m_cloth->GetInitialParticles();

        m_cloth->GetClothConfigurator()->SetMass(globalMass);

        const auto& particles = m_cloth->GetParticles();
        for (size_t i = 0; i < initialParticles.size(); ++i)
        {
            EXPECT_NEAR(particles[i].GetW(), initialParticles[i].GetW() / globalMass, Tolerance);
        }

        // The inverse masses (W element) should have been copied into the previous particles inside NvCloth
        // to take effect for the next simulation update.
        const nv::cloth::MappedRange<const physx::PxVec4> nvClothPreviousParticles = nv::cloth::readPreviousParticles(*m_nvCloth);
        for (size_t i = 0; i < initialParticles.size(); ++i)
        {
            EXPECT_NEAR(nvClothPreviousParticles[static_cast<uint32_t>(i)].w, initialParticles[i].GetW() / globalMass, Tolerance);
        }
    }
} // namespace UnitTest
