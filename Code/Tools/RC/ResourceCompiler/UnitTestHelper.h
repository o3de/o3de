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
#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_UNITTESTHELPER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_UNITTESTHELPER_H
#pragma once

#include "IUnitTestHelper.h"

class UnitTestHelper
    : public IUnitTestHelper
{
public:
    UnitTestHelper();
    ~UnitTestHelper();

    bool TestBool(bool testValueIsTrue, const char* testValueStatement) override;
    unsigned int GetTestsPerformedCount();
    unsigned int GetTestsSucceededCount();
    bool AllUnitTestsPassed();
private:
    unsigned int m_testsPerformed;
    unsigned int m_testsSucceeded;
};


#endif // #ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_UNITTESTHELPER_H
