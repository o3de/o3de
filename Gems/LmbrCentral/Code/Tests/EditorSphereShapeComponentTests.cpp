/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LmbrCentral_precompiled.h"
#include "LmbrCentralReflectionTest.h"
#include "Shape/EditorSphereShapeComponent.h"

namespace LmbrCentral
{
    // Serialized legacy EditorSphereShapeComponent v1.
    const char kEditorSphereComponentVersion1[] =
        R"DELIMITER(<ObjectStream version="1">
        <Class name="EditorSphereShapeComponent" field="element" version="1" type="{2EA56CBF-63C8-41D9-84D5-0EC2BECE748E}">
            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                    <Class name="AZ::u64" field="Id" value="11428802534905560348" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
            </Class>
            <Class name="SphereShapeConfig" field="Configuration" version="1" type="{4AADFD75-48A7-4F31-8F30-FE4505F09E35}">
                <Class name="float" field="Radius" value="0.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
        </Class>
    </ObjectStream>)DELIMITER";

    class LoadEditorSphereShapeComponentFromVersion1
        : public LoadEditorComponentTest<EditorSphereShapeComponent>
    {
    protected:
        const char* GetSourceDataBuffer() const override { return kEditorSphereComponentVersion1; }
    };

    TEST_F(LoadEditorSphereShapeComponentFromVersion1, Application_IsRunning)
    {
        ASSERT_NE(GetApplication(), nullptr);
    }

    TEST_F(LoadEditorSphereShapeComponentFromVersion1, Components_Load)
    {
        EXPECT_NE(m_object.get(), nullptr);
    }

    TEST_F(LoadEditorSphereShapeComponentFromVersion1, EditorComponent_Found)
    {
        EXPECT_EQ(m_entity->GetComponents().size(), 2);
        EXPECT_NE(m_entity->FindComponent(m_object->GetId()), nullptr);
    }

    TEST_F(LoadEditorSphereShapeComponentFromVersion1, Radius_MatchesSourceData)
    {
        float radius = 0.0f;
        SphereShapeComponentRequestsBus::EventResult(
            radius, m_entity->GetId(), &SphereShapeComponentRequests::GetRadius);

        EXPECT_FLOAT_EQ(radius, 0.57f);
    }
}
