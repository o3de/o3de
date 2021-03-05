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

