#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            using ::testing::Invoke;
            using ::testing::Return;
            using ::testing::Matcher;
            using ::testing::_;

            class MockAssetImportRequestHandler
                : public AssetImportRequestBus::Handler
            {
            public:
                MockAssetImportRequestHandler()
                {
                    BusConnect();
                }

                ~MockAssetImportRequestHandler() override
                {
                    BusDisconnect();
                }

                MOCK_METHOD1(GetSupportedFileExtensions,
                    void(AZStd::unordered_set<AZStd::string>& extensions));
                MOCK_METHOD1(GetManifestExtension,
                    void(AZStd::string& result));

                MOCK_METHOD2(PrepareForAssetLoading,
                    ProcessingResult(Containers::Scene& scene, RequestingApplication requester));
                MOCK_METHOD4(LoadAsset,
                    LoadingResult(Containers::Scene& scene, const AZStd::string& path, const Uuid& guid, RequestingApplication requester));
                MOCK_METHOD2(FinalizeAssetLoading,
                    void(Containers::Scene& scene, RequestingApplication requester));
                MOCK_METHOD3(UpdateManifest,
                    ProcessingResult(Containers::Scene& scene, ManifestAction action, RequestingApplication requester));

                MOCK_CONST_METHOD1(GetPolicyName, void(AZStd::string& result));

                void DefaultGetSupportedFileExtensions(AZStd::unordered_set<AZStd::string>& extensions)
                {
                    extensions.insert(".asset");
                }

                void DefaultGetManifestExtension(AZStd::string& result)
                {
                    result = ".manifest";
                }

                void SetDefaultExtensions()
                {
                    ON_CALL(*this, GetSupportedFileExtensions(Matcher<AZStd::unordered_set<AZStd::string>&>(_)))
                        .WillByDefault(Invoke(this, &MockAssetImportRequestHandler::DefaultGetSupportedFileExtensions));
                    ON_CALL(*this, GetManifestExtension(Matcher<AZStd::string&>(_)))
                        .WillByDefault(Invoke(this, &MockAssetImportRequestHandler::DefaultGetManifestExtension));
                }

                void SetDefaultProcessingResults(bool forManifest)
                {
                    ON_CALL(*this, PrepareForAssetLoading(Matcher<Containers::Scene&>(_), Matcher<RequestingApplication>(_)))
                        .WillByDefault(Return(ProcessingResult::Ignored));
                    if (forManifest)
                    {
                        ON_CALL(*this, LoadAsset(Matcher<Containers::Scene&>(_), Matcher<const AZStd::string&>(_), Matcher<const Uuid&>(_), Matcher<RequestingApplication>(_)))
                            .WillByDefault(Return(LoadingResult::ManifestLoaded));
                    }
                    else
                    {
                        ON_CALL(*this, LoadAsset(Matcher<Containers::Scene&>(_), Matcher<const AZStd::string&>(_), Matcher<const Uuid&>(_), Matcher<RequestingApplication>(_)))
                            .WillByDefault(Return(LoadingResult::AssetLoaded));
                    }
                    ON_CALL(*this, UpdateManifest(Matcher<Containers::Scene&>(_), Matcher<ManifestAction>(_), Matcher<RequestingApplication>(_)))
                        .WillByDefault(Return(ProcessingResult::Ignored));
                }
            };
        } // Events
    } // SceneAPI
} // AZ
