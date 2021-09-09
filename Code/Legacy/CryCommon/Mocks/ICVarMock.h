/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IConsole.h>
#include <AzTest/AzTest.h>

#include <AzCore/std/function/function_template.h>

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
    MOCK_METHOD1(AddOnChangeFunctor, uint64(const AZStd::function<void()>& pChangeFunctor));
    MOCK_CONST_METHOD0(GetOnChangeCallback, ConsoleVarFunc());
    MOCK_CONST_METHOD0(GetRealIVal, int());
    MOCK_METHOD2(SetLimits, void(float, float));
    MOCK_METHOD2(GetLimits, void(float&, float&));
    MOCK_METHOD0(HasCustomLimits, bool());
    MOCK_METHOD1(SetDataProbeString, void(const char*));
};
