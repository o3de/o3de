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
#include <AzCore/std/parallel/mutex.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <AzFramework/Entity/EntityContext.h>

namespace AzFramework
{
    class SceneSystemComponent
        : public AZ::Component
        , public SceneSystemInterface::Registrar
    {
    public:
        AZ_COMPONENT(SceneSystemComponent, "{7AC53AF0-BE1A-437C-BE3E-4D6A998DA945}", AZ::Component, ISceneSystem);

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
        // SceneSystemInterface overrides
        //////////////////////////////////////////////////////////////////////////
        AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> CreateScene(AZStd::string_view name) override;
        AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> CreateSceneWithParent(
            AZStd::string_view name, AZStd::shared_ptr<Scene> parent) override;
        [[nodiscard]] AZStd::shared_ptr<Scene> GetScene(AZStd::string_view name) override;
        void IterateActiveScenes(const ActiveIterationCallback& callback) override;
        void IterateZombieScenes(const ZombieIterationCallback& callback) override;
        bool RemoveScene(AZStd::string_view name) override;
        void ConnectToEvents(SceneEvent::Handler& handler) override;

    private:
        AZ_DISABLE_COPY(SceneSystemComponent);

        AZStd::vector<AZStd::shared_ptr<Scene>> m_activeScenes;
        AZStd::vector<AZStd::weak_ptr<Scene>> m_zombieScenes;
        // Using a mutex around the events as other threads may respond to a new/deleted scene by making
        // local updates and unregistering themselves. Since Scene is single threaded, no updates (other
        // then unregistering an event) should be done from other threads though.
        AZStd::recursive_mutex m_eventMutex;
        SceneEvent m_events;
    };
}
