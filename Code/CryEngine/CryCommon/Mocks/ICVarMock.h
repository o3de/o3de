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

#include <IConsole.h>
#include <AzTest/AzTest.h>

#include <SFunctor.h>

class CVarMock
    : public ICVar
{
public:
    MOCK_METHOD0(Release, void());
    MOCK_CONST_METHOD0(GetIVal, int());
    MOCK_CONST_METHOD0(GetI64Val, int64());
    MOCK_CONST_METHOD0(GetFVal, float());
    MOCK_CONST_METHOD0(GetString, const char*());
    MOCK_CONST_METHOD0(GetDataProbeString, const char*());
    MOCK_METHOD1(Set, void(const char*));
    MOCK_METHOD1(ForceSet, void(const char*));
    MOCK_METHOD1(Set, void(float));
    MOCK_METHOD1(Set, void(int));
    MOCK_METHOD1(ClearFlags, void(const int));
    MOCK_CONST_METHOD0(GetFlags, int());
    MOCK_METHOD1(SetFlags, int(const int));
    MOCK_METHOD0(GetType, int());
    MOCK_CONST_METHOD0(GetName, const char*());
    MOCK_METHOD0(GetHelp, const char*());
    MOCK_CONST_METHOD0(IsConstCVar, bool());
    MOCK_METHOD1(SetOnChangeCallback, void(ConsoleVarFunc));
    MOCK_METHOD1(AddOnChangeFunctor, uint64(const SFunctor& pChangeFunctor));
    MOCK_CONST_METHOD0(GetNumberOfOnChangeFunctors, uint64());
    MOCK_CONST_METHOD1(GetOnChangeFunctor, const SFunctor&(uint64));
    MOCK_METHOD1(RemoveOnChangeFunctor, bool(uint64));
    MOCK_CONST_METHOD0(GetOnChangeCallback, ConsoleVarFunc());
    MOCK_CONST_METHOD1(GetMemoryUsage, void(class ICrySizer* pSizer));
    MOCK_CONST_METHOD0(GetRealIVal, int());
    MOCK_METHOD2(SetLimits, void(float, float));
    MOCK_METHOD2(GetLimits, void(float&, float&));
    MOCK_METHOD0(HasCustomLimits, bool());
    MOCK_METHOD1(SetDataProbeString, void(const char*));
};
