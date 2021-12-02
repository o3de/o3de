#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
            class SceneManifest;
        }
        namespace DataTypes
        {
            namespace Utilities
            {

                //! Checks if the given name is already in use by another manifest entry of a (derived) type.
                template<typename T>
                bool IsNameAvailable(const AZStd::string& name, const Containers::SceneManifest& manifest);
                //! Checks if the given name is already in use by another manifest entry of a (derived) type.
                SCENE_CORE_API bool IsNameAvailable(const AZStd::string& name, const Containers::SceneManifest& manifest, const Uuid& type);

                //! Creates a unique name for given (derived) type starting with the given base name.
                template<typename T>
                inline AZStd::string CreateUniqueName(const AZStd::string& baseName, const Containers::SceneManifest& manifest);
                //! Creates a unique name for given (derived) type starting with the given base name and specialized on the sub name.
                template<typename T>
                inline AZStd::string CreateUniqueName(const AZStd::string& baseName, const AZStd::string& subName, const Containers::SceneManifest& manifest);
                //! Creates a unique name for given (derived) type starting with the given base name.
                SCENE_CORE_API AZStd::string CreateUniqueName(const AZStd::string& baseName, const Containers::SceneManifest& manifest, const Uuid& type);
                //! Creates a unique name for given (derived) type starting with the given base name and specialized on the sub name.
                SCENE_CORE_API AZStd::string CreateUniqueName(const AZStd::string& baseName, const AZStd::string& subName, 
                    const Containers::SceneManifest& manifest, const Uuid& type);

                //! Creates a uuid that remains stable between runs. Use this to make sure that objects that are default generated get the same uuid
                //! when generated again between runs. Use this version if this is the only or primary object. Do not use this function to create
                //! a uuid for objects the user manually adds, which should use a random uuid.
                SCENE_CORE_API Uuid CreateStableUuid(const Containers::Scene& scene, const Uuid& typeId);
                //! Creates a uuid that remains stable between runs. Use this to make sure that objects that are default generated get the same uuid
                //! when generated again between runs. Use this version if there are multiple objects of the same type automatically generated that
                //! are not the primary object. For instance if there are multiple mesh groups, where some groups only have a single mesh and the remaining
                //! meshes go in the default mesh group, the default mesh group would use the previous CreateStableUuid, and the additional mesh groups
                //! can use this with the selected mesh as the sub id. Other alternatives might be all the selected nodes concatenated into a single string.
                SCENE_CORE_API Uuid CreateStableUuid(const Containers::Scene& scene, const Uuid& typeId, const AZStd::string& subId);
                //! Creates a uuid that remains stable between runs. Use this to make sure that objects that are default generated get the same uuid
                //! when generated again between runs. Use this version if there are multiple objects of the same type automatically generated that
                //! are not the primary object. For instance if there are multiple mesh groups, where some groups only have a single mesh and the remaining
                //! meshes go in the default mesh group, the default mesh group would use the previous CreateStableUuid, and the additional mesh groups
                //! can use this with the selected mesh as the sub id. Other alternatives might be all the selected nodes concatenated into a single string.
                SCENE_CORE_API Uuid CreateStableUuid(const Containers::Scene& scene, const Uuid& typeId, const char* subId);

            } // Utilities
        } // DataTypes
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.inl>
