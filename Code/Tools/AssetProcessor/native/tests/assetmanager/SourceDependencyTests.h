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
    class SourceDependencyTests : public AssetManagerTestingBase
    {
    public:
        void SetUp() override;

        AssetProcessor::IUuidRequests* m_uuidInterface{};
    };
} // namespace UnitTests
