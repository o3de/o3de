/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#endif
#include <native/utilities/BatchApplicationManager.h>

namespace UnitTestUtils
{
    class AssertAbsorber;
}

namespace AssetProcessor
{
    class MockAssetDatabaseRequestsHandler;
}

namespace UnitTest
{
    class AssetProcessorUnitTestAppManager
        : public BatchApplicationManager
    {
    public:
        explicit AssetProcessorUnitTestAppManager(int* argc, char*** argv);

        bool PrepareForTests();

        AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_platformConfig;
        AZStd::unique_ptr<ConnectionManager> m_connectionManager;
    };

    class AssetProcessorUnitTestBase
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        AssetProcessorUnitTestBase();
        ~AssetProcessorUnitTestBase();

    protected:
        void SetUp() override;
        void TearDown() override;

        AZStd::unique_ptr<AssetProcessorUnitTestAppManager> m_appManager;
        AZStd::unique_ptr<AssetProcessor::MockAssetDatabaseRequestsHandler> m_assetDatabaseRequestsHandler;
        AZStd::unique_ptr<UnitTestUtils::AssertAbsorber> m_errorAbsorber;
    };
}
