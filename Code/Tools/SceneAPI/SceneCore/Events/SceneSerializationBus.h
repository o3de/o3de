/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace Events
        {
            //! EBus to deal with serialization to and from disks of scene and manifest files.
            class SceneSerialization
                : public AZ::EBusTraits
            {
            public:
                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
                
                // Use a mutex to lock the EBus in case tools are running on different threads.
                using MutexType = AZStd::recursive_mutex;

                virtual ~SceneSerialization() = 0;

                //! Loads a scene and its corresponding manifest if available, otherwise a new manifest
                //! is created.
                //! @param sceneFilePath The absolute or relative path to the scene file in the source folder.
                //! @param sceneSourceGuid The source uuid for the scene file. If a null-uuid is given LoadScene will attempt to query the Asset Processor for the uuid.
                //! @param watchFolder is the scan folder that it was found inside
                //! @return The loaded scene or null if the file couldn't be fully resolved or an error 
                //! occurred during loading.
                virtual AZStd::shared_ptr<Containers::Scene> LoadScene(const AZStd::string& sceneFilePath, Uuid sceneSourceGuid, const AZStd::string& watchFolder) = 0;

                //! The scene system caches loaded scenes. This checks if the given scene is valid and in the cache or not.
                //! @param sceneFilePath The absolute or relative path to the scene file in the source folder.
                //! @return True if the given scene is actively cached, false if not.
                virtual bool IsSceneCached(const AZStd::string& /*sceneFilePath*/) { return false; }
            };

            using SceneSerializationBus = AZ::EBus<SceneSerialization>;

            inline SceneSerialization::~SceneSerialization() = default;
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ
