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
    struct JobDependencySubIdTest : AssetManagerTestingBase
    {
        void CreateTestData(AZ::u64 hashA, AZ::u64 hashB, bool useSubId);
        void RunTest(bool firstProductChanged, bool secondProductChanged);

        AZ::Uuid m_assetType = AZ::Uuid::CreateName("test");
        AZ::IO::Path m_parentFile, m_childFile;
    };
}
