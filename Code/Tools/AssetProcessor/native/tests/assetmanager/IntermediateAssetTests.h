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
    {
    public:
        void SetUp() override;
        void TearDown() override;

    protected:
        void DeleteIntermediateTest(const char* fileToDelete);
        void IncorrectBuilderConfigurationTest(bool commonPlatform, AssetBuilderSDK::ProductOutputFlags flags);
    };
} // namespace UnitTests
