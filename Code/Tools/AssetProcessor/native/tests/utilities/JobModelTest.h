/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <native/resourcecompiler/JobsModel.h>
#include <native/tests/AssetProcessorTest.h>
#include <QCoreApplication>

class UnitTestJobModel 
    : public AssetProcessor::JobsModel
{
public:
    ~UnitTestJobModel() override = default;
    friend class GTEST_TEST_CLASS_NAME_(JobModelUnitTests, Test_RemoveMiddleJob);
    friend class GTEST_TEST_CLASS_NAME_(JobModelUnitTests, Test_RemoveFirstJob);
    friend class GTEST_TEST_CLASS_NAME_(JobModelUnitTests, Test_RemoveLastJob);
    friend class GTEST_TEST_CLASS_NAME_(JobModelUnitTests, Test_RemoveAllJobsBySource);
    friend class GTEST_TEST_CLASS_NAME_(JobModelUnitTests, Test_RemoveAllJobsBySourceFolder);
    friend class JobModelUnitTests;
};

class JobModelUnitTests
    : public AssetProcessor::AssetProcessorTest
{
public:
    void SetUp() override;
    void TearDown() override;

    void VerifyModel();

    UnitTestJobModel* m_unitTestJobModel;
};

