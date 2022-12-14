/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <native/tests/assetmanager/AssetManagerTestingBase.h>

namespace UnitTests
{
    class IntermediateAssetTests
        : public AssetManagerTestingBase
        , protected AZ::Debug::TraceMessageBus::Handler
    {
    public:
        void SetUp() override;
        void TearDown() override;

    protected:
        bool OnPreAssert(const char*, int, const char*, const char* message) override;
        bool OnPreError(const char*, const char*, int, const char*, const char* message) override;

        void DeleteIntermediateTest(const char* fileToDelete);
        void IncorrectBuilderConfigurationTest(bool commonPlatform, AssetBuilderSDK::ProductOutputFlags flags);

        int m_expectedErrors = 0;
    };
} // namespace UnitTests
