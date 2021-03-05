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
#include "LmbrCentral_precompiled.h"
#include "LmbrCentralReflectionTest.h"
#include "Shape/EditorPolygonPrismShapeComponent.h"

namespace LmbrCentral
{
    // Serialized legacy EditorPolygonPrismShapeComponent v1.
    const char kEditorPolygonPrismComponentVersion1[] =
        R"DELIMITER(<ObjectStream version="1">
        <Class name="EditorPolygonPrismShapeComponent" field="element" version="1" type="{5368F204-FE6D-45C0-9A4F-0F933D90A785}">
            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                    <Class name="AZ::u64" field="Id" value="2508877310741125152" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
            </Class>
            <Class name="PolygonPrismCommon" field="Configuration" version="1" type="{BDB453DE-8A51-42D0-9237-13A9193BE724}">
                <Class name="AZStd::shared_ptr" field="PolygonPrism" type="{2E879A16-9143-5862-A5B3-EDED931C60BC}">
                    <Class name="PolygonPrism" field="element" version="1" type="{F01C8BDD-6F24-4344-8945-521A8750B30B}">
                        <Class name="float" field="Height" value="1.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                        <Class name="VertexContainer&lt;Vector2 &gt;" field="VertexContainer" type="{EBE98B36-0783-5226-9739-064BD41EBB52}">
                            <Class name="AZStd::vector" field="Vertices" type="{82AC1A71-2EA7-5FBC-9B3B-72B1CCFDD292}">
                                <Class name="Vector2" field="element" value="-0.5700000 -0.5700000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                <Class name="Vector2" field="element" value="0.5700000 -0.5700000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                <Class name="Vector2" field="element" value="0.5700000 0.5700000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                <Class name="Vector2" field="element" value="-0.5700000 0.5700000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                            </Class>
                        </Class>
                    </Class>
                </Class>
            </Class>
        </Class>
    </ObjectStream>)DELIMITER";

    class LoadEditorPolygonPrismShapeComponentFromVersion1
        : public LoadEditorComponentTest<EditorPolygonPrismShapeComponent>
    {
    protected:
        const char* GetSourceDataBuffer() const override { return kEditorPolygonPrismComponentVersion1; }
    };

    TEST_F(LoadEditorPolygonPrismShapeComponentFromVersion1, Application_IsRunning)
    {
        ASSERT_NE(GetApplication(), nullptr);
    }

    TEST_F(LoadEditorPolygonPrismShapeComponentFromVersion1, Components_Load)
    {
        EXPECT_NE(m_object.get(), nullptr);
    }

    TEST_F(LoadEditorPolygonPrismShapeComponentFromVersion1, EditorComponent_Found)
    {
        EXPECT_EQ(m_entity->GetComponents().size(), 2);
        EXPECT_NE(m_entity->FindComponent(m_object->GetId()), nullptr);
    }

    TEST_F(LoadEditorPolygonPrismShapeComponentFromVersion1, Height_MatchesSourceData)
    {
        AZ::ConstPolygonPrismPtr polygonPrism;
        PolygonPrismShapeComponentRequestBus::EventResult(polygonPrism, m_entity->GetId(), &PolygonPrismShapeComponentRequestBus::Events::GetPolygonPrism);

        EXPECT_FLOAT_EQ(polygonPrism->GetHeight(), 1.57f);
    }

    TEST_F(LoadEditorPolygonPrismShapeComponentFromVersion1, Vertices_MatchesSourceData)
    {
        AZ::ConstPolygonPrismPtr polygonPrism;
        PolygonPrismShapeComponentRequestBus::EventResult(polygonPrism, m_entity->GetId(), &PolygonPrismShapeComponentRequestBus::Events::GetPolygonPrism);

        AZStd::vector<AZ::Vector2> sourceVertices = 
        {
            AZ::Vector2(-0.57f, -0.57f),
            AZ::Vector2(0.57f, -0.57f),
            AZ::Vector2(0.57f, 0.57f),
            AZ::Vector2(-0.57f, 0.57f)
        };
        EXPECT_EQ(polygonPrism->m_vertexContainer.GetVertices(), sourceVertices);
    }
}

