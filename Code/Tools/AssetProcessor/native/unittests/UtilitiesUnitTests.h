/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef UTILITIESUNITTESTS_H
#define UTILITIESUNITTESTS_H

#include "UnitTestRunner.h"

class UtilitiesUnitTests
    : public UnitTestRun
{
public:
    virtual void StartTest() override;
    virtual int UnitTestPriority() const override { return -200; } // utilities should be done first since everything depends on them.
};

#endif // UTILITIESUNITTESTS_H

