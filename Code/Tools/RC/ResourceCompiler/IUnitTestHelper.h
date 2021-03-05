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
#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_IUNITTESTHELPER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_IUNITTESTHELPER_H
#pragma once

// IT IS RECOMMENDED TO USE THESE MACROS FOR UNIT TESTS INSTEAD OF THE METHODS LISTED BELOW
#define TEST_BOOL(statement) TestBool(statement, #statement)

class IUnitTestHelper
{
public:
    virtual ~IUnitTestHelper() {};

    // This tests a boolean value that should be true, the test will fail if it is false
    // It is recommended that you use the TEST_BOOL macro to auto-stringify the statement for easier identification when unit tests fail
    // Alternatively, specify your own testValueStatement to also help identify which unit test fails
    virtual bool TestBool(bool testValueIsTrue, const char* testValueStatement) = 0;
};


#endif // #ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_IUNITTESTHELPER_H
