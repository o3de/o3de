/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Input/User/LocalUserId.h>

#include <SaveDataSystemComponent.h>

class SaveDataTest
    : public UnitTest::LeakDetectionFixture
{
public:
    static AzFramework::LocalUserId GetDefaultTestUserId();
protected:
    void SetUp() override;
    void TearDown() override;

    void SetupInternal();
    void TearDownInternal();

private:
    AZStd::unique_ptr<SaveData::SaveDataSystemComponent> m_saveDataSystemComponent;
};

