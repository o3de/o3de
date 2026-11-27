/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <XR_Traits_Platform.h>

class XRTest
    : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    void SetupInternal();
    void TearDownInternal();
};

