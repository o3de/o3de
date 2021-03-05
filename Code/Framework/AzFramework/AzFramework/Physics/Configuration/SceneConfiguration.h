/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/Physics/World.h> //this will be removed with LYN-438.

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    //! Configuration object that contains data to setup a Scene.
    struct SceneConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(SceneConfiguration, "{4ABF9993-8E52-4E41-B38D-28FD569B4EAF}");
        static void Reflect(AZ::ReflectContext* context);

        static SceneConfiguration CreateDefault();

        // Legacy members Will be removed and replaced with LYN-438 work.
        Physics::WorldConfiguration m_legacyConfiguration;
        AZ::Crc32 m_legacyId; //use SceneConfiguration::m_SceneName instead

        AZStd::string m_sceneName = "DefaultScene"; //!< Name given to the scene.

        bool operator==(const SceneConfiguration& other) const;
        bool operator!=(const SceneConfiguration& other) const;
    };

    //! Alias for a list of SceneConfiguration objects, used for the creation of multiple Scenes at once.
    using SceneConfigurationList = AZStd::vector<SceneConfiguration>;
}
