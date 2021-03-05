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

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <Atom/RHI.Reflect/Base.h>

namespace UnitTest
{
    class RPITestFixture;
}

namespace AZ
{
    namespace RPI
    {
        class Validation
        {
            friend class UnitTest::RPITestFixture;
        public:
            static bool IsEnabled()
            {
                return s_isEnabled;
            }
        private:
            static bool s_isEnabled;
        };

        class PassValidation
        {
        public:
            static constexpr bool IsEnabled()
            {
                return RHI::BuildOptions::IsDebugBuild || RHI::BuildOptions::IsProfileBuild;
            }
        };

        template <typename T>
        using Ptr = RHI::Ptr<T>;

        template <typename T>
        using ConstPtr = RHI::ConstPtr<T>;

        namespace SrgBindingSlot
        {
            // Values have to match BindingSlots in SrgSemantics.azsli
            static constexpr uint32_t Draw = 0;
            static constexpr uint32_t Object = 1;
            static constexpr uint32_t Material = 2;
            static constexpr uint32_t Pass = 4;
        };
    }
}
