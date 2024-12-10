/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Budget.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Configuration.h>

ATOM_RPI_REFLECT_API AZ_DECLARE_BUDGET(RPI);

namespace UnitTest
{
    class RPITestFixture;
}

namespace AZ
{
    namespace RPI
    {
        class ATOM_RPI_REFLECT_API Validation
        {
            friend class UnitTest::RPITestFixture;
        public:
            static bool IsEnabled()
            {
                return s_isEnabled;
            }
        private:
            static void SetEnabled(bool enabled);

            static bool s_isEnabled;
        };

        class ATOM_RPI_REFLECT_API PassValidation
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
            static constexpr uint32_t SubPass = 3;
            static constexpr uint32_t Pass = 4;
            static constexpr uint32_t View = 5;
            static constexpr uint32_t Scene = 6;
            static constexpr uint32_t Bindless = 7;
        };
    }
}
