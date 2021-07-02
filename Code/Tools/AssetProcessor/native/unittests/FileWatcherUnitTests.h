/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef FILEMONITORUNITTESTS_H
#define FILEMONITORUNITTESTS_H
#include "UnitTestRunner.h"

class FileWatcherUnitTestRunner
    : public UnitTestRun
{
public:
    virtual void StartTest() override;
};

#endif // FILEMONITORUNITTESTS_H
