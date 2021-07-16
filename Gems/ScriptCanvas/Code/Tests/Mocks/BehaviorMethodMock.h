/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzTest/AzTest.h>

#include <ScriptCanvas/Libraries/Core/Method.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    class BehaviorMethodMock : public AZ::BehaviorMethod
    {
    public:
        BehaviorMethodMock() : AZ::BehaviorMethod(nullptr) {};

        MOCK_CONST_METHOD3(Call, bool(AZ::BehaviorValueParameter*, unsigned int, AZ::BehaviorValueParameter*));
        MOCK_CONST_METHOD0(HasResult, bool());
        MOCK_CONST_METHOD0(IsMember, bool());
        MOCK_CONST_METHOD0(HasBusId, bool());
        MOCK_CONST_METHOD0(GetBusIdArgument, const AZ::BehaviorParameter* ());
        MOCK_METHOD3(OverrideParameterTraits, void(size_t, AZ::u32, AZ::u32));
        MOCK_CONST_METHOD0(GetNumArguments, size_t());
        MOCK_CONST_METHOD0(GetMinNumberOfArguments, size_t());
        MOCK_CONST_METHOD1(GetArgument, const AZ::BehaviorParameter*(size_t));
        MOCK_CONST_METHOD1(GetArgumentName, const AZStd::string*(size_t));
        MOCK_METHOD2(SetArgumentName, void(size_t, const AZStd::string&));
        MOCK_CONST_METHOD1(GetArgumentToolTip, const AZStd::string*(size_t));
        MOCK_METHOD2(SetArgumentToolTip, void(size_t, const AZStd::string&));
        MOCK_METHOD2(SetDefaultValue, void(size_t, AZ::BehaviorDefaultValuePtr));
        MOCK_CONST_METHOD1(GetDefaultValue, AZ::BehaviorDefaultValuePtr(size_t));
        MOCK_CONST_METHOD0(GetResult, const AZ::BehaviorParameter*());
    };
}
