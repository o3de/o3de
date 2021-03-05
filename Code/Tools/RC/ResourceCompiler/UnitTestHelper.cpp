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
#include "ResourceCompiler_precompiled.h"

#include "UnitTestHelper.h"
#include "IRCLog.h"

UnitTestHelper::UnitTestHelper()
    : m_testsPerformed(0)
    , m_testsSucceeded(0)
{
}

UnitTestHelper::~UnitTestHelper()
{
}

bool UnitTestHelper::TestBool(bool testValueIsTrue, const char* testValueStatement)
{
    ++m_testsPerformed;
    if (testValueIsTrue)
    {
        ++m_testsSucceeded;
    }
    else
    {
        // Set a breakpoint here if you are debugging unit test failure
        // Add callstack and other useful information here
        RCLogError("Unit test failed! Evaluated to false when true was expected. Statement is: %s", testValueStatement ? testValueStatement : "Unknown statement");
    }
    return testValueIsTrue;
}

unsigned int UnitTestHelper::GetTestsPerformedCount()
{
    return m_testsPerformed;
}

unsigned int UnitTestHelper::GetTestsSucceededCount()
{
    return m_testsSucceeded;
}

bool UnitTestHelper::AllUnitTestsPassed()
{
    return m_testsSucceeded == m_testsPerformed;
}
