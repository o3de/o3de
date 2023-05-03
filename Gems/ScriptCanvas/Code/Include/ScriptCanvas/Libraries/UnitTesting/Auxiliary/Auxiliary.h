/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <ScriptCanvas/Data/Data.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    namespace UnitTesting
    {
        namespace Auxiliary
        {
            class EBusTraits
                : public AZ::EBusTraits
            {
            public:
                static void Reflect(AZ::ReflectContext* context);

                virtual const char* CStyleToCStyle(const char*) = 0;
                virtual AZStd::string CStyleToString(const char*) = 0;
                virtual AZStd::string_view CStyleToStringView(const char*) = 0;
                virtual AZ::u32 IntOne(AZ::u32) = 0;
                virtual AZ::u32 IntTwo(AZ::u32) = 0;
                virtual AZ::u32 IntThree(AZ::u32) = 0;
            };

            using EBus = AZ::EBus<EBusTraits>;

            class EBusHandler
                : public EBus::Handler
                , public AZ::BehaviorEBusHandler
            {
            public:
                AZ_EBUS_BEHAVIOR_BINDER
                (EBusHandler
                    , "{5168D163-AAB9-417D-9FD4-CE10541D51CE}"
                    , AZ::SystemAllocator
                    , CStyleToCStyle
                    , CStyleToString
                    , CStyleToStringView
                    , IntOne
                    , IntTwo
                    , IntThree);

                const char* CStyleToCStyle(const char* string) override
                {
                    const char* result = nullptr;
                    CallResult(result, FN_CStyleToCStyle, string);
                    return result;
                }

                AZStd::string CStyleToString(const char* string) override
                {
                    AZStd::string result;
                    CallResult(result, FN_CStyleToString, string);
                    return result;
                }

                AZStd::string_view CStyleToStringView(const char* string) override
                {
                    AZStd::string_view result;
                    CallResult(result, FN_CStyleToStringView, string);
                    return result;
                }

                AZ::u32 IntOne(AZ::u32 input) override
                {
                    AZ::u32 result;
                    CallResult(result, FN_IntOne, input);
                    return result;
                }

                AZ::u32 IntTwo(AZ::u32 input) override
                {
                    AZ::u32 result;
                    CallResult(result, FN_IntTwo, input);
                    return result;
                }

                AZ::u32 IntThree(AZ::u32 input) override
                {
                    AZ::u32 result;
                    CallResult(result, FN_IntThree, input);
                    return result;
                }
            };

            class StringConversion
            {
            public:
                AZ_TYPE_INFO(StringConversion, "{47A9CF0C-6F34-4E0C-B1F9-F908FC2B7388}");

                static const char* CStyleToCStyle(const char* input);

                static void Reflect(AZ::ReflectContext* reflection);
            };

            class TypeExposition
            {
            public:
                AZ_TYPE_INFO(TypeExposition, "{742F8581-B03E-42C2-A332-2A47C588BD1F}");

                static void Reflect(AZ::ReflectContext* reflection);

            private:
                AZStd::array<AZ::Vector3, 2> m_arrayVector3_2;
                AZ::Outcome<AZ::Vector3> m_outcomeVector3Void;
            };
        }
    }
}
