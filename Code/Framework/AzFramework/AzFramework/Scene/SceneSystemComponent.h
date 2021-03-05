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

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/map.h>
#include <AzFramework/Scene/SceneSystemBus.h>
#include <AzFramework/Entity/EntityContext.h>

namespace AzFramework
{
    class SceneSystemComponent
        : public AZ::Component
        , public SceneSystemRequestBus::Handler
    {
    public:

        AZ_COMPONENT(SceneSystemComponent, "{7AC53AF0-BE1A-437C-BE3E-4D6A998DA945}", AZ::Component);

        SceneSystemComponent();
        ~SceneSystemComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // Component overrides
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        //////////////////////////////////////////////////////////////////////////
        // SceneSystemRequestsBus::Handler
        //////////////////////////////////////////////////////////////////////////
        AZ::Outcome<Scene*, AZStd::string> CreateScene(AZStd::string_view name) override;
        Scene* GetScene(AZStd::string_view name) override;
        AZStd::vector<Scene*> GetAllScenes() override;
        bool RemoveScene(AZStd::string_view name) override;
        bool SetSceneForEntityContextId(EntityContextId entityContextId, Scene* scene) override;
        bool RemoveSceneForEntityContextId(EntityContextId entityContextId, Scene* scene) override;
        Scene* GetSceneFromEntityContextId(EntityContextId entityContextId) override;

    private:

        AZ_DISABLE_COPY(SceneSystemComponent);

        // Container of scene in order of creation
        AZStd::vector<AZStd::unique_ptr<Scene>> m_scenes;

        // Map of entity context Ids to scenes. Using a vector because lookups will be common, but the size will be small.
        AZStd::vector<AZStd::pair<EntityContextId, Scene*>> m_entityContextToScenes;
    };
}