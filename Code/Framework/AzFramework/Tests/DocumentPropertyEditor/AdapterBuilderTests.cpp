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
    using AdapterBuilderDpeTests = DocumentPropertyEditorTestFixture;

    TEST_F(AdapterBuilderDpeTests, VisitSimpleStructure)
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

        Dom::Value domFromBuilder = builder.FinishAndTakeResult();

        /**
        Expect the following structure:
        <Adapter>
            <Row>
                <Label Value="label"/>
                <PropertyEditor type="TextEditor" attr=2 Value="lorem ipsum"/>
            </Row>
        </Adapter>
        */
        Dom::Value expectedDom = Dom::Value::CreateNode(Nodes::Adapter::Name);
        Dom::Value row = Dom::Value::CreateNode(Nodes::Row::Name);
        Dom::Value label = Dom::Value::CreateNode(Nodes::Label::Name);
        label[Nodes::Label::Value.GetName()] = Dom::Value("label", true);
        row.ArrayPushBack(label);
        Dom::Value editor = Dom::Value::CreateNode(Nodes::PropertyEditor::Name);
        editor[Nodes::PropertyEditor::Type.GetName()] = Dom::Value("TextEditor", true);
        editor["attr"] = 2;
        editor[Nodes::PropertyEditor::Value.GetName()] = Dom::Value("lorem ipsum", true);
        row.ArrayPushBack(editor);
        expectedDom.ArrayPushBack(row);

        EXPECT_TRUE(Dom::Utils::DeepCompareIsEqual(expectedDom, domFromBuilder));
    }

    TEST_F(AdapterBuilderDpeTests, VisitNestedRows)
    {
        AdapterBuilder builder;
        builder.BeginAdapter();
        Dom::Value expectedDom = Dom::Value::CreateNode(Nodes::Adapter::Name);
        for (int i = 0; i < 2; ++i)
        {
            builder.BeginRow();
            Dom::Value ri = Dom::Value::CreateNode(Nodes::Row::Name);
            for (int j = 0; j < 2; ++j)
            {
                builder.BeginRow();
                Dom::Value rj = Dom::Value::CreateNode(Nodes::Row::Name);
                for (int k = 0; k < 3; ++k)
                {
                    builder.BeginRow();
                    Dom::Value rk = Dom::Value::CreateNode(Nodes::Row::Name);
                    rj.ArrayPushBack(rk);
                    builder.EndRow();
                }
                ri.ArrayPushBack(rj);
                builder.EndRow();
            }
            expectedDom.ArrayPushBack(ri);
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
        Dom::Value domFromBuilder = builder.FinishAndTakeResult();
        EXPECT_TRUE(Dom::Utils::DeepCompareIsEqual(expectedDom, domFromBuilder));
    }
}
