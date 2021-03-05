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
#include "Shape/EditorCompoundShapeComponent.h"

namespace LmbrCentral
{
    // Serialized legacy EditorCompoundShapeComponent v1.
    const char kEditorCompoundComponentVersion1[] =
        R"DELIMITER(<ObjectStream version="3">
        <Class name="EditorCompoundShapeComponent" field="element" version="1" type="{837AA0DF-9C14-4311-8410-E7983E1F4B8D}">
            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                    <Class name="AZ::u64" field="Id" value="10467239283436660413" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
            </Class>
            <Class name="CompoundShapeConfiguration" field="Configuration" version="1" type="{4CEB4E5C-4CBD-4A84-88BA-87B23C103F3F}">
                <Class name="AZStd::list" field="Child Shape Entities" type="{BD759900-55F5-5687-A98B-FA0515FD4783}">
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="9" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="0" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="2" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="1" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                    <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                        <Class name="AZ::u64" field="id" value="0" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                    </Class>
                </Class>
            </Class>
        </Class>
    </ObjectStream>)DELIMITER";

    class LoadEditorCompoundShapeComponentFromVersion1
        : public LoadEditorComponentTest<EditorCompoundShapeComponent>
    {
    protected:
        const char* GetSourceDataBuffer() const override { return kEditorCompoundComponentVersion1; }
    };

    TEST_F(LoadEditorCompoundShapeComponentFromVersion1, Application_IsRunning)
    {
        ASSERT_NE(GetApplication(), nullptr);
    }

    TEST_F(LoadEditorCompoundShapeComponentFromVersion1, Components_Load)
    {
        EXPECT_NE(m_object.get(), nullptr);
    }

    TEST_F(LoadEditorCompoundShapeComponentFromVersion1, EditorComponent_Found)
    {
        EXPECT_EQ(m_entity->GetComponents().size(), 2);
        EXPECT_NE(m_entity->FindComponent(m_object->GetId()), nullptr);
    }

    TEST_F(LoadEditorCompoundShapeComponentFromVersion1, HasChildId_Succeeds)
    {
        const AZ::u64 potentialIds[] = { 9,0,2,1,0 };
        for (const AZ::u64 id : potentialIds)
        {
            bool hasReference = false;
            LmbrCentral::CompoundShapeComponentHierarchyRequestsBus::EventResult(hasReference, m_entity->GetId(),
                &LmbrCentral::CompoundShapeComponentHierarchyRequestsBus::Events::HasChildId, AZ::EntityId(id));
            EXPECT_TRUE(hasReference);
        }
    }

    TEST_F(LoadEditorCompoundShapeComponentFromVersion1, HasChildId_Fails)
    {
        const AZ::u64 potentialIds[] = { 8,6,7,5,3 };
        for (const AZ::u64 id : potentialIds)
        {
            bool hasReference = false;
            LmbrCentral::CompoundShapeComponentHierarchyRequestsBus::EventResult(hasReference, m_entity->GetId(),
                &LmbrCentral::CompoundShapeComponentHierarchyRequestsBus::Events::HasChildId, AZ::EntityId(id));
            EXPECT_FALSE(hasReference);
        }
    }

    TEST_F(LoadEditorCompoundShapeComponentFromVersion1, ValidateChildIds_Succeeds)
    {
        bool valid = false;
        LmbrCentral::CompoundShapeComponentHierarchyRequestsBus::EventResult(valid, m_entity->GetId(),
            &LmbrCentral::CompoundShapeComponentHierarchyRequestsBus::Events::ValidateChildIds);
        EXPECT_TRUE(valid);
    }
}