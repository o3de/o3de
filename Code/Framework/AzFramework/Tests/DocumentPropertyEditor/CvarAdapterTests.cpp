/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/Console.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/IConsoleTypes.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/CvarAdapter.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <Tests/DocumentPropertyEditor/DocumentPropertyEditorFixture.h>

namespace AZ::DocumentPropertyEditor::Tests
{
    AZ_CVAR(int32_t, dpe_TestInt, 1, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "DPE test int32");
    AZ_CVAR(uint8_t, dpe_TestUint, 22, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "DPE test uint8");
    AZ_CVAR(double, dpe_TestDbl, 4.0, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "DPE test double");
    AZ_CVAR(CVarFixedString, dpe_TestString, "test", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "DPE test string");
    AZ_CVAR(bool, dpe_TestBool, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "DPE test bool");
    AZ_CVAR(AZ::Vector2, dpe_TestVec2, AZ::Vector2::CreateZero(), nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "DPE test vec2");
    AZ_CVAR(AZ::Vector3, dpe_TestVec3, AZ::Vector3::CreateOne(), nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "DPE test vec3");
    AZ_CVAR(AZ::Color, dpe_TestColor, AZ::Color(), nullptr, ConsoleFunctorFlags::DontReplicate, "DPE test color");

    class CvarAdapterDpeTests : public DocumentPropertyEditorTestFixture
    {
    public:
        void SetUp() override
        {
            DocumentPropertyEditorTestFixture::SetUp();
            m_console = AZStd::make_unique<AZ::Console>();
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());
            m_adapter = AZStd::make_unique<CvarAdapter>();
        }

        void TearDown() override
        {
            m_adapter.reset();
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console.reset();
            DocumentPropertyEditorTestFixture::TearDown();
        }

        Dom::Value GetEntryRow(AZStd::string_view cvarName)
        {
            Dom::Value rows = m_adapter->GetContents();
            for (auto it = rows.ArrayBegin(); it != rows.ArrayEnd(); ++it)
            {
                Dom::Value label = (*it)[0][Nodes::Label::Value.GetName()];
                if (label.GetString() == cvarName)
                {
                    return *it;
                }
            }
            return {};
        }

        Dom::Value GetEntryValue(AZStd::string_view cvarName)
        {
            Dom::Value row = GetEntryRow(cvarName);
            EXPECT_FALSE(row.IsNull());
            return row[1][Nodes::PropertyEditor::Value.GetName()];
        }

        template <typename T>
        T GetTypedEntryValue(AZStd::string_view cvarName)
        {
            return Dom::Utils::ValueToType<T>(GetEntryValue(cvarName)).value();
        }

        void SetEntryValue(AZStd::string_view cvarName, Dom::Value value)
        {
            Dom::Value row = GetEntryRow(cvarName);
            ASSERT_FALSE(row.IsNull());
            auto result = Nodes::PropertyEditor::OnChanged.InvokeOnDomNode(row[1], value, Nodes::PropertyEditor::ValueChangeType::FinishedEdit);
            EXPECT_TRUE(result.IsSuccess());
            AZ_Error("CvarAdapterDpeTests", result.IsSuccess(), "%s", result.GetError().c_str());
        }

        template <typename T>
        void SetTypedEntryValue(AZStd::string_view cvarName, const T& value)
        {
            SetEntryValue(cvarName, Dom::Utils::ValueFromType(value));
        }

        AZStd::unique_ptr<AZ::Console> m_console;
        AZStd::unique_ptr<CvarAdapter> m_adapter;
    };

    TEST_F(CvarAdapterDpeTests, IntCvar)
    {
        EXPECT_EQ(static_cast<int32_t>(dpe_TestInt), GetEntryValue("dpe_TestInt").GetInt64());
        SetEntryValue("dpe_TestInt", Dom::Value(42));
        EXPECT_EQ(42, dpe_TestInt);
        EXPECT_EQ(42, GetEntryValue("dpe_TestInt").GetInt64());
    }

    TEST_F(CvarAdapterDpeTests, UintCvar)
    {
        EXPECT_EQ(static_cast<uint8_t>(dpe_TestUint), GetEntryValue("dpe_TestUint").GetUint64());
        SetEntryValue("dpe_TestUint", Dom::Value(42));
        EXPECT_EQ(42, dpe_TestUint);
        EXPECT_EQ(42, GetEntryValue("dpe_TestUint").GetUint64());
    }

    TEST_F(CvarAdapterDpeTests, DoubleCvar)
    {
        EXPECT_EQ(static_cast<double>(dpe_TestDbl), GetEntryValue("dpe_TestDbl").GetDouble());
        SetEntryValue("dpe_TestDbl", Dom::Value(6.28));
        EXPECT_EQ(6.28, dpe_TestDbl);
        EXPECT_EQ(6.28, GetEntryValue("dpe_TestDbl").GetDouble());
    }

    TEST_F(CvarAdapterDpeTests, StringCvar)
    {
        EXPECT_EQ(static_cast<CVarFixedString>(dpe_TestString), GetEntryValue("dpe_TestString").GetString());
        SetEntryValue("dpe_TestString", Dom::Value("new string", true));
        EXPECT_EQ("new string", static_cast<CVarFixedString>(dpe_TestString));
        EXPECT_EQ("new string", GetEntryValue("dpe_TestString").GetString());
    }

    TEST_F(CvarAdapterDpeTests, BoolCvar)
    {
        EXPECT_EQ(static_cast<bool>(dpe_TestBool), GetEntryValue("dpe_TestBool").GetBool());
        SetEntryValue("dpe_TestBool", Dom::Value(true));
        EXPECT_EQ(true, static_cast<bool>(dpe_TestBool));
        EXPECT_EQ(true, GetEntryValue("dpe_TestBool").GetBool());
    }

    TEST_F(CvarAdapterDpeTests, Vec2Cvar)
    {
        AZ::Vector2 value = GetTypedEntryValue<AZ::Vector2>("dpe_TestVec2");
        EXPECT_EQ(static_cast<AZ::Vector2>(dpe_TestVec2), value);
        const AZ::Vector2 newValue(2.0, 4.5);
        SetTypedEntryValue("dpe_TestVec2", newValue);
        value = GetTypedEntryValue<AZ::Vector2>("dpe_TestVec2");
        EXPECT_EQ(newValue, value);
        EXPECT_EQ(newValue, static_cast<AZ::Vector2>(dpe_TestVec2));
    }

    TEST_F(CvarAdapterDpeTests, Vec3Cvar)
    {
        AZ::Vector3 value = GetTypedEntryValue<AZ::Vector3>("dpe_TestVec3");
        EXPECT_EQ(static_cast<AZ::Vector3>(dpe_TestVec3), value);
        const AZ::Vector3 newValue(1.0, 0.0, 2.25);
        SetTypedEntryValue("dpe_TestVec3", newValue);
        value = GetTypedEntryValue<AZ::Vector3>("dpe_TestVec3");
        EXPECT_EQ(newValue, value);
        EXPECT_EQ(newValue, static_cast<AZ::Vector3>(dpe_TestVec3));
    }

    TEST_F(CvarAdapterDpeTests, ColorCvar)
    {
        AZ::Color value = GetTypedEntryValue<AZ::Color>("dpe_TestColor");
        EXPECT_EQ(static_cast<AZ::Color>(dpe_TestColor), value);
        const AZ::Color newValue(0.0f, 1.0f, 0.5f, 0.25f);
        SetTypedEntryValue("dpe_TestColor", newValue);
        value = GetTypedEntryValue<AZ::Color>("dpe_TestColor");
        EXPECT_EQ(newValue, value);
        EXPECT_EQ(newValue, static_cast<AZ::Color>(dpe_TestColor));
    }
} // namespace AZ::DocumentPropertyEditor::Tests
