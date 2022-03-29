/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/EBus.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Data/Data.h>

#include "UnitTestBusMacros.h"
#include "UnitTesting.h"

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    namespace UnitTesting
    {
        class BusTraits : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

            using BusIdType = AZ::Data::AssetId;

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
            
        };

        using Bus = AZ::EBus<BusTraits>;
    }
} 
