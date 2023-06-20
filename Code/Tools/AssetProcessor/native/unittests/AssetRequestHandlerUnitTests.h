/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <native/unittests/AssetProcessorUnitTests.h>
#endif

namespace AssetProcessor
{
    class MockAssetRequestHandler;
    class MockConnectionHandler;

    class AssetRequestHandlerUnitTests
        : public QObject
        , public UnitTest::AssetProcessorUnitTestBase
    {
        Q_OBJECT
    public:
        ~AssetRequestHandlerUnitTests();
    protected:
        void SetUp() override;
        void TearDown() override;

        AZStd::unique_ptr<MockAssetRequestHandler> m_requestHandler;

        AZStd::unique_ptr<AssetProcessor::MockConnectionHandler> m_connection;
        bool m_requestedCompileGroup = false;
        bool m_requestedAssetExists = false;
        QString m_platformSet;
        NetworkRequestID m_requestIdSet;
        QString m_searchTermSet;
    };
}
