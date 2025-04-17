/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PythonTestingUtility.h"
#include "PythonTraceMessageSink.h"
#include <EditorPythonBindings/PythonCommon.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <EditorPythonBindings/CustomTypeBindingBus.h>
#include <Source/PythonSystemComponent.h>
#include <Source/PythonReflectionComponent.h>
#include <Source/PythonProxyObject.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/MathUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    template <typename T>
    struct CustomType
    {
        CustomType() = default;
        CustomType(T value)
        {
            m_value = value;
        }
        T m_value = {};
    };

    struct MyCustomData
    {
        AZ::s32 s32Field = -32;
        AZ::u32 u32Field = 32;
        AZ::s16 s16Field = -16;
        AZ::u16 u16Field = 16;

        bool Compare(const MyCustomData& other) const
        {
            return other.s32Field == s32Field
                && other.u32Field == u32Field
                && other.s16Field == s16Field
                && other.u16Field == u16Field;
        }
    };

    AZ_TYPE_INFO_SPECIALIZE(CustomType<int>, "{78BFA28F-7FF3-4DC6-B9E9-2DF158E6496B}");
    AZ_TYPE_INFO_SPECIALIZE(CustomType<float>, "{4B71C5C7-6947-4510-88A6-87F9F975F9CB}");
    AZ_TYPE_INFO_SPECIALIZE(CustomType<AZStd::string>, "{61ED57E0-50B2-4AD7-997A-FD343A964C49}");
    AZ_TYPE_INFO_SPECIALIZE(CustomType<MyCustomData>, "{839E35B3-14EF-4776-A5A1-C7B914374A66}");
}

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // test class/struts
    struct CustomTypeHandlerTester final
    {
        AZ_TYPE_INFO(CustomTypeHandlerTester, "{C59220A9-1479-434C-BBBD-4262090507FA}");

        AZ::CustomType<int> CreateCustomTypeInt(int value)
        {
            return AZ::CustomType<int>(value);
        }

        int ReturnCustomTypeInt(const AZ::CustomType<int>& value) const
        {
            return value.m_value;
        }

        AZ::CustomType<float> CreateCustomTypeFloat(float value)
        {
            return AZ::CustomType<float>(value);
        }

        float ReturnCustomTypeFloat(const AZ::CustomType<float>& value) const
        {
            return value.m_value;
        }

        bool CompareCustomTypeFloatValues(const AZ::CustomType<float>& lhs, const AZ::CustomType<float>& rhs) const
        {
            return AZ::IsClose(lhs.m_value, rhs.m_value, std::numeric_limits<float>::epsilon());
        }

        AZ::CustomType<AZStd::string> CreateCustomTypeString(const AZStd::string& value)
        {
            return AZ::CustomType<AZStd::string>(value);
        }

        AZ::CustomType<AZStd::string> CombineCustomTypeString(const AZ::CustomType<AZStd::string>& lhs, const AZ::CustomType<AZStd::string>& rhs)
        {
            return AZ::CustomType<AZStd::string>(lhs.m_value + rhs.m_value);
        }

        AZStd::string ReturnCustomTypeString(const AZ::CustomType<AZStd::string>& value)
        {
            return value.m_value;
        }

        AZ::CustomType<AZ::MyCustomData> CreateCustomData(
            AZ::s32 s32Value,
            AZ::u32 u32Value,
            AZ::s16 s16Value,
            AZ::u16 u16Value)
        {
            auto value = AZ::CustomType<AZ::MyCustomData>();
            value.m_value.s32Field = s32Value;
            value.m_value.u32Field = u32Value;
            value.m_value.s16Field = s16Value;
            value.m_value.u16Field = u16Value;
            return value;
        }

        AZ::CustomType<AZ::MyCustomData> CombineCustomData(const AZ::CustomType<AZ::MyCustomData>& lhs, const AZ::CustomType<AZ::MyCustomData>& rhs)
        {
            AZ::CustomType<AZ::MyCustomData> combined;
            combined.m_value.s32Field = lhs.m_value.s32Field + rhs.m_value.s32Field;
            combined.m_value.u32Field = lhs.m_value.u32Field + rhs.m_value.u32Field;
            combined.m_value.s16Field = lhs.m_value.s16Field + rhs.m_value.s16Field;
            combined.m_value.u16Field = lhs.m_value.u16Field + rhs.m_value.u16Field;
            return combined;
        }

        void Reflect(AZ::ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<CustomTypeHandlerTester>("CustomTypeHandlerTester")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Method("CreateCustomTypeInt", &CustomTypeHandlerTester::CreateCustomTypeInt)
                    ->Method("ReturnCustomTypeInt", &CustomTypeHandlerTester::ReturnCustomTypeInt)
                    ->Method("CreateCustomTypeFloat", &CustomTypeHandlerTester::CreateCustomTypeFloat)
                    ->Method("ReturnCustomTypeFloat", &CustomTypeHandlerTester::ReturnCustomTypeFloat)
                    ->Method("CompareCustomTypeFloatValues", &CustomTypeHandlerTester::CompareCustomTypeFloatValues)
                    ->Method("CreateCustomTypeString", &CustomTypeHandlerTester::CreateCustomTypeString)
                    ->Method("CombineCustomTypeString", &CustomTypeHandlerTester::CombineCustomTypeString)
                    ->Method("ReturnCustomTypeString", &CustomTypeHandlerTester::ReturnCustomTypeString)
                    ->Method("CreateCustomData", &CustomTypeHandlerTester::CreateCustomData)
                    ->Method("CombineCustomData", &CustomTypeHandlerTester::CombineCustomData)
                    ;
            }

        }
    };

    struct CustomTypeBindingNotificationBusHandler final
        : public EditorPythonBindings::CustomTypeBindingNotificationBus::MultiHandler
    {
        using Handle = EditorPythonBindings::CustomTypeBindingNotifications::ValueHandle;
        constexpr static Handle NoAllocation { ~0LL };

        AZStd::unordered_map<void*, AZ::TypeId> m_allocationMap;

        CustomTypeBindingNotificationBusHandler()
        {
            BusConnect(azrtti_typeid<AZ::CustomType<int>>());
            BusConnect(azrtti_typeid<AZ::CustomType<float>>());
            BusConnect(azrtti_typeid<AZ::CustomType<AZStd::string>>());
            BusConnect(azrtti_typeid<AZ::CustomType<AZ::MyCustomData>>());
        }

        ~CustomTypeBindingNotificationBusHandler()
        {
            BusDisconnect();
        }

        using AllocationHandle = EditorPythonBindings::CustomTypeBindingNotifications::AllocationHandle;
        AllocationHandle AllocateDefault() override
        {
            AZ::BehaviorObject behaviorObject;
            const AZ::TypeId& typeId = *EditorPythonBindings::CustomTypeBindingNotificationBus::GetCurrentBusId();
            if (typeId == azrtti_typeid<AZ::CustomType<int>>())
            {
                behaviorObject.m_address = azmalloc(sizeof(AZ::CustomType<int>));
                behaviorObject.m_typeId = typeId;
                m_allocationMap[behaviorObject.m_address] = behaviorObject.m_typeId;
                return { {reinterpret_cast<Handle>(behaviorObject.m_address), AZStd::move(behaviorObject)} };
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<float>>())
            {
                behaviorObject.m_address = azmalloc(sizeof(AZ::CustomType<float>));
                behaviorObject.m_typeId = typeId;
                m_allocationMap[behaviorObject.m_address] = behaviorObject.m_typeId;
                return { {reinterpret_cast<Handle>(behaviorObject.m_address), AZStd::move(behaviorObject)} };
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<AZStd::string>>())
            {
                behaviorObject.m_address = new AZ::CustomType<AZStd::string>();
                behaviorObject.m_typeId = typeId;
                m_allocationMap[behaviorObject.m_address] = behaviorObject.m_typeId;
                return { {reinterpret_cast<Handle>(behaviorObject.m_address), AZStd::move(behaviorObject)} };
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<AZ::MyCustomData>>())
            {
                behaviorObject.m_address = azmalloc(sizeof(AZ::CustomType<AZ::MyCustomData>));
                new (behaviorObject.m_address) AZ::CustomType<AZ::MyCustomData>();
                behaviorObject.m_typeId = typeId;
                m_allocationMap[behaviorObject.m_address] = behaviorObject.m_typeId;
                return { {reinterpret_cast<Handle>(behaviorObject.m_address), AZStd::move(behaviorObject)} };
            }
            return AZStd::nullopt;
        }

        AZStd::optional<ValueHandle> PythonToBehavior(
            PyObject* pyObj,
            [[maybe_unused]] AZ::BehaviorParameter::Traits traits,
            AZ::BehaviorArgument& outValue) override
        {
            const AZ::TypeId& typeId = *EditorPythonBindings::CustomTypeBindingNotificationBus::GetCurrentBusId();
            if (typeId == azrtti_typeid<AZ::CustomType<int>>())
            {
                outValue.ConvertTo<AZ::CustomType<int>>();
                outValue.StoreInTempData<AZ::CustomType<int>>({ aznumeric_cast<int>(PyLong_AsLong(pyObj)) });
                return { NoAllocation };
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<float>>())
            {
                float floatValue = aznumeric_cast<float>(PyFloat_AsDouble(pyObj));
                outValue.ConvertTo<AZ::CustomType<float>>();
                outValue.StoreInTempData<AZ::CustomType<float>>({ floatValue });
                return { NoAllocation };
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<AZStd::string>>())
            {
                if (PyUnicode_Check(pyObj))
                {
                    Py_ssize_t pySize = 0;
                    const char* pyData = PyUnicode_AsUTF8AndSize(pyObj, &pySize);
                    if (pyData)
                    {
                        auto data = new AZ::CustomType<AZStd::string>();
                        data->m_value.assign(pyData, pyData + pySize);

                        outValue.ConvertTo<AZ::CustomType<AZStd::string>>();
                        outValue.m_value = data;
                        m_allocationMap[outValue.m_value] = typeId;
                        return { reinterpret_cast<Handle>(outValue.m_value) };
                    }
                    return { NoAllocation };
                }
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<AZ::MyCustomData>>())
            {
                if (PyTuple_Check(pyObj) && PyTuple_Size(pyObj) == 4)
                {
                    void* data = azmalloc(sizeof(AZ::CustomType<AZ::MyCustomData>));
                    new (data) AZ::CustomType<AZ::MyCustomData>();
                    m_allocationMap[data] = typeId;

                    AZ::CustomType<AZ::MyCustomData>* myData = reinterpret_cast<AZ::CustomType<AZ::MyCustomData>*>(data);
                    myData->m_value.s32Field = aznumeric_cast<AZ::s32>(PyLong_AsLong(PyTuple_GetItem(pyObj, 0)));
                    myData->m_value.u32Field = aznumeric_cast<AZ::u32>(PyLong_AsLong(PyTuple_GetItem(pyObj, 1)));
                    myData->m_value.s16Field = aznumeric_cast<AZ::s16>(PyLong_AsLong(PyTuple_GetItem(pyObj, 2)));
                    myData->m_value.u16Field = aznumeric_cast<AZ::u16>(PyLong_AsLong(PyTuple_GetItem(pyObj, 3)));

                    outValue.ConvertTo<AZ::CustomType<AZ::MyCustomData>>();
                    outValue.m_value = data;
                    return { reinterpret_cast<Handle>(data) };
                }
            }
            return AZStd::nullopt;
        }

        AZStd::optional<ValueHandle> BehaviorToPython(
            const AZ::BehaviorArgument& behaviorValue,
            PyObject*& outPyObj) override
        {
            const AZ::TypeId& typeId = *EditorPythonBindings::CustomTypeBindingNotificationBus::GetCurrentBusId();
            if (typeId == azrtti_typeid<AZ::CustomType<int>>())
            {
                AZ::CustomType<int>* value = behaviorValue.GetAsUnsafe<AZ::CustomType<int>>();
                outPyObj = PyLong_FromLong(value->m_value);
                return { NoAllocation };
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<float>>())
            {
                AZ::CustomType<float>* value = behaviorValue.GetAsUnsafe<AZ::CustomType<float>>();
                outPyObj = PyFloat_FromDouble(value->m_value);
                return { NoAllocation };
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<AZStd::string>>())
            {
                AZ::CustomType<AZStd::string>* value = behaviorValue.GetAsUnsafe<AZ::CustomType<AZStd::string>>();
                outPyObj = PyUnicode_FromString(value->m_value.c_str());
                return { NoAllocation };
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<AZ::MyCustomData>>())
            {
                AZ::CustomType<AZ::MyCustomData>* value = behaviorValue.GetAsUnsafe<AZ::CustomType<AZ::MyCustomData>>();
                outPyObj = PyTuple_New(4);
                PyTuple_SetItem(outPyObj, 0, PyLong_FromLong(value->m_value.s32Field));
                PyTuple_SetItem(outPyObj, 1, PyLong_FromLong(value->m_value.u32Field));
                PyTuple_SetItem(outPyObj, 2, PyLong_FromLong(value->m_value.s16Field));
                PyTuple_SetItem(outPyObj, 3, PyLong_FromLong(value->m_value.u16Field));
                return { NoAllocation };
            }
            return AZStd::nullopt;
        }

        bool CanConvertPythonToBehavior(
            [[maybe_unused]] AZ::BehaviorParameter::Traits traits,
            PyObject* pyObj) const override
        {
            const AZ::TypeId& typeId = *EditorPythonBindings::CustomTypeBindingNotificationBus::GetCurrentBusId();
            if (typeId == azrtti_typeid<AZ::CustomType<int>>())
            {
                return PyLong_Check(pyObj);
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<float>>())
            {
                return PyFloat_Check(pyObj);
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<AZStd::string>>())
            {
                return PyUnicode_Check(pyObj);
            }
            else if (typeId == azrtti_typeid<AZ::CustomType<AZ::MyCustomData>>())
            {
                return PyTuple_Check(pyObj);
            }
            return false;
        }

        void CleanUpValue(ValueHandle handle) override
        {
            auto handleEntry = m_allocationMap.find(reinterpret_cast<void*>(handle));
            if (handleEntry != m_allocationMap.end())
            {
                const AZ::TypeId& typeId = handleEntry->second;
                if (typeId == azrtti_typeid<AZ::CustomType<int>>())
                {
                    azfree(reinterpret_cast<void*>(handle));
                }
                else if (typeId == azrtti_typeid<AZ::CustomType<float>>())
                {
                    azfree(reinterpret_cast<void*>(handle));
                }
                else if (typeId == azrtti_typeid<AZ::CustomType<AZStd::string>>())
                {
                    delete reinterpret_cast<AZ::CustomType<AZStd::string>*>(handle);
                }
                else if (typeId == azrtti_typeid<AZ::CustomType<AZ::MyCustomData>>())
                {
                    azfree(reinterpret_cast<void*>(handle));
                }
                m_allocationMap.erase(handleEntry);
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fixtures

    struct CustomTypeHandlerTests
        : public PythonTestingFixture
    {
        void SetUp() override
        {
            PythonTestingFixture::SetUp();
            PythonTestingFixture::RegisterComponentDescriptors();
        }

        void TearDown() override
        {
            PythonTestingFixture::TearDown();
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // tests

    TEST_F(CustomTypeHandlerTests, CustomTypeHandler_ReturnsCustom_Works)
    {
        CustomTypeBindingNotificationBusHandler customTypeBindingNotificationBusHandler;

        CustomTypeHandlerTester customTypeHandlerTester;
        customTypeHandlerTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test
                tester = azlmbr.test.CustomTypeHandlerTester()
                customValue = tester.CreateCustomTypeInt(42)
                if (None == customValue):
                    raise RuntimeError('None == customValue')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        e.Deactivate();
    }

    TEST_F(CustomTypeHandlerTests, CustomTypeHandler_AcceptsCustom_Works)
    {
        CustomTypeBindingNotificationBusHandler customTypeBindingNotificationBusHandler;

        CustomTypeHandlerTester customTypeHandlerTester;
        customTypeHandlerTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test
                tester = azlmbr.test.CustomTypeHandlerTester()
                customValue = tester.CreateCustomTypeInt(42)
                value = tester.ReturnCustomTypeInt(customValue)
                if (value != 42):
                    raise RuntimeError('value != 42')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        e.Deactivate();
    }

    TEST_F(CustomTypeHandlerTests, CustomTypeHandler_CustomFloatValues_Works)
    {
        CustomTypeBindingNotificationBusHandler customTypeBindingNotificationBusHandler;

        CustomTypeHandlerTester customTypeHandlerTester;
        customTypeHandlerTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test
                tester = azlmbr.test.CustomTypeHandlerTester()
                lhsValue = tester.CreateCustomTypeFloat(42.0)
                rhsValue = tester.CreateCustomTypeFloat(tester.ReturnCustomTypeFloat(lhsValue))
                if (tester.CompareCustomTypeFloatValues(lhsValue,rhsValue) is False):
                    raise RuntimeError('tester.CompareCustomTypeFloatValues(lhsValue,rhsValue) is False')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        e.Deactivate();
    }

    TEST_F(CustomTypeHandlerTests, CustomTypeHandler_CustomStringValues_Works)
    {
        CustomTypeBindingNotificationBusHandler customTypeBindingNotificationBusHandler;

        CustomTypeHandlerTester customTypeHandlerTester;
        customTypeHandlerTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test
                tester = azlmbr.test.CustomTypeHandlerTester()
                babble = tester.CreateCustomTypeString('babble')
                fish = tester.CreateCustomTypeString('fish')
                babbleFish = tester.CombineCustomTypeString(babble, fish)
                if (tester.ReturnCustomTypeString(babbleFish) != 'babblefish'):
                    raise RuntimeError("tester.ReturnCustomTypeString(babbleFish) != 'babblefish'")
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        e.Deactivate();
    }

    TEST_F(CustomTypeHandlerTests, CustomTypeHandler_CustomDataValue_Works)
    {
        CustomTypeBindingNotificationBusHandler customTypeBindingNotificationBusHandler;

        CustomTypeHandlerTester customTypeHandlerTester;
        customTypeHandlerTester.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.test
                tester = azlmbr.test.CustomTypeHandlerTester()
                lhs = tester.CreateCustomData(-1, 1, -2, 2)
                rhs = tester.CreateCustomData(0, 0, 1, 1)
                outTuple = tester.CombineCustomData(lhs, rhs)
                if (outTuple[0] != -1 or outTuple[1] != 1 or outTuple[2] != -1 or outTuple[3] != 3):
                    raise RuntimeError("outTuple[0] != -1 or outTuple[1] != 1 or outTuple[2] != -2 or outTuple[3] != 2")
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }

        e.Deactivate();
    }
}
