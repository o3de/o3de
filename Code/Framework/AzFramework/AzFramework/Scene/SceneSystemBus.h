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

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Entity/EntityContext.h>

namespace AzFramework
{
    // Forward declarations
    class Scene;

    //! Interface used to create, get, or destroy scenes.
    class SceneSystemRequests
        : public AZ::EBusTraits
    {
    public:

        virtual ~SceneSystemRequests() = default;

        //! Single handler policy since there should only be one instance of this system component.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Creates a scene with a given name.
        //!  - If there is already a scene with the provided name this will return AZ::Failure(). 
        //!  - If isDefault is set to true and there is already a default scene, the default scene will be switched to this one. 
        virtual AZ::Outcome<Scene*, AZStd::string> CreateScene(AZStd::string_view name) = 0;

        //! Gets a scene with a given name
        //!  - If a scene does not exist with the given name, nullptr is returned.
        virtual Scene* GetScene(AZStd::string_view name) = 0;

        //! Gets all the scenes that currently exist.
        virtual AZStd::vector<Scene*> GetAllScenes() = 0;

        //! Remove a scene with a given name and return if the operation was successful.
        //!  - If the removed scene is the default scene, there will no longer be a default scene.
        virtual bool RemoveScene(AZStd::string_view name) = 0;

        //! Add a mapping from the provided EntityContextId to a Scene
        //! - If a scene is already associated with this EntityContextId, nothing is changed and false is returned.
        virtual bool SetSceneForEntityContextId(EntityContextId entityContextId, Scene* scene) = 0;

        //! Remove a mapping from the provided EntityContextId to a Scene
        //!  - If no scene is found from the provided EntityContextId, false is returned.
        virtual bool RemoveSceneForEntityContextId(EntityContextId entityContextId, Scene* scene) = 0;

        //! Get the scene associated with an EntityContextId
        //!  - If no scene is found for the provided EntityContextId, nullptr is returned.
        virtual Scene* GetSceneFromEntityContextId(EntityContextId entityContextId) = 0;
    };

    using SceneSystemRequestBus = AZ::EBus<SceneSystemRequests>;

    //! Interface used for notifications from the scene system
    class SceneSystemNotifications
        : public AZ::EBusTraits
    {
    public:

        virtual ~SceneSystemNotifications() = default;

        //! There can be multiple listeners to changes in the scene system.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Called when a scene has been created.
        virtual void SceneCreated(Scene& /*scene*/) {};

        //! Called just before a scene is removed.
        virtual void SceneAboutToBeRemoved(Scene& /*scene*/) {};

    };

    using SceneSystemNotificationBus = AZ::EBus<SceneSystemNotifications>;

    //! Interface used for notifications about individual scenes
    class SceneNotifications
        : public AZ::EBusTraits
    {
    public:

        virtual ~SceneNotifications() = default;

        //! There can be multiple listeners to changes in the scene system.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Bus is listened to using the pointer of the scene
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        //! Specifies that events are addressed by the pointer to the scene
        using BusIdType = Scene*;

        //! Called just before a scene is removed.
        virtual void SceneAboutToBeRemoved() {};

        //! Called when an entity context is mapped to this scene.
        virtual void EntityContextMapped(EntityContextId /*entityContextId*/) {};

        //! Called when an entity context is unmapped from this scene.
        virtual void EntityContextUnmapped(EntityContextId /*entityContextId*/) {};
    };

    using SceneNotificationBus = AZ::EBus<SceneNotifications>;

} // AzFramework