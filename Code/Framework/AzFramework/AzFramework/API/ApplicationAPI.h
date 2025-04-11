/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/string.h>
#include <AzCore/PlatformIncl.h>

#include <AzFramework/CommandLine/CommandLine.h>

namespace AZ
{
    class Entity;
    class ComponentApplication;

    namespace Data
    {
        class AssetDatabase;
    }
}

namespace AzFramework
{
    class ApplicationRequests
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - Application is a singleton
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        ApplicationRequests() = default;
        ~ApplicationRequests() = default;

        using Bus = AZ::EBus<ApplicationRequests>;

        typedef AZStd::recursive_mutex MutexType;

        /// Fixup slashes and lowercase path.
        virtual void NormalizePath(AZStd::string& /*path*/) = 0;

        /// Fixup slashes.
        virtual void NormalizePathKeepCase(AZStd::string& /*path*/) = 0;

        /// Make path relative, based on the application root.
        virtual void MakePathRootRelative(AZStd::string& /*fullPath*/) {}

        /// Make path relative, based on the asset root.
        virtual void MakePathAssetRootRelative(AZStd::string& /*fullPath*/) {}

        /// Make path relative to the provided root.
        virtual void MakePathRelative(AZStd::string& /*fullPath*/, const char* /*rootPath*/) {}

        /// Get the Command Line arguments passed in.
        virtual const CommandLine* GetCommandLine() { return nullptr; }

        /// Get the Command Line arguments passed in. (Avoids collisions with platform specific macros.)
        virtual const CommandLine* GetApplicationCommandLine() { return nullptr; }

        /// Pump the system event loop once, regardless of whether there are any events to process.
        virtual void PumpSystemEventLoopOnce() {}

        /// Pump the system event loop until there are no events left to process.
        virtual void PumpSystemEventLoopUntilEmpty() {}

        /// Execute a function in a new thread and pump the system event loop at the specified frequency until the thread returns.
        virtual void PumpSystemEventLoopWhileDoingWorkInNewThread(const AZStd::chrono::milliseconds& /*eventPumpFrequency*/,
            const AZStd::function<void()>& /*workForNewThread*/,
            const char* /*newThreadName*/) {}

        /// Run the main loop until ExitMainLoop is called.
        virtual void RunMainLoop() {}

        /// Request to exit the main loop.
        virtual void ExitMainLoop() {}

        /// Returns true is ExitMainLoop has been called, false otherwise.
        virtual bool WasExitMainLoopRequested() { return false; }

        /// Terminate the application due to an error
        virtual void TerminateOnError(int errorCode) { exit(errorCode); }

        /// Resolve a path thats relative to the engine folder to an absolute path
        virtual void ResolveEnginePath(AZStd::string& /*engineRelativePath*/) const {}

        /// Calculate the branch token from the current application's engine root
        virtual void CalculateBranchTokenForEngineRoot(AZStd::string& token) const = 0;

        /// Returns true if Editor Mode Feedback is enabled
        virtual bool IsEditorModeFeedbackEnabled() const { return false; }

        /// Returns true if Prefab System is enabled, false if Legacy Slice System is enabled
        /// @deprecated the Legacy Slice System for level editing has been deprecated.
        virtual bool IsPrefabSystemEnabled() const { return true; }

        /// Returns true if the additional work in progress Prefab features are enabled, false otherwise
        virtual bool ArePrefabWipFeaturesEnabled() const { return false; }

        /// Sets whether or not the Prefab System should be enabled.  The application will need to be restarted when this changes
        virtual void SetPrefabSystemEnabled([[maybe_unused]] bool enable) {}

        /// Returns true if Prefab System is enabled for use with levels, false if legacy level system is enabled (level.pak)
        /// @deprecated Use 'IsPrefabSystemEnabled' instead
        virtual bool IsPrefabSystemForLevelsEnabled() const { return false; }

        /// Returns true if code should assert when the Legacy Slice System is used
        virtual bool ShouldAssertForLegacySlicesUsage() const { return false; }

        /*!
        * Returns a Type Uuid of the component for the given componentId and entityId.
        * if no component matches the entity and component Id pair, a Null Uuid is returned
        * \param entityId - the Id of the entity containing the component
        * \param componentId - the Id of the component whose TypeId you wish to get
        */
        virtual AZ::Uuid GetComponentTypeId(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) { (void)entityId; (void)componentId; return AZ::Uuid::CreateNull(); };

        template<typename ComponentFactoryType>
        void RegisterComponentType()
        {
            RegisterComponent(new ComponentFactoryType());
        }
    };


    class ApplicationLifecycleEvents
        : public AZ::EBusTraits
    {
    public:
        enum class Event
        {
            None = 0,
            Unconstrain,
            Constrain,
            Suspend,
            Resume
        };

        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~ApplicationLifecycleEvents() {}

        using Bus = AZ::EBus<ApplicationLifecycleEvents>;

        // AzFramework::Application listens for all the system specific
        // events, and translates them into the system independent ones
        // defined here. Applications are free to listen to one or both
        // sets of events but responding just to the system independent
        // ones should be sufficient for most applications.
        virtual void OnApplicationConstrained(Event /*lastEvent*/) {}
        virtual void OnApplicationUnconstrained(Event /*lastEvent*/) {}

        virtual void OnApplicationSuspended(Event /*lastEvent*/) {}
        virtual void OnApplicationResumed(Event /*lastEvent*/) {}

        virtual void OnMobileApplicationWillTerminate() {}
        virtual void OnMobileApplicationLowMemoryWarning() {}

        // Events triggered when the application window has been
        // created/destoryed.  This is currently only supported 
        // on Android so the renderer can correctly manage the 
        // rendering context.
        virtual void OnApplicationWindowCreated() {}
        virtual void OnApplicationWindowDestroy() {}

        // Event triggered when an orientation change occurs.
        // This is currently only supported on Android so the 
        // renderer can handle orientation changes.
        virtual void OnApplicationWindowRedrawNeeded() {}

        // Event triggered when the application is about to stop.
        // This is useful to unload certain resources before any entities are destroyed.
        virtual void OnApplicationAboutToStop() {}
    };

    class LevelLoadBlockerRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Gives handlers the opportunity to block the loadlevel command.
        //! There can be multiple handlers for this bus; if any one of them return true the loadlevel command will be stopped.
        //! @return Return true to stop loading.
        virtual bool ShouldBlockLevelLoading([[maybe_unused]]const char* levelName) { return false; }
    };
    using LevelLoadBlockerBus = AZ::EBus<LevelLoadBlockerRequests>;

    class ILevelSystemLifecycle
    {
    public:
        AZ_TYPE_INFO(ILevelSystemLifecycle, "{BDF3616A-4725-4B5D-AB71-34DFCDF9C5B0}");
        virtual ~ILevelSystemLifecycle() = default;

        //! Returns the name of the currently loaded level.
        //! Note: for spawnable level system, this is the cache folder path to the level asset. Example: levels/mylevel/mylevel.spawnable
        //! @return Level name or empty string if no level loaded.
        virtual const char* GetCurrentLevelName() const = 0;

        //! Checks if a level is loaded
        //! @return true if a level is loaded; otherwise false.
        virtual bool IsLevelLoaded() const = 0;
    };
    using LevelSystemLifecycleInterface = AZ::Interface<ILevelSystemLifecycle>;

    class LevelSystemLifecycleNotifications
        : public AZ::EBusTraits
    {
    public:
        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        // Called when loading a level fails due to it not being found.
        virtual void OnLevelNotFound([[maybe_unused]] const char* levelName) {}

        // Called after ILevelSystem::PrepareNextLevel() completes.
        virtual void OnPrepareNextLevel([[maybe_unused]] const char* levelName) {}

        // Called after ILevelSystem::OnLoadingStart() completes, before the level actually starts loading.
        virtual void OnLoadingStart([[maybe_unused]] const char* levelName) {}

        // Called after the level finished
        virtual void OnLoadingComplete([[maybe_unused]] const char* levelName) {}

        // Called when there's an error loading a level, with the level info and a description of the error.
        virtual void OnLoadingError([[maybe_unused]] const char* levelName, [[maybe_unused]] const char* error) {}

        // Called whenever the loading status of a level changes. progressAmount goes from 0->100.
        virtual void OnLoadingProgress([[maybe_unused]] const char* levelName, [[maybe_unused]] int progressAmount) {}

        // Called after a level is unloaded, before the data is freed.
        virtual void OnUnloadComplete([[maybe_unused]] const char* levelName) {}
    };
    using LevelSystemLifecycleNotificationBus = AZ::EBus<LevelSystemLifecycleNotifications>;
} // namespace AzFramework

DECLARE_EBUS_EXTERN(AzFramework::ApplicationRequests);
