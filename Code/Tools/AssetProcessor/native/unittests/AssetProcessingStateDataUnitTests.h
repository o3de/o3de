/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

