/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomUtils.h>
#include <AzCore/std/any.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <Tests/DocumentPropertyEditor/DocumentPropertyEditorFixture.h>

namespace AZ::DocumentPropertyEditor::Tests
{
    class SchemaDpeTests : public DocumentPropertyEditorTestFixture
    {
    public:
        static constexpr auto intAttr = AttributeDefinition<int>("intAttr");
        static constexpr auto doubleAttr = AttributeDefinition<double>("doubleAttr");
        static constexpr auto strAttr = AttributeDefinition<AZStd::string_view>("strAttr");
        static constexpr auto fnAttr = CallbackAttributeDefinition<int(int, int)>("fnAttr");
    };

    TEST_F(SchemaDpeTests, ValueToDom)
    {
        Dom::Value v;
        v = intAttr.ValueToDom(2);
        EXPECT_EQ(2, v.GetInt64());
        v = doubleAttr.ValueToDom(4.5);
        EXPECT_EQ(4.5, v.GetDouble());
        v = strAttr.ValueToDom("test string");
        EXPECT_EQ("test string", v.GetString());
    }

    TEST_F(SchemaDpeTests, InvokeCallback)
    {
        Dom::Value v = fnAttr.ValueToDom(
            [](int x, int y)
            {
                return x + y;
            });
        EXPECT_EQ(5, fnAttr.InvokeOnDomValue(v, 2, 3).GetValue());
        EXPECT_EQ(0, fnAttr.InvokeOnDomValue(v, 5, -5).GetValue());
        EXPECT_FALSE(fnAttr.InvokeOnDomValue(Dom::Value(), 1, 2).IsSuccess());
        EXPECT_FALSE(fnAttr
                         .InvokeOnDomValue(
                             Dom::Value::FromOpaqueValue(AZStd::any(
                                 AZStd::function<int(int)>([](int x)
                                 {
                                     return x;
                                 }))),
                             1, 2)
                         .IsSuccess());
    }
} // namespace AZ::DocumentPropertyEditor::Tests
