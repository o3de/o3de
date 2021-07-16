/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef PLATFORMCONFIGURATIONUNITTESTS_H
#define PLATFORMCONFIGURATIONUNITTESTS_H

#include "UnitTestRunner.h"

class PlatformConfigurationTests
    : public UnitTestRun
{
public:
    virtual void StartTest() override;
    virtual int UnitTestPriority() const override { return -1; }
};

#endif // PLATFORMCONFIGURATIONUNITTESTS_H

