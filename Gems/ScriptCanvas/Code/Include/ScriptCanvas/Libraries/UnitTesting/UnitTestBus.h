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

#include "UnitTesting.h"
#include "UnitTestBusMacros.h"
#include <AzCore/EBus/EBus.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    namespace UnitTesting
    {
        class BusTraits : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = ScriptCanvasId;

            virtual void AddFailure(const Report& report) = 0;
            
            virtual void AddSuccess(const Report& report) = 0;  
            
            virtual void Checkpoint(const Report& report) = 0;

            virtual void ExpectFalse(const bool value, const Report& report) = 0;

            virtual void ExpectTrue(const bool value, const Report& report) = 0;
            
            virtual void MarkComplete(const Report& report) = 0;

            /**
            \note the expect* nodes are designed this way, instead of with datums, so that the 
            compiled unit tests do not have to refer to datums at all
            */
            
            SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_DECLARATIONS(ExpectEqual);
                        
            SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_DECLARATIONS(ExpectNotEqual);
            
            SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_DECLARATIONS(ExpectGreaterThan);
            
            SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_DECLARATIONS(ExpectGreaterThanEqual);
            
            SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_DECLARATIONS(ExpectLessThan);
            
            SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_DECLARATIONS(ExpectLessThanEqual);
            
        }; // class BusTraits

        using Bus = AZ::EBus<BusTraits>;

    } // namespace UnitTest
} // namespace ScriptCanvas
