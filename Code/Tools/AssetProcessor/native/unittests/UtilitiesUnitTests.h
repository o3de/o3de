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

