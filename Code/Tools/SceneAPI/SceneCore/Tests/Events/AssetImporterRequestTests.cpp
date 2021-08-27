/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Math/Guid.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneCore/Mocks/Events/MockAssetImportRequest.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            using ::testing::StrictMock;
            using ::testing::NiceMock;
            using ::testing::Invoke;
            using ::testing::Return;
            using ::testing::Matcher;
            using ::testing::_;

            /*
            * ProcessingResultCombinerTests
            */

            TEST(ProcessingResultCombinerTests, GetResult_GetStoredValue_ReturnsTheDefaultValue)
            {
                ProcessingResultCombiner combiner;
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetResult());
            }

            TEST(ProcessingResultCombinerTests, OperatorEquals_SuccessIsStored_ResultIsSuccess)
            {
                ProcessingResultCombiner combiner;
                combiner = ProcessingResult::Success;
                EXPECT_EQ(ProcessingResult::Success, combiner.GetResult());
            }

            TEST(ProcessingResultCombinerTests, OperatorEquals_FailureIsStored_ResultIsFailure)
            {
                ProcessingResultCombiner combiner;
                combiner = ProcessingResult::Failure;
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetResult());
            }

            TEST(ProcessingResultCombinerTests, OperatorEquals_SuccessDoesNotOverwriteFailure_ResultIsFailure)
            {
                ProcessingResultCombiner combiner;
                combiner = ProcessingResult::Failure;
                combiner = ProcessingResult::Success;
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetResult());
            }

            TEST(ProcessingResultCombinerTests, OperatorEquals_IgnoreDoesNotChangeTheStoredValue_ResultIsSuccess)
            {
                ProcessingResultCombiner combiner;
                combiner = ProcessingResult::Success;
                combiner = ProcessingResult::Ignored;
                EXPECT_EQ(ProcessingResult::Success, combiner.GetResult());
            }


            /*
            * LoadingResultCombinerTests
            */

            TEST(LoadingResultCombinerTests, GetResult_GetStoredValues_ReturnsTheDefaultValues)
            {
                LoadingResultCombiner combiner;
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetManifestResult());
            }

            TEST(LoadingResultCombinerTests, OperatorEquals_AssetLoadedIsStored_ResultIsSuccess)
            {
                LoadingResultCombiner combiner;
                combiner = LoadingResult::AssetLoaded;
                EXPECT_EQ(ProcessingResult::Success, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetManifestResult());
            }

            TEST(LoadingResultCombinerTests, OperatorEquals_ManifestLoadedIsStored_ResultIsSuccess)
            {
                LoadingResultCombiner combiner;
                combiner = LoadingResult::ManifestLoaded;
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Success, combiner.GetManifestResult());
            }

            TEST(LoadingResultCombinerTests, OperatorEquals_AssetFailureIsStored_ResultIsFailure)
            {
                LoadingResultCombiner combiner;
                combiner = LoadingResult::AssetFailure;
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetManifestResult());
            }

            TEST(LoadingResultCombinerTests, OperatorEquals_ManifestFailureIsStored_ResultIsFailure)
            {
                LoadingResultCombiner combiner;
                combiner = LoadingResult::ManifestFailure;
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetManifestResult());
            }

            TEST(LoadingResultCombinerTests, OperatorEquals_LoadedDoesNotOverwriteFailure_ResultIsFailure)
            {
                LoadingResultCombiner combiner;
                combiner = LoadingResult::AssetFailure;
                combiner = LoadingResult::ManifestFailure;

                combiner = LoadingResult::AssetLoaded;
                combiner = LoadingResult::ManifestLoaded;
                
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetManifestResult());
            }

            TEST(LoadingResultCombinerTests, OperatorEquals_IgnoreDoesNotChangeTheStoredValue_ResultIsSuccess)
            {
                LoadingResultCombiner combiner;
                combiner = LoadingResult::AssetLoaded;
                combiner = LoadingResult::ManifestLoaded;

                combiner = LoadingResult::Ignored;
                combiner = LoadingResult::Ignored;

                EXPECT_EQ(ProcessingResult::Success, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Success, combiner.GetManifestResult());
            }


            /*
            * AssetImporterRequestTests
            */
            class AssetImporterRequestTests
                : public ::testing::Test
                , public Debug::TraceMessageBus::Handler
            {
            public:
                void SetUp() override
                {
                    BusConnect();
                    m_testId = Uuid::CreateString("{46CAD46C-A3CB-4E2A-A775-2ED9DCF018FC}");
                }

                void TearDown() override
                {
                    BusDisconnect();
                }

                bool OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override
                {
                    m_assertTriggered = true;
                    return true;
                }
                 
                Uuid m_testId;
                bool m_assertTriggered = false;
            };
            
            TEST_F(AssetImporterRequestTests, LoadSceneFromVerifiedPath_FailureToPrepare_LoadAndFollowingStepsNotCalledAndReturnsNullptr)
            {
                StrictMock<MockAssetImportRequestHandler> handler;
                handler.SetDefaultExtensions();
                handler.SetDefaultProcessingResults(false);

                ON_CALL(handler, PrepareForAssetLoading(Matcher<Containers::Scene&>(_), Matcher<AssetImportRequest::RequestingApplication>(_)))
                    .WillByDefault(Return(ProcessingResult::Failure));

                EXPECT_CALL(handler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(handler, GetSupportedFileExtensions(_)).Times(0);

                EXPECT_CALL(handler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(handler, LoadAsset(_, _, _, _)).Times(0);
                EXPECT_CALL(handler, FinalizeAssetLoading(_, _)).Times(0);
                EXPECT_CALL(handler, UpdateManifest(_, _, _)).Times(0);

                AZStd::shared_ptr<Containers::Scene> result = 
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid());
                EXPECT_EQ(nullptr, result);
            }

            TEST_F(AssetImporterRequestTests, LoadSceneFromVerifiedPath_AssetFailureToLoad_UpdateManifestNotCalledAndReturnsNullptr)
            {
                StrictMock<MockAssetImportRequestHandler> handler;
                handler.SetDefaultExtensions();
                handler.SetDefaultProcessingResults(false);

                ON_CALL(handler, LoadAsset(
                    Matcher<Containers::Scene&>(_), Matcher<const AZStd::string&>(_), Matcher<const Uuid&>(_), Matcher<AssetImportRequest::RequestingApplication>(_)))
                    .WillByDefault(Return(LoadingResult::AssetFailure));

                EXPECT_CALL(handler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(handler, GetSupportedFileExtensions(_)).Times(0);

                EXPECT_CALL(handler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(handler, LoadAsset(_, _, _, _)).Times(1);
                EXPECT_CALL(handler, FinalizeAssetLoading(_, _)).Times(1);
                EXPECT_CALL(handler, UpdateManifest(_, _, _)).Times(0);

                AZStd::shared_ptr<Containers::Scene> result = 
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid());
                EXPECT_EQ(nullptr, result);
            }

            TEST_F(AssetImporterRequestTests, LoadSceneFromVerifiedPath_ManifestFailureToLoad_UpdateManifestNotCalledAndReturnsNullptr)
            {
                StrictMock<MockAssetImportRequestHandler> handler;
                handler.SetDefaultExtensions();
                handler.SetDefaultProcessingResults(true);

                ON_CALL(handler, LoadAsset(
                    Matcher<Containers::Scene&>(_), Matcher<const AZStd::string&>(_), Matcher<const Uuid&>(_), Matcher<AssetImportRequest::RequestingApplication>(_)))
                    .WillByDefault(Return(LoadingResult::ManifestFailure));

                EXPECT_CALL(handler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(handler, GetSupportedFileExtensions(_)).Times(0);

                EXPECT_CALL(handler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(handler, LoadAsset(_, _, _, _)).Times(1);
                EXPECT_CALL(handler, FinalizeAssetLoading(_, _)).Times(1);
                EXPECT_CALL(handler, UpdateManifest(_, _, _)).Times(0);

                AZStd::shared_ptr<Containers::Scene> result =
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid());
                EXPECT_EQ(nullptr, result);
            }

            TEST_F(AssetImporterRequestTests, LoadSceneFromVerifiedPath_NothingLoaded_UpdateManifestNotCalledAndReturnsNullptr)
            {
                StrictMock<MockAssetImportRequestHandler> handler;
                handler.SetDefaultExtensions();
                handler.SetDefaultProcessingResults(false);

                ON_CALL(handler, LoadAsset(
                    Matcher<Containers::Scene&>(_), Matcher<const AZStd::string&>(_), Matcher<const Uuid&>(_), Matcher<AssetImportRequest::RequestingApplication>(_)))
                    .WillByDefault(Return(LoadingResult::Ignored));

                EXPECT_CALL(handler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(handler, GetSupportedFileExtensions(_)).Times(0);

                EXPECT_CALL(handler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(handler, LoadAsset(_, _, _, _)).Times(1);
                EXPECT_CALL(handler, FinalizeAssetLoading(_, _)).Times(1);
                EXPECT_CALL(handler, UpdateManifest(_, _, _)).Times(0);

                AZStd::shared_ptr<Containers::Scene> result =
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid());
                EXPECT_EQ(nullptr, result);
            }

            TEST_F(AssetImporterRequestTests, LoadSceneFromVerifiedPath_ManifestUpdateFailed_ReturnsNullptr)
            {
                StrictMock<MockAssetImportRequestHandler> assetHandler;
                assetHandler.SetDefaultExtensions();
                assetHandler.SetDefaultProcessingResults(false);
                StrictMock<MockAssetImportRequestHandler> manifestHandler;
                manifestHandler.SetDefaultExtensions();
                manifestHandler.SetDefaultProcessingResults(true);

                ON_CALL(assetHandler, UpdateManifest(Matcher<Containers::Scene&>(_), Matcher<AssetImportRequest::ManifestAction>(_), 
                    Matcher<AssetImportRequest::RequestingApplication>(_)))
                    .WillByDefault(Return(ProcessingResult::Failure));

                EXPECT_CALL(assetHandler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(assetHandler, GetSupportedFileExtensions(_)).Times(0);
                EXPECT_CALL(manifestHandler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(manifestHandler, GetSupportedFileExtensions(_)).Times(0);

                EXPECT_CALL(assetHandler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(assetHandler, LoadAsset(_, _, _, _)).Times(1);
                EXPECT_CALL(assetHandler, FinalizeAssetLoading(_, _)).Times(1);
                EXPECT_CALL(assetHandler, UpdateManifest(_, _, _)).Times(1);
                EXPECT_CALL(manifestHandler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(manifestHandler, LoadAsset(_, _, _, _)).Times(1);
                EXPECT_CALL(manifestHandler, FinalizeAssetLoading(_, _)).Times(1);
                EXPECT_CALL(manifestHandler, UpdateManifest(_, _, _)).Times(1);

                AZStd::shared_ptr<Containers::Scene> result =
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid());
                EXPECT_EQ(nullptr, result);
            }

            TEST_F(AssetImporterRequestTests, LoadSceneFromVerifiedPath_FullLoad_ReturnsValidScenePointer)
            {
                StrictMock<MockAssetImportRequestHandler> assetHandler;
                assetHandler.SetDefaultExtensions();
                assetHandler.SetDefaultProcessingResults(false);
                StrictMock<MockAssetImportRequestHandler> manifestHandler;
                manifestHandler.SetDefaultExtensions();
                manifestHandler.SetDefaultProcessingResults(true);

                EXPECT_CALL(assetHandler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(assetHandler, GetSupportedFileExtensions(_)).Times(0);
                EXPECT_CALL(manifestHandler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(manifestHandler, GetSupportedFileExtensions(_)).Times(0);

                EXPECT_CALL(assetHandler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(assetHandler, LoadAsset(_, _, _, _)).Times(1);
                EXPECT_CALL(assetHandler, FinalizeAssetLoading(_, _)).Times(1);
                EXPECT_CALL(assetHandler, UpdateManifest(_, _, _)).Times(1);
                EXPECT_CALL(manifestHandler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(manifestHandler, LoadAsset(_, _, _, _)).Times(1);
                EXPECT_CALL(manifestHandler, FinalizeAssetLoading(_, _)).Times(1);
                EXPECT_CALL(manifestHandler, UpdateManifest(_, _, _)).Times(1);

                AZStd::shared_ptr<Containers::Scene> result =
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid());
                EXPECT_NE(nullptr, result);
            }
            
        } // Events
    } // SceneAPI
} // AZ
