/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include "UnitTestRunner.h"
#endif

namespace AssetProcessor
{
    class AssetDatabaseConnection;
}

class AssetProcessingStateDataUnitTest
    : public UnitTestRun
{
    Q_OBJECT
public:
    void AssetProcessingStateDataTest();
    void ExistenceTest(AssetProcessor::AssetDatabaseConnection* stateData);
    void DataTest(AssetProcessor::AssetDatabaseConnection* stateData);
    void BuilderInfoTest(AssetProcessor::AssetDatabaseConnection* stateData);
    void SourceDependencyTest(AssetProcessor::AssetDatabaseConnection* stateData);
    void SourceFingerprintTest(AssetProcessor::AssetDatabaseConnection* stateData);

    virtual void StartTest() override;
    virtual int UnitTestPriority() const override { return -10; } // other classes depend on this one

Q_SIGNALS:
public Q_SLOTS:

private:
};

