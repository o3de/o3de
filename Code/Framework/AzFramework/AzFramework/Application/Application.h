/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/Instance/InstancePool.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/fixed_string.h>

#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/API/ApplicationAPI.h>


namespace AZ
{
    class Component;

    namespace Internal
    {
        class ComponentFactoryInterface;
    }

    namespace IO
    {
        class Archive;
        class FileIOBase;
        class LocalFileIO;
    }
}

namespace AzFramework
{
    class Application
        : public AZ::ComponentApplication
        , public AZ::UserSettingsFileLocatorBus::Handler
        , public ApplicationRequests::Bus::Handler
    {
    public:
        // Base class for platform specific implementations of the application.
        class Implementation
        {
        public:
            virtual ~Implementation() = default;
            virtual void PumpSystemEventLoopOnce() = 0;
            virtual void PumpSystemEventLoopUntilEmpty() = 0;
            virtual void TerminateOnError(int errorCode) { exit(errorCode); }
        };

        class ImplementationFactory
        {
        public:
            AZ_TYPE_INFO(ImplementationFactory, "{D840FD45-97BC-40D6-A92B-C83B607EA9D5}");
            virtual ~ImplementationFactory() = default;
            virtual AZStd::unique_ptr<Implementation> Create() = 0;
        };

        AZ_RTTI(Application, "{0BD2388B-F435-461C-9C84-D0A96CAF32E4}", AZ::ComponentApplication);
        AZ_CLASS_ALLOCATOR(Application, AZ::SystemAllocator);

        // Publicized types & methods from base ComponentApplication.
        using AZ::ComponentApplication::Descriptor;
        using AZ::ComponentApplication::StartupParameters;
        using AZ::ComponentApplication::GetSerializeContext;
        using AZ::ComponentApplication::RegisterComponentDescriptor;

        /**
         * You can pass your command line parameters from main here
         * so that they are thus available in GetCommandLine later, and can be retrieved.
         * see notes in GetArgC() and GetArgV() for details about the arguments.
         */
        Application(int* argc, char*** argv);  ///< recommended:  supply &argc and &argv from void main(...) here.
        Application(); ///< for backward compatibility.  If you call this, GetArgC and GetArgV will return nullptr.
        // Allows passing in a JSON Merge Patch string that can bootstrap
        // the settings registry with an initial set of settings
        explicit Application(AZ::ComponentApplicationSettings componentAppSettings);
        Application(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings);
        ~Application();

        /**
         * Executes the AZ:ComponentApplication::Create method and initializes Application constructs.
         * Uses a variant to maintain backwards compatibility with both the Start(const Descriptor& descriptor, const StartupParamters&)
         * and the Start(const char* descriptorFile, const StaartupParameters&) overloads
         */
        virtual void Start(const Descriptor& descriptor, const StartupParameters& startupParameters = StartupParameters());
        /**
         * Executes AZ::ComponentApplication::Destroy, and shuts down Application specific constructs.
         */
        virtual void Stop();

        void Tick() override;


        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;

        //////////////////////////////////////////////////////////////////////////
        //! ApplicationRequests::Bus::Handler
        void ResolveEnginePath(AZStd::string& engineRelativePath) const override;
        void CalculateBranchTokenForEngineRoot(AZStd::string& token) const override;
        bool IsEditorModeFeedbackEnabled() const override;
        bool IsPrefabSystemEnabled() const override;
        bool ArePrefabWipFeaturesEnabled() const override;
        void SetPrefabSystemEnabled(bool enable) override;
        bool IsPrefabSystemForLevelsEnabled() const override;
        bool ShouldAssertForLegacySlicesUsage() const override;

#pragma push_macro("GetCommandLine")
#undef GetCommandLine
        const CommandLine* GetCommandLine() override { return &m_commandLine; }
#pragma pop_macro("GetCommandLine")
        const CommandLine* GetApplicationCommandLine() override { return &m_commandLine; }

        void MakePathRootRelative(AZStd::string& fullPath) override;
        void MakePathAssetRootRelative(AZStd::string& fullPath) override;
        void MakePathRelative(AZStd::string& fullPath, const char* rootPath) override;
        void NormalizePath(AZStd::string& path) override;
        void NormalizePathKeepCase(AZStd::string& path) override;
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;
        void PumpSystemEventLoopWhileDoingWorkInNewThread(const AZStd::chrono::milliseconds& eventPumpFrequency,
                                                          const AZStd::function<void()>& workForNewThread,
                                                          const char* newThreadName) override;
        void RunMainLoop() override;
        void ExitMainLoop() override { m_exitMainLoopRequested = true; }
        bool WasExitMainLoopRequested() override { return m_exitMainLoopRequested; }
        void TerminateOnError(int errorCode) override;
        AZ::Uuid GetComponentTypeId(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;

        //////////////////////////////////////////////////////////////////////////

        // Convenience function that should be called instead of the standard exit() function to ensure platform requirements are met.
        static void Exit(int errorCode) { ApplicationRequests::Bus::Broadcast(&ApplicationRequests::TerminateOnError, errorCode); }

    protected:

        /**
         * Called by Start method. Override to add custom startup logic.
         */
        virtual void StartCommon(AZ::Entity* systemEntity);

        /**
         * set the LocalFileIO and ArchiveFileIO instances file aliases if the 
         * FileIOBase environment variable is pointing to the instances owned by
         * the application
         */
        void SetFileIOAliases();

        //////////////////////////////////////////////////////////////////////////
        //! AZ::ComponentApplication
        void RegisterCoreComponents() override;
        void Reflect(AZ::ReflectContext* context) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //! UserSettingsFileLocatorBus
        AZStd::string ResolveFilePath(AZ::u32 providerId) override;
        //////////////////////////////////////////////////////////////////////////

        AZ::Component* EnsureComponentAdded(AZ::Entity* systemEntity, const AZ::Uuid& typeId);

        template <typename ComponentType>
        AZ::Component* EnsureComponentAdded(AZ::Entity* systemEntity)
        {
            return EnsureComponentAdded(systemEntity, ComponentType::RTTI_Type());
        }

        virtual const char* GetCurrentConfigurationName() const;

        void CreateReflectionManager() override;

        AZ::StringFunc::Path::FixedString m_configFilePath;

        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_directFileIO; ///> The Direct file IO instance is a LocalFileIO.
        AZStd::unique_ptr<AZ::IO::FileIOBase> m_archiveFileIO; ///> The Default file IO instance is a ArchiveFileIO.
        AZStd::unique_ptr<AZ::IO::Archive> m_archive; ///> The AZ::IO::Instance
        AZStd::unique_ptr<Implementation> m_pimpl;
        AZStd::unique_ptr<AZ::NativeUI::NativeUIRequests> m_nativeUI;
        AZStd::unique_ptr<AZ::InstancePoolManager> m_poolManager;

        bool m_ownsConsole = false;

        bool m_exitMainLoopRequested = false;

    };
} // namespace AzFramework

AZ_DECLARE_BUDGET(AzFramework);

