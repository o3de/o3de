/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/UnitTest/UnitTest.h>
#include "assetBuilderSDKTest.h"

namespace AssetProcessorTests
{
    class JobOutputTests
        : public AssetProcessor::AssetBuilderSDKTest
        , public UnitTest::TraceBusRedirector
    {
    protected:
        void SetUp() override
        {
            AssetProcessor::AssetBuilderSDKTest::SetUp();
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
            m_productFile1 = AZStd::make_unique<AZStd::string>();
            m_productFile2 = AZStd::make_unique<AZStd::string>();
        }

        void TearDown() override
        {
            m_productFile1.reset();
            m_productFile2.reset();
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            AssetProcessor::AssetBuilderSDKTest::TearDown();
        }

        bool OnError([[maybe_unused]] const char* window, const char* message) override
        {
            if (!m_productFile1->empty() && !m_productFile1->empty())
            {
                // filter out expected errors
                AZStd::string incoming {message};
                if (incoming.find(m_productFile1->c_str()) != AZStd::string::npos &&
                    incoming.find(m_productFile2->c_str()) != AZStd::string::npos )
                {
                    return true;
                }
            }

            if (UnitTest::TestRunner::Instance().m_isAssertTest)
            {
                UnitTest::TestRunner::Instance().ProcessAssert(message, __FILE__, __LINE__, UnitTest::AssertionExpr(false));
                return true;
            }
            else if (UnitTest::TestRunner::Instance().m_suppressErrors)
            {
                GTEST_MESSAGE_(message, ::testing::TestPartResult::kNonFatalFailure);
                return true;
            }
            return false;
        }

        AZStd::unique_ptr<AZStd::string> m_productFile1;
        AZStd::unique_ptr<AZStd::string> m_productFile2;
    };

    TEST_F(JobOutputTests, JobProduct_DifferentSubIds_Works)
    {
        AssetBuilderSDK::ProcessJobResponse processJobResponse;
        processJobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

        {
            AssetBuilderSDK::JobProduct jobProduct;
            jobProduct.m_productSubID = 279033426;
            jobProduct.m_productFileName = "default_Boxes_F2724FB9_F0D2_5BEB_B6C8_2162A1FF281F__lod0.azlod";
            jobProduct.m_productAssetType = AZ::Data::AssetType::CreateString("{65B5A801-B9B9-4160-9CB4-D40DAA50B15C}");
            processJobResponse.m_outputProducts.emplace_back(jobProduct);
        }

        {
            AssetBuilderSDK::JobProduct jobProduct;
            jobProduct.m_productSubID = 1;
            jobProduct.m_productFileName = "default_Boxes_B1C126EF_C4D4_522C_864B_0FE3684F7CA1__lod0_TANGENT0.azbuffer";
            jobProduct.m_productAssetType = AZ::Data::AssetType::CreateString("{F6C5EA8A-1DB3-456E-B970-B6E2AB262AED}");
            processJobResponse.m_outputProducts.emplace_back(jobProduct);
        }

        EXPECT_TRUE(processJobResponse.Succeeded());
        EXPECT_TRUE(processJobResponse.ReportProductCollisions());
    }

    TEST_F(JobOutputTests, JobProduct_SubIdsWithCollisions_Detected)
    {
        AssetBuilderSDK::ProcessJobResponse processJobResponse;
        processJobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        *m_productFile2 = "default_Boxes_B1C126EF_C4D4_522C_864B_0FE3684F7CA1__lod0_TANGENT0.azbuffer";
        *m_productFile1 = "default_Boxes_F2724FB9_F0D2_5BEB_B6C8_2162A1FF281F__lod0.azlod";

        {
            AssetBuilderSDK::JobProduct jobProduct;
            jobProduct.m_productSubID = 279033426;
            jobProduct.m_productFileName = *m_productFile2;
            jobProduct.m_productAssetType = AZ::Data::AssetType::CreateString("{F6C5EA8A-1DB3-456E-B970-B6E2AB262AED}");
            processJobResponse.m_outputProducts.emplace_back(jobProduct);
        }

        {
            AssetBuilderSDK::JobProduct jobProduct;
            jobProduct.m_productSubID = 279033426;
            jobProduct.m_productFileName = *m_productFile1;
            jobProduct.m_productAssetType = AZ::Data::AssetType::CreateString("{65B5A801-B9B9-4160-9CB4-D40DAA50B15C}");
            processJobResponse.m_outputProducts.emplace_back(jobProduct);
        }

        EXPECT_TRUE(processJobResponse.Succeeded());
        EXPECT_FALSE(processJobResponse.ReportProductCollisions());
    }
}
