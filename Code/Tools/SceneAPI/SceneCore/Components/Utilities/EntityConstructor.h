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
