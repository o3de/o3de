/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string_view.h>

namespace AzFramework
{
    class Scene final
    {
    public:
        AZ_TYPE_INFO(Scene, "{DB449BB3-7A95-434D-BC61-47ACBB1F3436}");
        AZ_CLASS_ALLOCATOR(Scene, AZ::SystemAllocator);

        friend class ISceneSystem;

        constexpr static AZStd::string_view MainSceneName = "Main";
        constexpr static AZStd::string_view EditorMainSceneName = "Editor";

        enum class RemovalEventType
        {
            Zombified, // The scene has be marked for destruction and is no longer visible in the scene system.
            Destroyed, // The scene has been destroyed.
        };
        using RemovalEvent = AZ::Event<Scene&, RemovalEventType>;
        enum class SubsystemEventType
        {
            Added,
            Removed
        };
        using SubsystemEvent = AZ::Event<Scene&, SubsystemEventType, const AZ::TypeId&>;

        explicit Scene(AZStd::string name);
        Scene(AZStd::string name, AZStd::shared_ptr<Scene> parent);
        ~Scene();

        [[nodiscard]] const AZStd::string& GetName() const;

        [[nodiscard]] const AZStd::shared_ptr<Scene>& GetParent();
        [[nodiscard]] AZStd::shared_ptr<const Scene> GetParent() const;

        [[nodiscard]] bool IsAlive() const;

        void ConnectToEvents(RemovalEvent::Handler& handler);
        void ConnectToEvents(SubsystemEvent::Handler& handler);
        
        // Set the instance of a subsystem associated with this scene.
        template <typename T>
        bool SetSubsystem(T&& system);

        // Unset the instance of a subsystem associated with this scene.
        template <typename T>
        bool UnsetSubsystem();

        // Unset the instance of the exact system associated with this scene.
        // Use this to make sure the expected instance is removed or to make sure type deduction is done in the same way as during setting.
        template<typename T>
        bool UnsetSubsystem(const T& system);

        // Get the instance of a subsystem associated with this scene. This call will also look in parent scenes if not found on the target
        // scene. Returns a pointer to the subsystem if found, otherwise returns a nullptr.
        [[nodiscard]] AZStd::any* FindSubsystem(const AZ::TypeId& typeId);
        // Get the instance of a subsystem associated with this scene. This call will also look in parent scenes if not found on the target
        // scene. Returns a pointer to the subsystem if found, otherwise returns a nullptr.
        [[nodiscard]] const AZStd::any* FindSubsystem(const AZ::TypeId& typeId) const;
        // Get the instance of a subsystem associated with this scene. This call will also look in parent scenes if not found on the target
        // scene. Returns a pointer to the subsystem if found, otherwise returns a nullptr.
        template <typename T>
        [[nodiscard]] T* FindSubsystem();
        // Get the instance of a subsystem associated with this scene. This call will also look in parent scenes if not found on the target
        // scene. Returns a pointer to the subsystem if found, otherwise returns a nullptr.
        template<typename T>
        [[nodiscard]] const T* FindSubsystem() const;

        // Get the instance of a subsystem associated with this scene. This call will only look in the selected scene. Returns a pointer to
        // the subsystem if found, otherwise returns a nullptr.
        [[nodiscard]] AZStd::any* FindSubsystemInScene(const AZ::TypeId& typeId);
        // Get the instance of a subsystem associated with this scene. This call will only look in the selected scene. Returns a pointer to
        // the subsystem if found, otherwise returns a nullptr.
        [[nodiscard]] const AZStd::any* FindSubsystemInScene(const AZ::TypeId& typeId) const;
        // Get the instance of a subsystem associated with this scene. This call will only look in the selected scene. Returns a pointer to
        // the subsystem if found, otherwise returns a nullptr.
        template<typename T>
        [[nodiscard]] T* FindSubsystemInScene();
        // Get the instance of a subsystem associated with this scene. This call will only look in the selected scene. Returns a pointer to
        // the subsystem if found, otherwise returns a nullptr.
        template<typename T>
        [[nodiscard]] const T* FindSubsystemInScene() const;

    private:
        void MarkForDestruction();
        void RemoveSubsystem(size_t index, const AZ::TypeId& subsystemType);

        RemovalEvent m_removalEvent;
        SubsystemEvent m_subsystemEvent;

        // Storing keys separate from data to optimize for fast key search.
        AZStd::vector<AZ::TypeId> m_systemKeys;
        AZStd::vector<AZStd::any> m_systemObjects;

        // Name that identifies the scene.
        AZStd::string m_name;
        // Parent to this scene. Any subsystems are inherited from the parent but can be overwritten locally.
        AZStd::shared_ptr<Scene> m_parent;
        // If false, the scene has been removed from scene system and can no longer be found. As soon as all handles to the scene are
        // released it will be destroyed.
        bool m_isAlive{ true };
    };
} // AzFramework

#include <AzFramework/Scene/Scene.inl>
