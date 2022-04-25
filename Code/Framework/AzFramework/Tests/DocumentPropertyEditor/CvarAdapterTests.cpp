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

        void SetEntryValue(AZStd::string_view cvarName, Dom::Value value)
        {
            Dom::Value row = GetEntryRow(cvarName);
            ASSERT_FALSE(row.IsNull());
            auto result = Nodes::PropertyEditor::OnChanged.InvokeOnDomNode(row[1], value);
            EXPECT_TRUE(result.IsSuccess());
            AZ_Error("CvarAdapterDpeTests", result.IsSuccess(), "%s", result.GetError().c_str());
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
        Dom::Value value = GetEntryValue("dpe_TestVec2");
        EXPECT_EQ(static_cast<AZ::Vector2>(dpe_TestVec2), AZ::Vector2((float)value[0].GetDouble(), (float)value[1].GetDouble()));
        value[0].SetDouble(2.0);
        value[1].SetDouble(4.5);
        SetEntryValue("dpe_TestVec2", value);
        value = GetEntryValue("dpe_TestVec2");
        EXPECT_EQ(2.0, GetEntryValue("dpe_TestVec2")[0].GetDouble());
        EXPECT_EQ(4.5, GetEntryValue("dpe_TestVec2")[1].GetDouble());
        EXPECT_EQ(AZ::Vector2(2.0f, 4.5f), static_cast<AZ::Vector2>(dpe_TestVec2));
    }

    TEST_F(CvarAdapterDpeTests, Vec3Cvar)
    {
        Dom::Value value = GetEntryValue("dpe_TestVec3");
        EXPECT_EQ(
            static_cast<AZ::Vector3>(dpe_TestVec3),
            AZ::Vector3((float)value[0].GetDouble(), (float)value[1].GetDouble(), (float)value[2].GetDouble()));
        value[1].SetDouble(5.0);
        SetEntryValue("dpe_TestVec3", value);
        value = GetEntryValue("dpe_TestVec3");
        EXPECT_EQ(5.0, GetEntryValue("dpe_TestVec3")[1].GetDouble());
        EXPECT_EQ(5.f, static_cast<AZ::Vector3>(dpe_TestVec3).GetY());
    }

    TEST_F(CvarAdapterDpeTests, ColorCvar)
    {
        Dom::Value value = GetEntryValue("dpe_TestColor");
        EXPECT_EQ(
            static_cast<AZ::Color>(dpe_TestColor),
            AZ::Color((float)value[0].GetDouble(), (float)value[1].GetDouble(), (float)value[2].GetDouble(), (float)value[3].GetDouble()));
        value[0].SetDouble(0.0);
        value[1].SetDouble(1.0);
        value[2].SetDouble(0.5);
        value[3].SetDouble(0.25);
        SetEntryValue("dpe_TestColor", value);
        value = GetEntryValue("dpe_TestColor");
        EXPECT_EQ(0.0, GetEntryValue("dpe_TestColor")[0].GetDouble());
        EXPECT_EQ(1.0, GetEntryValue("dpe_TestColor")[1].GetDouble());
        EXPECT_EQ(0.5, GetEntryValue("dpe_TestColor")[2].GetDouble());
        EXPECT_EQ(0.25, GetEntryValue("dpe_TestColor")[3].GetDouble());
        EXPECT_EQ(AZ::Color(0.f, 1.f, 0.5f, 0.25f), static_cast<AZ::Color>(dpe_TestColor));
    }
} // namespace AZ::DocumentPropertyEditor::Tests
