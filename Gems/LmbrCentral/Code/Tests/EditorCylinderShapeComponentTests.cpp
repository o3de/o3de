/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LmbrCentral_precompiled.h"
#include "LmbrCentralReflectionTest.h"
#include "Shape/EditorCylinderShapeComponent.h"

namespace LmbrCentral
{
    // Serialized legacy EditorCylinderShapeComponent v1.
    const char kEditorCylinderComponentVersion1[] =
        R"DELIMITER(<ObjectStream version="1">
        <Class name="EditorCylinderShapeComponent" field="element" version="1" type="{D5FC4745-3C75-47D9-8C10-9F89502487DE}">
            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                    <Class name="AZ::u64" field="Id" value="2283148451428660584" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
            </Class>
            <Class name="CylinderShapeConfig" field="Configuration" version="1" type="{53254779-82F1-441E-9116-81E1FACFECF4}">
                <Class name="float" field="Height" value="1.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                <Class name="float" field="Radius" value="0.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
        </Class>
    </ObjectStream>)DELIMITER";

    class LoadEditorCylinderShapeComponentFromVersion1
        : public LoadEditorComponentTest<EditorCylinderShapeComponent>
    {
    protected:
        const char* GetSourceDataBuffer() const override { return kEditorCylinderComponentVersion1; }
    };

    TEST_F(LoadEditorCylinderShapeComponentFromVersion1, Application_IsRunning)
    {
        ASSERT_NE(GetApplication(), nullptr);
    }

    TEST_F(LoadEditorCylinderShapeComponentFromVersion1, Components_Load)
    {
        EXPECT_NE(m_object.get(), nullptr);
    }

    TEST_F(LoadEditorCylinderShapeComponentFromVersion1, EditorComponent_Found)
    {
        EXPECT_EQ(m_entity->GetComponents().size(), 2);
        EXPECT_NE(m_entity->FindComponent(m_object->GetId()), nullptr);
    }

    TEST_F(LoadEditorCylinderShapeComponentFromVersion1, Height_MatchesSourceData)
    {
        float height = 0.0f;
        CylinderShapeComponentRequestsBus::EventResult(
            height, m_entity->GetId(), &CylinderShapeComponentRequests::GetHeight);

        EXPECT_FLOAT_EQ(height, 1.57f);
    }

    TEST_F(LoadEditorCylinderShapeComponentFromVersion1, Radius_MatchesSourceData)
    {
        float radius = 0.0f;
        CylinderShapeComponentRequestsBus::EventResult(
            radius, m_entity->GetId(), &CylinderShapeComponentRequests::GetRadius);

        EXPECT_FLOAT_EQ(radius, 0.57f);
    }
}

