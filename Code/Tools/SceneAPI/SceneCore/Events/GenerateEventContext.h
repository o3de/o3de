/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    class GenerateEventBaseContext
        : public ICallContext
    {
    public:
        AZ_RTTI(GenerateEventBaseContext, "{1717EB67-33A1-4516-8167-746093F7AEB6}", ICallContext)

        SCENE_CORE_API GenerateEventBaseContext(Containers::Scene& scene, const char* platformIdentifier);

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

    // Signals the scene generation step is about to happen
    class PreGenerateEventContext
        : public GenerateEventBaseContext
    {
    public:
        AZ_RTTI(PreGenerateEventContext, "{0D1AB113-D35E-4C35-9820-E7B22F37D90C}", GenerateEventBaseContext)
        using GenerateEventBaseContext::GenerateEventBaseContext;
    };

    // Signals that new data such as procedurally generated objects should be added to the Scene
    class GenerateEventContext
        : public GenerateEventBaseContext
    {
    public:
        AZ_RTTI(GenerateEventContext, "{B53CCBBF-965A-4709-AD33-AFD5F3AE8580}", GenerateEventBaseContext)
        using GenerateEventBaseContext::GenerateEventBaseContext;
    };

    // Signals that new LODs should be added to the Scene
    class GenerateLODEventContext
        : public GenerateEventBaseContext
    {
    public:
        AZ_RTTI(GenerateLODEventContext, "{2E3A6B98-1409-4895-8092-B7F8A410EF0D}", GenerateEventBaseContext)
        using GenerateEventBaseContext::GenerateEventBaseContext;
    };

    // Signals that any new data, such as tangents and bitangents, should be added to the Scene
    class GenerateAdditionEventContext
        : public GenerateEventBaseContext
    {
    public:
        AZ_RTTI(GenerateAdditionEventContext, "{105106FE-9ED7-48E6-9EA8-C7268BE8C625}", GenerateEventBaseContext)
        using GenerateEventBaseContext::GenerateEventBaseContext;
    };

    // Signals that data simplification / complexity reduction should be run
    class GenerateSimplificationEventContext
        : public GenerateEventBaseContext
    {
    public:
        AZ_RTTI(GenerateSimplificationEventContext, "{77F44B7F-C5BC-4411-B53F-E4307691841B}", GenerateEventBaseContext)
        using GenerateEventBaseContext::GenerateEventBaseContext;
    };

    // Signals that the generation step is complete
    class PostGenerateEventContext
        : public GenerateEventBaseContext
    {
    public:
        AZ_RTTI(PostGenerateEventContext, "{3EE65CBF-6C0E-425A-9ECC-3CC8FC4372F7}", GenerateEventBaseContext)
        using GenerateEventBaseContext::GenerateEventBaseContext;
    };
} // namespace AZ::SceneAPI::Events
