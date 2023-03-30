/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabTestFixture.h>

#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;

    class PrefabUndoComponentPropertyTestFixture
        : public PrefabTestFixture
    {
    protected:
        inline static const char* WheelEntityName = "WheelEntity";
        inline static const char* CarPrefabName = "CarPrefab";
        inline static const char* SuperCarPrefabName = "SuperCarPrefab";

        //! Contains entity info of the created entity.
        struct EntityInfo
        {
            AZ::EntityId m_entityId;
            EntityAlias m_entityAlias;
        };

        //! Contains instance info of the created prefab instance.
        struct InstanceInfo
        {
            AZ::EntityId m_containerEntityId;
            InstanceAlias m_instanceAlias;
        };

        //! Contains a change patch to the property value.
        struct PropertyChangePatch
        {
            AZ::Dom::Path m_pathToProperty; // relative to entity, so no entity alias path is included.
            AZ::Dom::Value m_propertyValue;
        };

        void SetUpEditorFixtureImpl() override;

        //! Creates entities and prefabs for testing. SuperCar prefab owns a Car prefab. Car prefab owns a Wheel entity.
        //! Returns true if the entities and prefabs are created successfully.
        //! @{
        bool CreateWheelEntityHierarchy(EntityInfo& wheelEntityInfo);
        bool CreateCarPrefabHierarchy(InstanceInfo& carInstanceInfo, EntityInfo& wheelEntityInfo);
        bool CreateSuperCarPrefabHierarchy(InstanceInfo& superCarInstanceInfo, InstanceInfo& carInstanceInfo, EntityInfo& wheelEntityInfo);
        //! @}

        //! Generates property change patch for various properties.
        //! The data will be used in configuring undo nodes.
        //! @{
        PropertyChangePatch MakeTransformTranslationPropertyChangePatch(const AZ::EntityId entityId, const AZ::Vector3& translation);
        PropertyChangePatch MakeTransformStaticPropertyChangePatch(const AZ::EntityId entityId, const float isStatic);
        //! @}

        //! Helper function to convert AZ::Dom::Value to PrefabDom (rapidjson::Document).
        void ConvertToPrefabDomValue(PrefabDom& outputDom, const AZ::Dom::Value& domValue);

        PrefabOverridePublicInterface* m_prefabOverridePublicInterface = nullptr;
        PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
    };
} // namespace UnitTest
