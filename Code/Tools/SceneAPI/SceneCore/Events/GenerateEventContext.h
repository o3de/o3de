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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ::SceneAPI::Containers { class Scene; }
namespace AZ::SceneAPI::DataTypes { class IGroup; }

namespace AZ::SceneAPI::Events
{
    class ExportProductList;

    // Signals the scene generation step is about to happen
    class PreGenerateEventContext
        : public ICallContext
    {
    public:
        AZ_RTTI(PreGenerateEventContext, "{0D1AB113-D35E-4C35-9820-E7B22F37D90C}", ICallContext)
        SCENE_CORE_API PreGenerateEventContext(Containers::Scene& scene, const char* platformIdentifier);

        SCENE_CORE_API Containers::Scene& GetScene() const;
        SCENE_CORE_API const char* GetPlatformIdentifier() const;

    private:
        Containers::Scene& m_scene;
        
        /**
        * The platform identifier is configured in the AssetProcessorPlatformConfig.ini and is data driven
        * it is generally a value like "pc" or "ios" or such.
        * this const char* points at memory owned by the caller but it will always survive for the duration of the call.
        */
        const char* m_platformIdentifier = nullptr;
    };

    // Signals that all appropriate objects should be generated into the Scene
    class GenerateEventContext
        : public ICallContext
    {
    public:
        AZ_RTTI(GenerateEventContext, "{B53CCBBF-965A-4709-AD33-AFD5F3AE8580}", ICallContext)

        SCENE_CORE_API GenerateEventContext(Containers::Scene& scene, const char* platformIdentifier);

        SCENE_CORE_API Containers::Scene& GetScene() const;
        SCENE_CORE_API const char* GetPlatformIdentifier() const;

    private:
        Containers::Scene& m_scene;
        
        /** 
        * The platform identifier is configured in the AssetProcessorPlatformConfig.ini and is data driven
        * it is generally a value like "pc" or "ios" or such.
        * this const char* points at memory owned by the caller but it will always survive for the duration of the call.
        */
        const char* m_platformIdentifier = nullptr;
    };

    // Signals that the generation step is complete
    class PostGenerateEventContext
        : public ICallContext
    {
    public:
        AZ_RTTI(PostGenerateEventContext, "{3EE65CBF-6C0E-425A-9ECC-3CC8FC4372F7}", ICallContext)

        SCENE_CORE_API PostGenerateEventContext(Containers::Scene& scene, const char* platformIdentifier);

        SCENE_CORE_API Containers::Scene& GetScene() const;
        SCENE_CORE_API const char* GetPlatformIdentifier() const;

    private:
        Containers::Scene& m_scene;
        
        /**
        * The platform identifier is configured in the AssetProcessorPlatformConfig.ini and is data driven
        * it is generally a value like "pc" or "ios" or such.
        * this const char* points at memory owned by the caller but it will always survive for the duration of the call.
        */
        const char* m_platformIdentifier = nullptr;
    };
} // namespace AZ::SceneAPI::Events
