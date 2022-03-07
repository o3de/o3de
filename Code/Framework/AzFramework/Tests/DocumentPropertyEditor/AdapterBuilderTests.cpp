/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/DocumentPropertyEditor/DocumentPropertyEditorFixture.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/std/any.h>

namespace AZ::DocumentPropertyEditor::Tests
{
    using AdapterBuilderTests = DocumentPropertyEditorTestFixture;

    TEST_F(AdapterBuilderTests, VisitSimpleStructure)
    {
        AdapterBuilder builder;
        builder.BeginAdapter();
        builder.BeginRow();
        builder.Label("label");
        builder.BeginPropertyEditor("TextEditor", Dom::Value("lorem ipsum", true));
        builder.Attribute(AZ::Name("attr"), Dom::Value(2));
        builder.EndPropertyEditor();
        builder.EndRow();
        builder.EndAdapter();

        Dom::Value result = builder.FinishAndTakeResult();

        /**
        Expect the following structure:
        <Adapter>
            <Row>
                <Label>label</Label>
                <PropertyEditor type="TextEditor" attr=2>value</TextEditor>
            </Row>
        </Adapter>
        */
        Dom::Value expectedValue = Dom::Value::CreateNode("Adapter");
        Dom::Value row = Dom::Value::CreateNode("Row");
        Dom::Value label = Dom::Value::CreateNode("Label");
        label.SetNodeValue(Dom::Value("label", true));
        row.ArrayPushBack(label);
        Dom::Value editor = Dom::Value::CreateNode("PropertyEditor");
        editor["type"] = Dom::Value("TextEditor", true);
        editor["attr"] = 2;
        editor.SetNodeValue(Dom::Value("lorem ipsum", true));
        row.ArrayPushBack(editor);
        expectedValue.ArrayPushBack(row);

        EXPECT_TRUE(Dom::Utils::DeepCompareIsEqual(expectedValue, result));
    }

    TEST_F(AdapterBuilderTests, VisitNestedRows)
    {
        AdapterBuilder builder;
        builder.BeginAdapter();
        Dom::Value expectedValue = Dom::Value::CreateNode("Adapter");
        for (int i = 0; i < 2; ++i)
        {
            builder.BeginRow();
            Dom::Value ri = Dom::Value::CreateNode("Row");
            for (int j = 0; j < 2; ++j)
            {
                builder.BeginRow();
                Dom::Value rj = Dom::Value::CreateNode("Row");
                for (int k = 0; k < 3; ++k)
                {
                    builder.BeginRow();
                    Dom::Value rk = Dom::Value::CreateNode("Row");
                    rj.ArrayPushBack(rk);
                    builder.EndRow();
                }
                ri.ArrayPushBack(rj);
                builder.EndRow();
            }
            expectedValue.ArrayPushBack(ri);
            builder.EndRow();
        }
        builder.EndAdapter();

        /**
        Expect the following structure:
        <Adapter>
            <Row>
                <Row>
                    <Row />
                    <Row />
                    <Row />
                </Row>
                <Row>
                    <Row />
                    <Row />
                    <Row />
                </Row>
            </Row>
            <Row>
                <Row>
                    <Row />
                    <Row />
                    <Row />
                </Row>
                <Row>
                    <Row />
                    <Row />
                    <Row />
                </Row>
            </Row>
        </Adapter>
        */
        Dom::Value result = builder.FinishAndTakeResult();
        EXPECT_TRUE(Dom::Utils::DeepCompareIsEqual(expectedValue, result));
    }
}
