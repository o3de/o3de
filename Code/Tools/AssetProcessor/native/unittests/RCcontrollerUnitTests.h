/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <native/unittests/AssetProcessorUnitTests.h>

namespace AssetProcessor
{
    class RCController;
    class RCJob;
    class RCJobListModel;
    class RCQueueSortModel;
}

class RCcontrollerUnitTests
    : public QObject
    , public UnitTest::AssetProcessorUnitTestBase
{
public:
    void SetUp() override;
    void TearDown() override;

protected:
    void FinishJob(AssetProcessor::RCJob* rcJob);
    void PrepareRCJobListModelTest(int& numJobs);
    void PrepareCompileGroupTests(const QStringList& tempJobNames, bool& gotCreated, bool& gotCompleted, AssetProcessor::NetworkRequestID& gotGroupID, AzFramework::AssetSystem::AssetStatus& gotStatus);
    void Reset();

    void ConnectCompileGroupSignalsAndSlots(bool& gotCreated, bool& gotCompleted, AssetProcessor::NetworkRequestID& gotGroupID, AzFramework::AssetSystem::AssetStatus& gotStatus);
    void ConnectJobSignalsAndSlots(bool& allJobsCompleted, AssetProcessor::JobEntry& entry);

    AZStd::unique_ptr<AssetProcessor::RCController> m_rcController;
    AssetBuilderSDK::AssetBuilderDesc m_assetBuilderDesc;
    AssetProcessor::RCJobListModel* m_rcJobListModel;
    AssetProcessor::RCQueueSortModel* m_rcQueueSortModel;

    QList<AssetProcessor::RCJob*> m_createdJobs;

};
