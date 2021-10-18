/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzTest/AzTest.h>

#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace AzToolsFramework
{
    class EditorFocusModeFixture
        : public UnitTest::ToolsApplicationFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        void GenerateTestHierarchy();
        AZ::EntityId CreateEditorEntity(const char* name, AZ::EntityId parentId);

        AZStd::unordered_map<AZStd::string, AZ::EntityId> m_entityMap;

        ContainerEntityInterface* m_containerEntityInterface = nullptr;
        FocusModeInterface* m_focusModeInterface = nullptr;

    public:
        AzToolsFramework::EntityIdList GetSelectedEntities();

        AzFramework::EntityContextId m_editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::CameraState m_cameraState;

        inline static const AZ::Vector3 CameraPosition = AZ::Vector3(10.0f, 15.0f, 10.0f);

        inline static const char* CityEntityName = "City";
        inline static const char* StreetEntityName = "Street";
        inline static const char* CarEntityName = "Car";
        inline static const char* SportsCarEntityName = "SportsCar";
        inline static const char* Passenger1EntityName = "Passenger1";
        inline static const char* Passenger2EntityName = "Passenger2";

        inline static AZ::Vector3 WorldCarEntityPosition = AZ::Vector3(5.0f, 15.0f, 0.0f);
    };

}
