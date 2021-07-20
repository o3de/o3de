/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;
        namespace PrefabDomUtils
        {
            inline static const char* InstancesName = "Instances";
            inline static const char* PatchesName = "Patches";
            inline static const char* SourceName = "Source";
            inline static const char* LinkIdName = "LinkId";
            inline static const char* EntityIdName = "Id";
            inline static const char* EntitiesName = "Entities";
            inline static const char* ContainerEntityName = "ContainerEntity";

            /**
            * Find Prefab value from given parent value and target value's name.
            * @param parentValue A parent value in Prefab DOM.
            * @param valueName A name of child value of parentValue.
            * @return Reference to the child value of parentValue with name valueName if the child value exists.
            */
            PrefabDomValueReference FindPrefabDomValue(PrefabDomValue& parentValue, const char* valueName);
            PrefabDomValueConstReference FindPrefabDomValue(const PrefabDomValue& parentValue, const char* valueName);

            /**
            * Stores a valid Prefab Instance within a Prefab Dom. Useful for generating Templates
            * @param instance The instance to store
            * @param prefabDom The prefabDom that will be used to store the Instance data
            * @return bool on whether the operation succeeded
            */
            bool StoreInstanceInPrefabDom(const Instance& instance, PrefabDom& prefabDom);

            enum class LoadInstanceFlags : uint8_t
            {
                //! No flags used during the call to LoadInstanceFromPrefabDom.
                None = 0,
                //! By default entities will get a stable id when they're deserialized. In cases where the new entities need to be kept
                //! unique, e.g. when they are duplicates of live entities, this flag will assign them a random new id.
                AssignRandomEntityId = 1 << 0
            };
            AZ_DEFINE_ENUM_BITWISE_OPERATORS(LoadInstanceFlags)

            /**
            * Loads a valid Prefab Instance from a Prefab Dom. Useful for generating Instances.
            * @param instance The Instance to load.
            * @param prefabDom The prefabDom that will be used to load the Instance data.
            * @param shouldClearContainers Whether to clear containers in Instance while loading.
            * @return bool on whether the operation succeeded.
            */
            bool LoadInstanceFromPrefabDom(
                Instance& instance, const PrefabDom& prefabDom, LoadInstanceFlags flags = LoadInstanceFlags::None);

            /**
            * Loads a valid Prefab Instance from a Prefab Dom. Useful for generating Instances.
            * @param instance The Instance to load.
            * @param referencedAssets AZ::Assets discovered during json load are added to this list
            * @param prefabDom The prefabDom that will be used to load the Instance data.
            * @param shouldClearContainers Whether to clear containers in Instance while loading.
            * @return bool on whether the operation succeeded.
            */
            bool LoadInstanceFromPrefabDom(
                Instance& instance, const PrefabDom& prefabDom, AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets,
                LoadInstanceFlags flags = LoadInstanceFlags::None);

            /**
            * Loads a valid Prefab Instance from a Prefab Dom. Useful for generating Instances.
            * @param instance The Instance to load.
            * @param newlyAddedEntities The new instances added during deserializing the instance. These are the entities found
            *       in the prefabDom.
            * @param prefabDom The prefabDom that will be used to load the Instance data.
            * @param shouldClearContainers Whether to clear containers in Instance while loading.
            * @return bool on whether the operation succeeded.
            */
            bool LoadInstanceFromPrefabDom(
                Instance& instance, Instance::EntityList& newlyAddedEntities, const PrefabDom& prefabDom,
                LoadInstanceFlags flags = LoadInstanceFlags::None);

            inline PrefabDomPath GetPrefabDomInstancePath(const char* instanceName)
            {
                return PrefabDomPath()
                    .Append(InstancesName)
                    .Append(instanceName);
            };

            /**
             * Gets a set of all the template source paths in the given dom.
             * @param prefabDom The DOM to get the template source paths from.
             * @param[out] templateSourcePaths The set of template source paths to populate.
             */
            void GetTemplateSourcePaths(const PrefabDomValue& prefabDom, AZStd::unordered_set<AZ::IO::Path>& templateSourcePaths);

            /**
             * Gets the instances DOM value from the given prefab DOM.
             * 
             * @return the instances DOM value or AZStd::nullopt if it instances can't be found.
             */
            PrefabDomValueConstReference GetInstancesValue(const PrefabDomValue& prefabDom);

            /**
             * Prints the contents of the given prefab DOM value to the debug output console in a readable format.
             * @param printMessage The message that will be printed before printing the PrefabDomValue
             * @param prefabDomValue The DOM value to be printed. A 'PrefabDom' type can also be passed into this variable.
             */
            void PrintPrefabDomValue(
                [[maybe_unused]] const AZStd::string_view printMessage,
                [[maybe_unused]] const AzToolsFramework::Prefab::PrefabDomValue& prefabDomValue);

        } // namespace PrefabDomUtils
    } // namespace Prefab
} // namespace AzToolsFramework

