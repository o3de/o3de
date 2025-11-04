/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EditorPythonBindings/PythonCommon.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include "PythonTraceMessageSink.h"
#include "PythonTestingUtility.h"

#include <Source/PythonSystemComponent.h>
#include <Source/PythonReflectionComponent.h>
#include <Source/PythonMarshalComponent.h>
#include <Source/PythonProxyObject.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/std/hash.h>

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // test class/structs

    struct MyCustomType
    {
        AZ_TYPE_INFO(MyCustomType, "{E4BE9816-E3E0-49EA-99B0-D72403461548}");

    public:
        AZ::u8 m_data;

        void SetData(AZ::u8 v)
        {
            m_data = v;
        }

        AZ::u8 GetData() const
        {
            return m_data;
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MyCustomType>()
                    ->Version(1)
                    ->Field("data", &MyCustomType::m_data)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<MyCustomType>("MyCustomType")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test.pair")
                    ->Method("set_data", &MyCustomType::SetData)
                    ->Method("get_data", &MyCustomType::GetData)
                    ;
            }
        }
    };
}

// AZStd::hash specialization for UnitTest::MyCustomType, required by BehaviorContext for AZStd::pair with custom types.
template<>
struct AZStd::hash<UnitTest::MyCustomType>
{
    typedef UnitTest::MyCustomType    argument_type;
    typedef AZStd::size_t       result_type;
    constexpr result_type operator()(const argument_type& value) const
    {
        return AZStd::hash<AZ::u8>()(value.m_data);
    }
};
