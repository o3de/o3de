/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    class Entity;
    struct Uuid;

    namespace SceneAPI
    {
        namespace SceneCore
        {
            namespace EntityConstructor
            {
                using EntityPointer = AZStd::unique_ptr<AZ::Entity, void(*)(AZ::Entity*)>;

                SCENE_CORE_API EntityPointer BuildEntity(const char* entityName, const AZ::Uuid& baseComponentType);
                SCENE_CORE_API Entity* BuildEntityRaw(const char* entityName, const AZ::Uuid& baseComponentType);
                SCENE_CORE_API Entity* BuildSceneSystemEntity();
            } // namespace EntityConstructor
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ
