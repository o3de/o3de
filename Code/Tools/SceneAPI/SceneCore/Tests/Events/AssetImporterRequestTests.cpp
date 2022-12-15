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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/UnitTest/Mocks/MockSettingsRegistry.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneCore/Mocks/Events/MockAssetImportRequest.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/Groups/MockIGroup.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

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

            using ProcessingResultCombinerTests = UnitTest::LeakDetectionFixture;
            TEST_F(ProcessingResultCombinerTests, GetResult_GetStoredValue_ReturnsTheDefaultValue)
            {
                ProcessingResultCombiner combiner;
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetResult());
            }

            TEST_F(ProcessingResultCombinerTests, OperatorEquals_SuccessIsStored_ResultIsSuccess)
            {
                ProcessingResultCombiner combiner;
                combiner = ProcessingResult::Success;
                EXPECT_EQ(ProcessingResult::Success, combiner.GetResult());
            }

            TEST_F(ProcessingResultCombinerTests, OperatorEquals_FailureIsStored_ResultIsFailure)
            {
                ProcessingResultCombiner combiner;
                combiner = ProcessingResult::Failure;
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetResult());
            }

            TEST_F(ProcessingResultCombinerTests, OperatorEquals_SuccessDoesNotOverwriteFailure_ResultIsFailure)
            {
                ProcessingResultCombiner combiner;
                combiner = ProcessingResult::Failure;
                combiner = ProcessingResult::Success;
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetResult());
            }

            TEST_F(ProcessingResultCombinerTests, OperatorEquals_IgnoreDoesNotChangeTheStoredValue_ResultIsSuccess)
            {
                ProcessingResultCombiner combiner;
                combiner = ProcessingResult::Success;
                combiner = ProcessingResult::Ignored;
                EXPECT_EQ(ProcessingResult::Success, combiner.GetResult());
            }


            /*
            * LoadingResultCombinerTests
            */

            using LoadingResultCombinerTests = UnitTest::LeakDetectionFixture;
            TEST_F(LoadingResultCombinerTests, GetResult_GetStoredValues_ReturnsTheDefaultValues)
            {
                LoadingResultCombiner combiner;
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetManifestResult());
            }

            TEST_F(LoadingResultCombinerTests, OperatorEquals_AssetLoadedIsStored_ResultIsSuccess)
            {
                LoadingResultCombiner combiner;
                combiner = LoadingResult::AssetLoaded;
                EXPECT_EQ(ProcessingResult::Success, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetManifestResult());
            }

            TEST_F(LoadingResultCombinerTests, OperatorEquals_ManifestLoadedIsStored_ResultIsSuccess)
            {
                LoadingResultCombiner combiner;
                combiner = LoadingResult::ManifestLoaded;
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Success, combiner.GetManifestResult());
            }

            TEST_F(LoadingResultCombinerTests, OperatorEquals_AssetFailureIsStored_ResultIsFailure)
            {
                LoadingResultCombiner combiner;
                combiner = LoadingResult::AssetFailure;
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetManifestResult());
            }

            TEST_F(LoadingResultCombinerTests, OperatorEquals_ManifestFailureIsStored_ResultIsFailure)
            {
                LoadingResultCombiner combiner;
                combiner = LoadingResult::ManifestFailure;
                EXPECT_EQ(ProcessingResult::Ignored, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetManifestResult());
            }

            TEST_F(LoadingResultCombinerTests, OperatorEquals_LoadedDoesNotOverwriteFailure_ResultIsFailure)
            {
                LoadingResultCombiner combiner;
                combiner = LoadingResult::AssetFailure;
                combiner = LoadingResult::ManifestFailure;

                combiner = LoadingResult::AssetLoaded;
                combiner = LoadingResult::ManifestLoaded;
                
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetAssetResult());
                EXPECT_EQ(ProcessingResult::Failure, combiner.GetManifestResult());
            }

            TEST_F(LoadingResultCombinerTests, OperatorEquals_IgnoreDoesNotChangeTheStoredValue_ResultIsSuccess)
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
                : public UnitTest::LeakDetectionFixture
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
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid(), "");
                EXPECT_EQ(nullptr, result);
            }

            TEST_F(AssetImporterRequestTests, LoadSceneFromVerifiedPath_LoadWithEmptySceneManifest_ResultsInDefault)
            {
                NiceMock<MockAssetImportRequestHandler> manifestHandler;
                manifestHandler.SetDefaultExtensions();
                NiceMock<MockAssetImportRequestHandler> assetHandler;
                assetHandler.SetDefaultExtensions();

                ON_CALL(manifestHandler, PrepareForAssetLoading(Matcher<Containers::Scene&>(_), Matcher<AssetImportRequest::RequestingApplication>(_)))
                    .WillByDefault(Return(ProcessingResult::Success));

                ON_CALL(assetHandler, PrepareForAssetLoading(Matcher<Containers::Scene&>(_), Matcher<AssetImportRequest::RequestingApplication>(_)))
                    .WillByDefault(Return(ProcessingResult::Success));

                bool anEmptyManifestWorks = false;
                ON_CALL(manifestHandler, UpdateManifest).WillByDefault([&anEmptyManifestWorks](Containers::Scene&, AssetImportRequest::ManifestAction action, AssetImportRequest::RequestingApplication)
                {
                    if (action == AssetImportRequest::ManifestAction::ConstructDefault)
                    {
                        anEmptyManifestWorks = true;
                        return ProcessingResult::Success;
                    }
                    return ProcessingResult::Failure;
                });

                ON_CALL(manifestHandler, LoadAsset(
                    Matcher<Containers::Scene&>(_), Matcher<const AZStd::string&>(_), Matcher<const Uuid&>(_), Matcher<AssetImportRequest::RequestingApplication>(_)))
                    .WillByDefault(Return(LoadingResult::ManifestLoaded));

                ON_CALL(assetHandler, LoadAsset(
                    Matcher<Containers::Scene&>(_), Matcher<const AZStd::string&>(_), Matcher<const Uuid&>(_), Matcher<AssetImportRequest::RequestingApplication>(_)))
                    .WillByDefault(Return(LoadingResult::AssetLoaded));

                EXPECT_CALL(manifestHandler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(manifestHandler, GetSupportedFileExtensions(_)).Times(0);
                EXPECT_CALL(manifestHandler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(manifestHandler, LoadAsset(_, _, _, _)).Times(1);

                AZStd::shared_ptr<Containers::Scene> result =
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid(), "");
                EXPECT_NE(nullptr, result);
                EXPECT_TRUE(anEmptyManifestWorks);
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
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid(), "");
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
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid(), "");
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
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid(), "");
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
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid(), "");
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
                    AssetImportRequest::LoadSceneFromVerifiedPath("test.asset", m_testId, Events::AssetImportRequest::RequestingApplication::Generic, SceneCore::LoadingComponent::TYPEINFO_Uuid(), "");
                EXPECT_NE(nullptr, result);
            }

            /*
            * AssetImporterRequestToolTests
            */
            class AssetImporterRequestToolTests
                : public ::UnitTest::LeakDetectionFixture
            {
            public:
                void SetUp() override
                {
                    UnitTest::LeakDetectionFixture::SetUp();

                    m_settings.reset(new AZ::NiceSettingsRegistrySimpleMock);
                    ON_CALL(*m_settings.get(), Get(::testing::Matcher<bool&>(::testing::_), ::testing::_))
                        .WillByDefault([this](bool& value, AZStd::string_view key) -> bool
                            {
                                if (key == AZ::SceneAPI::Utilities::Key_AssetProcessorInDebugOutput)
                                {
                                    value = this->m_inDebugOutputMode;
                                }
                                else
                                {
                                    value = false;
                                }
                                return true;
                            });
                    AZ::SettingsRegistry::Register(m_settings.get());
                }

                void TearDown() override
                {
                    AZ::SettingsRegistry::Unregister(m_settings.get());
                    m_settings.reset();

                    UnitTest::LeakDetectionFixture::TearDown();
                }

                void SetDefaultResults(StrictMock<MockAssetImportRequestHandler>& handler)
                {
                    using namespace AZ::SceneAPI;

                    ON_CALL(handler, PrepareForAssetLoading(Matcher<Containers::Scene&>(_), Matcher<AssetImportRequest::RequestingApplication>(_)))
                        .WillByDefault(Return(ProcessingResult::Ignored));

                    ON_CALL(handler, LoadAsset)
                        .WillByDefault([](Containers::Scene&, const AZStd::string&, const Uuid&, AssetImportRequest::RequestingApplication) -> Events::LoadingResult
                            {
                                return LoadingResult::AssetLoaded;
                            }
                    );

                    ON_CALL(handler, UpdateManifest)
                        .WillByDefault([](Containers::Scene& scene, AssetImportRequest::ManifestAction, AssetImportRequest::RequestingApplication) -> Events::ProcessingResult
                            {
                                scene.GetManifest().AddEntry(AZStd::make_shared<AZ::SceneAPI::DataTypes::MockIGroup>());
                                return ProcessingResult::Success;
                            });
                }

                AZStd::unique_ptr<AZ::NiceSettingsRegistrySimpleMock> m_settings;
                bool m_inDebugOutputMode = false;
            };

            TEST_F(AssetImporterRequestToolTests, AssetImportRequestBus_UpdateSceneManifest_DoesNotLogHandlers)
            {
                using namespace AZ::SceneAPI::Events;
                using namespace AZ::SceneAPI::SceneCore;

                StrictMock<MockAssetImportRequestHandler> assetHandler;
                assetHandler.SetDefaultExtensions();
                assetHandler.SetDefaultProcessingResults(false);
                ON_CALL(assetHandler, GetPolicyName).WillByDefault([](AZStd::string& result)
                {
                    result = "assetHandler";
                });

                StrictMock<MockAssetImportRequestHandler> manifestHandler;
                manifestHandler.SetDefaultExtensions();
                manifestHandler.SetDefaultProcessingResults(true);
                ON_CALL(manifestHandler, GetPolicyName).WillByDefault([](AZStd::string& result)
                {
                    result = "manifestHandler";
                });

                EXPECT_CALL(assetHandler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(assetHandler, GetSupportedFileExtensions(_)).Times(0);
                EXPECT_CALL(assetHandler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(assetHandler, LoadAsset(_, _, _, _)).Times(1);
                EXPECT_CALL(assetHandler, FinalizeAssetLoading(_, _)).Times(1);
                EXPECT_CALL(assetHandler, UpdateManifest(_, _, _)).Times(1);
                EXPECT_CALL(assetHandler, GetPolicyName(_)).Times(0);

                EXPECT_CALL(manifestHandler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(manifestHandler, GetSupportedFileExtensions(_)).Times(0);
                EXPECT_CALL(manifestHandler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(manifestHandler, LoadAsset(_, _, _, _)).Times(1);
                EXPECT_CALL(manifestHandler, FinalizeAssetLoading(_, _)).Times(1);
                EXPECT_CALL(manifestHandler, UpdateManifest(_, _, _)).Times(1);
                EXPECT_CALL(manifestHandler, GetPolicyName(_)).Times(0);

                auto result = AssetImportRequest::LoadSceneFromVerifiedPath(
                    "test.asset",
                    AZ::Uuid("{B28DA8AF-B5F5-48E2-8E1A-3FE2CEFC2817}"),
                    AssetImportRequest::RequestingApplication::Generic,
                    LoadingComponent::TYPEINFO_Uuid(),
                    "");

                EXPECT_NE(nullptr, result);
            }

            TEST_F(AssetImporterRequestToolTests, AssetImportRequestBus_UpdateSceneManifest_DoesLogHandlers)
            {
                using namespace AZ::SceneAPI::Events;
                using namespace AZ::SceneAPI::SceneCore;

                m_inDebugOutputMode = true;

                StrictMock<MockAssetImportRequestHandler> assetHandler;
                assetHandler.SetDefaultExtensions();
                ON_CALL(assetHandler, GetPolicyName)
                    .WillByDefault([](AZStd::string& result)
                    {
                        result = "assetHandler";
                    });
                SetDefaultResults(assetHandler);

                StrictMock<MockAssetImportRequestHandler> manifestHandler;
                manifestHandler.SetDefaultExtensions();
                ON_CALL(manifestHandler, GetPolicyName)
                    .WillByDefault([](AZStd::string& result)
                    {
                        result = "manifestHandler";
                    });
                SetDefaultResults(manifestHandler);

                EXPECT_CALL(assetHandler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(assetHandler, GetSupportedFileExtensions(_)).Times(0);
                EXPECT_CALL(assetHandler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(assetHandler, LoadAsset(_, _, _, _)).Times(1);
                EXPECT_CALL(assetHandler, FinalizeAssetLoading(_, _)).Times(1);
                EXPECT_CALL(assetHandler, UpdateManifest(_, _, _)).Times(1);
                EXPECT_CALL(assetHandler, GetPolicyName(_)).Times(1);

                EXPECT_CALL(manifestHandler, GetManifestExtension(_)).Times(0);
                EXPECT_CALL(manifestHandler, GetSupportedFileExtensions(_)).Times(0);
                EXPECT_CALL(manifestHandler, PrepareForAssetLoading(_, _)).Times(1);
                EXPECT_CALL(manifestHandler, LoadAsset(_, _, _, _)).Times(1);
                EXPECT_CALL(manifestHandler, FinalizeAssetLoading(_, _)).Times(1);
                EXPECT_CALL(manifestHandler, UpdateManifest(_, _, _)).Times(1);
                EXPECT_CALL(manifestHandler, GetPolicyName(_)).Times(1);

                AssetImportRequest::LoadSceneFromVerifiedPath(
                    "test.asset",
                    AZ::Uuid("{B28DA8AF-B5F5-48E2-8E1A-3FE2CEFC2817}"),
                    AssetImportRequest::RequestingApplication::Generic,
                    LoadingComponent::TYPEINFO_Uuid(),
                    "");
            }

        } // Events
    } // SceneAPI
} // AZ
