/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/PrefabTestData.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>

namespace UnitTest
{
    namespace PrefabTestDomUtils
    {
        using namespace AzToolsFramework::Prefab;

        inline static const char* ComponentsValueName = "Components";
        inline static const char* ComponentIdName = "Id";
        inline static const char* EntitiesValueName = "Entities";
        inline static const char* EntityNameValueName = "Name";
        inline static const char* BoolPropertyName = "BoolProperty";

        inline PrefabDomPath GetPrefabDomEntitiesPath()
        {
            return PrefabDomPath()
                .Append(EntitiesValueName);
        };

        inline PrefabDomPath GetPrefabDomEntityPath(
            const EntityAlias& entityAlias)
        {
            return GetPrefabDomEntitiesPath()
                .Append(entityAlias.c_str(), static_cast<rapidjson::SizeType>(entityAlias.length()));
        };

        inline PrefabDomPath GetPrefabDomEntityNamePath(
            const EntityAlias& entityAlias)
        {
            return GetPrefabDomEntityPath(entityAlias)
                .Append(EntityNameValueName);
        };

        inline PrefabDomPath GetPrefabDomComponentsPath(const EntityAlias& entityAlias)
        {
            return GetPrefabDomEntityPath(entityAlias).Append(ComponentsValueName);
        };

        inline PrefabDomPath GetPrefabDomInstancesPath()
        {
            return PrefabDomPath()
                .Append(PrefabDomUtils::InstancesName);
        };

        inline PrefabDomPath GetPrefabDomInstancePath(
            const InstanceAlias& instanceAlias)
        {
            return GetPrefabDomInstancesPath().Append(instanceAlias.c_str(), static_cast<rapidjson::SizeType>(instanceAlias.length()));
        };

        inline PrefabDomPath GetPrefabDomInstancePath(
            const char* instanceName)
        {
            return GetPrefabDomInstancesPath().Append(instanceName);
        };

        inline PrefabDomPath GetPrefabDomSourcePath(
            const char* instanceName)
        {
            return GetPrefabDomInstancePath(instanceName).Append(PrefabDomUtils::SourceName);
        };

        inline PrefabDomPath GetPrefabDomPatchesPath(
            const char* instanceName)
        {
            return GetPrefabDomInstancePath(instanceName).Append(PrefabDomUtils::PatchesName);
        };

        inline const PrefabDomValue* GetPrefabDomComponents(
            const PrefabDom& prefabDom,
            const EntityAlias& entityAlias)
        {
            return PrefabTestDomUtils::GetPrefabDomComponentsPath(entityAlias).Get(prefabDom);
        }

        inline const PrefabDomValue* GetPrefabDomInstance(
            const PrefabDom& prefabDom,
            const InstanceAlias& instanceAlias)
        {
            return PrefabTestDomUtils::GetPrefabDomInstancePath(instanceAlias).Get(prefabDom);
        }

        inline const PrefabDomValue* GetPrefabDomEntity(
            const PrefabDom& prefabDom,
            const EntityAlias& entityAlias)
        {
            return PrefabTestDomUtils::GetPrefabDomEntityPath(entityAlias).Get(prefabDom);
        }

        inline const PrefabDomValue* GetPrefabDomEntityName(
            const PrefabDom& prefabDom,
            const EntityAlias& entityAlias)
        {
            return PrefabTestDomUtils::GetPrefabDomEntityNamePath(entityAlias).Get(prefabDom);
        }

        void SetPrefabDomInstance(
            PrefabDom& prefabDom,
            const char* instanceName,
            const char* source,
            const PrefabDomValue& patches);

        void ValidateInstances(
            TemplateId templateId,
            const PrefabDomValue& expectedContent,
            const PrefabDomPath& contentPath,
            bool isContentAnInstance = false,
            bool shouldCompareContainerEntities = true);

        PrefabDom CreatePrefabDom();
        PrefabDom CreatePrefabDom(const AZStd::vector<InstanceData>& instancesData);

        /**
         * Validates that the entities with the given entity aliases are present in the given prefab DOM.
         */
        void ValidatePrefabDomEntities(const AZStd::vector<EntityAlias>& entityAliases,
            PrefabDom& prefabDom);

        /**
         * Extracts the DOM of the instances using the given instance aliases from the prefab DOM and 
         * validates that they match with the expectedNestedInstanceDom.
         */
        void ValidatePrefabDomInstances(const AZStd::vector<InstanceAlias>& instanceAliases,
            const PrefabDom& prefabDom,
            const PrefabDom& expectedNestedInstanceDom,
            bool shouldCompareContainerEntities = true);

        void ComparePrefabDoms(PrefabDomValueConstReference valueA, PrefabDomValueConstReference valueB, bool shouldCompareLinkIds = true,
            bool shouldCompareContainerEntities = true);
        void ComparePrefabDomValues(PrefabDomValueConstReference valueA, PrefabDomValueConstReference valueB);

        void ValidateEntitiesOfInstances(
            AzToolsFramework::Prefab::TemplateId templateId,
            const AzToolsFramework::Prefab::PrefabDom& expectedPrefabDom,
            const AZStd::vector<EntityAlias>& entityAliases);

        void ValidateNestedInstancesOfInstances(
            AzToolsFramework::Prefab::TemplateId templateId,
            const AzToolsFramework::Prefab::PrefabDom& expectedPrefabDom,
            const AZStd::vector<InstanceAlias>& nestedInstanceAliases);

        void ValidateComponentsDomHasId(const PrefabDomValue& componentsDom, AZ::u64 componentId);

        //! Serializes Dom into a string
        AZStd::string DomToString(const AzToolsFramework::Prefab::PrefabDom& dom);
    } 
}

