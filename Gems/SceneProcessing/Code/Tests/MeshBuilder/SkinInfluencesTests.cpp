/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/Math/Random.h>

#include <Generation/Components/MeshOptimizer/MeshBuilder.h>
#include <Generation/Components/MeshOptimizer/MeshBuilderSkinningInfo.h>

namespace AZ::MeshBuilder
{
    struct SkinInfluencesTestParam
    {
        size_t numOrgVertices = 0;
        size_t maxSourceInfluences = 0;
        AZ::u32 maxInfluencesAfterOptimization = 0;
    };

    class SkinInfluencesFixture
        : public UnitTest::LeakDetectionFixture
        , public ::testing::WithParamInterface<SkinInfluencesTestParam>
    {
    public:
        static AZStd::unique_ptr<MeshBuilder> SetUpMeshBuilder(size_t numOrgVertices, size_t numSkinInfluences)
        {
            auto meshBuilder = AZStd::make_unique<MeshBuilder>(numOrgVertices);

            // Original vertex numbers
            meshBuilder->AddLayer<MeshBuilderVertexAttributeLayerUInt32>(
                numOrgVertices,
                false,
                false
            );

            // The positions layer
            meshBuilder->AddLayer<MeshBuilderVertexAttributeLayerVector3>(
                numOrgVertices,
                false,
                true
            );

            meshBuilder->SetSkinningInfo(SetUpSkinningInfo(numOrgVertices, numSkinInfluences));

            return meshBuilder;
        }

        static AZStd::unique_ptr<MeshBuilderSkinningInfo> SetUpSkinningInfo(size_t numOrgVertices, size_t numSkinInfluences)
        {
            auto skinningInfo = AZStd::make_unique<MeshBuilderSkinningInfo>(numOrgVertices);

            AZ::SimpleLcgRandom random;
            random.SetSeed(875960);

            const float expectedTotalWeight = 1.0f;
            for (size_t v = 0; v < numOrgVertices; ++v)
            {
                float totalWeight = 1.0f;
                for (size_t i = 0; i < numSkinInfluences; ++i)
                {
                    const float influenceWeight = (i != numSkinInfluences - 1 ? fmod(random.GetRandomFloat(), totalWeight) : totalWeight);
                    skinningInfo->AddInfluence(v, {i, influenceWeight});
                    totalWeight -= influenceWeight;
                }
                const float totalSkinInfluenceWeight = CalcSkinInfluencesTotalWeight(skinningInfo.get(), v);
                EXPECT_NEAR(totalSkinInfluenceWeight, expectedTotalWeight, 0.00001f /* tolerance */) << "totalSkinInfluenceWeight should be 1.0f or near 1.0f.";
            }
            return skinningInfo;
        }

        static AZStd::vector<MeshBuilderSkinningInfo::Influence> GetInfluenceVector(const MeshBuilderSkinningInfo* skinInfo, size_t vtxNum)
        {
            const size_t numInfluence = skinInfo->GetNumInfluences(vtxNum);
            AZStd::vector<MeshBuilderSkinningInfo::Influence> influences;
            influences.reserve(numInfluence);
            for (size_t i = 0; i < numInfluence; ++i)
            {
                influences.push_back(skinInfo->GetInfluence(vtxNum, i));
            }
            return influences;
        }

        static float CalcSkinInfluencesTotalWeight(const MeshBuilderSkinningInfo* skinInfo, size_t vtxNum)
        {
            AZStd::vector<MeshBuilderSkinningInfo::Influence> influences = GetInfluenceVector(skinInfo, vtxNum);
            return CalcTotalWeight(influences);
        }

        static float CalcTotalWeight(const AZStd::vector<MeshBuilderSkinningInfo::Influence>& influences)
        {
            float totalWeight = 0.0f;
            for (const auto& influence : influences)
            {
                totalWeight += influence.mWeight;
            }
            return totalWeight;
        }
    };

    // Test that skin influence's renormalization after Optimize still has the same sum.
    TEST_P(SkinInfluencesFixture, RenormalizationAfterOptimizeTests)
    {
        const SkinInfluencesTestParam& testParam = GetParam();
        auto meshBuilder = SetUpMeshBuilder(testParam.numOrgVertices, testParam.maxSourceInfluences);

        MeshBuilderSkinningInfo* testSkinInfo = meshBuilder->GetSkinningInfo();
        const float expectedTotalWeight = 1.0f;
        for (size_t v = 0; v < testParam.numOrgVertices; ++v)
        {
            AZStd::vector<MeshBuilderSkinningInfo::Influence> influences = GetInfluenceVector(testSkinInfo, v);
           
            testSkinInfo->Optimize(influences, testParam.maxInfluencesAfterOptimization);
            EXPECT_EQ(influences.size(), testParam.maxInfluencesAfterOptimization);
            const float totalWeight = CalcTotalWeight(influences);
            EXPECT_NEAR(totalWeight, expectedTotalWeight, 0.00001f /* tolerance */) << "totalWeight of all influences in a vertex should be 1.0f.";
        }
    }

    static constexpr const AZStd::array skinInfluenceTestData {
        SkinInfluencesTestParam {/*.numOrgVertices =*/3, /*.maxSourceInfluences =*/6, /*.maxInfluencesAfterOptimization =*/1},
        SkinInfluencesTestParam {/*.numOrgVertices =*/3, /*.maxSourceInfluences =*/8, /*.maxInfluencesAfterOptimization =*/2},
        SkinInfluencesTestParam {/*.numOrgVertices =*/6, /*.maxSourceInfluences =*/8, /*.maxInfluencesAfterOptimization =*/3},
        SkinInfluencesTestParam {/*.numOrgVertices =*/6, /*.maxSourceInfluences =*/12, /*.maxInfluencesAfterOptimization =*/4},
        SkinInfluencesTestParam {/*.numOrgVertices =*/100, /*.maxSourceInfluences =*/6, /*.maxInfluencesAfterOptimization =*/1},
        SkinInfluencesTestParam {/*.numOrgVertices =*/300, /*.maxSourceInfluences =*/8, /*.maxInfluencesAfterOptimization =*/2},
        SkinInfluencesTestParam {/*.numOrgVertices =*/500, /*.maxSourceInfluences =*/12, /*.maxInfluencesAfterOptimization =*/3},
        SkinInfluencesTestParam {/*.numOrgVertices =*/700, /*.maxSourceInfluences =*/12, /*.maxInfluencesAfterOptimization =*/3},
    };

    INSTANTIATE_TEST_CASE_P(SkinInfluenceOptimizeTests,
        SkinInfluencesFixture,
        ::testing::ValuesIn(skinInfluenceTestData)
    );
} // namespace AZ::MeshBuilder
