/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LmbrCentralReflectionTest.h"
#include "Shape/EditorCapsuleShapeComponent.h"

namespace LmbrCentral
{
    // Serialized legacy EditorCapsuleShapeComponent v1.
    const char kEditorCapsuleComponentVersion1[] =
        R"DELIMITER(<ObjectStream version="1">
        <Class name="EditorCapsuleShapeComponent" field="element" version="1" type="{06B6C9BE-3648-4DA2-9892-755636EF6E19}">
            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                    <Class name="AZ::u64" field="Id" value="10467239283436660413" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
            </Class>
            <Class name="CapsuleShapeConfig" field="Configuration" version="1" type="{00931AEB-2AD8-42CE-B1DC-FA4332F51501}">
                <Class name="float" field="Height" value="0.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                <Class name="float" field="Radius" value="1.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
        </Class>
    </ObjectStream>)DELIMITER";

    class LoadEditorCapsuleShapeComponentFromVersion1
        : public LoadEditorComponentTest<EditorCapsuleShapeComponent>
    {
    protected:
        const char* GetSourceDataBuffer() const override { return kEditorCapsuleComponentVersion1; }
    };

    TEST_F(LoadEditorCapsuleShapeComponentFromVersion1, Application_IsRunning)
    {
        ASSERT_NE(GetApplication(), nullptr);
    }

    TEST_F(LoadEditorCapsuleShapeComponentFromVersion1, Components_Load)
    {
        EXPECT_NE(m_object.get(), nullptr);
    }

    TEST_F(LoadEditorCapsuleShapeComponentFromVersion1, EditorComponent_Found)
    {
        EXPECT_EQ(m_entity->GetComponents().size(), 2);
        EXPECT_NE(m_entity->FindComponent(m_object->GetId()), nullptr);
    }

    TEST_F(LoadEditorCapsuleShapeComponentFromVersion1, Height_MatchesSourceData)
    {
        float height = 0.0f;
        CapsuleShapeComponentRequestsBus::EventResult(
            height, m_entity->GetId(), &CapsuleShapeComponentRequests::GetHeight);

        EXPECT_FLOAT_EQ(height, 0.57f);
    }

    TEST_F(LoadEditorCapsuleShapeComponentFromVersion1, Radius_MatchesSourceData)
    {
        float radius = 0.0f;
        CapsuleShapeComponentRequestsBus::EventResult(
            radius, m_entity->GetId(), &CapsuleShapeComponentRequests::GetRadius);

        EXPECT_FLOAT_EQ(radius, 1.57f);
    }
}

