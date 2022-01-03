/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef EDITORFRAMEWORKAPPLICATION_H
#define EDITORFRAMEWORKAPPLICATION_H

#include <AzCore/base.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include "EditorFrameworkAPI.h"

#pragma once

namespace AZ
{
    class SerializeContext;
}

namespace LegacyFramework
{
    struct ApplicationDesc
    {
        void* m_applicationModule;  // only necessary if you want to attach your application as a DLL plugin to another application, hosting it
        bool m_enableGUI; // false if you want none of the QT or GUI functionality to exist.  You cannot use project manager if you do this.
        bool m_enableGridmate; // false if you want to not activate the network communications module.
        bool m_enablePerforce; // false if you want to not activate perforce SCM integration.  note that this will eventually become a plugin anyway
        bool m_enableProjectManager; // false if you want to disable project management.  No project path will be set and the project picker GUI will not appear.
        bool m_shouldRunAssetProcessor; // false if you want to disable auto launching the asset processor.
        bool m_saveUserSettings; // true by default - set it to false if you want not to store usersettings (ie, you have no per-user state because you're something like an asset builder!)

        int m_argc;
        char** m_argv;

        char m_applicationName[AZ_MAX_PATH_LEN];

        ApplicationDesc(const char* name = "Application", int argc = 0, char** argv = nullptr);
        ApplicationDesc(const ApplicationDesc& other);
        ApplicationDesc& operator=(const ApplicationDesc& other);
    private:
    };


    class Application
        : public AZ::ComponentApplication
        , protected FrameworkApplicationMessages::Handler
        , protected CoreMessageBus::Handler
    {
        /// Create application, if systemEntityFileName is NULL, we will create with default settings.
    public:

        using CoreMessageBus::Handler::Run;
        virtual int Run(const ApplicationDesc& desc);
        Application();

        void CreateReflectionManager() override;

    protected:

        // ------------------------------------------------------------------
        // implementation of FrameworkApplicationMessages::Handler
        bool IsRunningInGUIMode() override { return m_desc.m_enableGUI; }
        bool RequiresGameProject() override { return m_desc.m_enableProjectManager; }
        bool ShouldRunAssetProcessor() override { return m_desc.m_shouldRunAssetProcessor; }
        void* GetMainModule() override;
        const char* GetApplicationName() override;
        const char* GetApplicationModule() override;
        const char* GetApplicationDirectory() override;
        const AzFramework::CommandLine* GetCommandLineParser() override;
        void TeardownApplicationComponent() override;
        void RunAssetProcessor() override;
        // ------------------------------------------------------------------

        void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override;

        // ------------------------------------------------------------------
        // implementation of CoreMessageBus::Handler
        void OnProjectSet(const char*  /*pathToProject*/) override;
        // ------------------------------------------------------------------

        // This is called during the bootstrap and makes all the components we should have for SYSTEM minimal functionality.
        // This happens BEFORE the project is ready.  Think carefully before you add additional system components.  Examples of system components
        // are memory managers, crash reporters, log writers, and the project manager itself which lets you switch to a project.
        virtual void CreateSystemComponents();

        // once the project is ready, then the CreateApplicationComponents() function is called.  Those components are guarinteed
        // to run inside a the context of a "Project" and thus have access to project data such as assets.
        virtual void CreateApplicationComponents();

        // called after the application entity is completed to pass some specifics to generic components.
        virtual void OnApplicationEntityActivated();

        // you must call EnsureComponentCreated and EnsureComponentRemoved functions inside the above function create functions:
        // returns TRUE if the component already existed, FALSE if it had to create one.
        // adds it as a SYSTEM component if its called inside CreateSystemComponents
        // adds it as an APPLICATION component if its called inside CreateApplicationComponents
        // will ERROR if you remove an application component after the application
        bool EnsureComponentCreated(AZ::Uuid componentCRC);

        // returns TRUE if the component existed, FALSE if the component did not exist.
        // will ERROR if you remove a system component after the system is booted
        bool EnsureComponentRemoved(AZ::Uuid componentCRC);

        /**
         * This is the function that will be called instantly after the memory
         * manager is created. This is where we should register all core component
         * factories that will participate in the loading of the bootstrap file
         * or all factories in general.
         * When you create your own application this is where you should FIRST call
         * ComponentApplication::RegisterCoreComponents and then register the application
         * specific core components.
         */
        void RegisterCoreComponents() override;
        AZ::Entity* m_ptrSystemEntity;

        int GetDesiredExitCode() override { return m_desiredExitCode; }
        void SetDesiredExitCode(int code) override { m_desiredExitCode = code; }
        bool GetAbortRequested() override { return m_abortRequested; }
        void SetAbortRequested() override { m_abortRequested = true; }
        AZStd::string GetApplicationGlobalStoragePath() override;
        bool IsPrimary() override { return m_isPrimary; }

        bool IsAppConfigWritable() override;

        AZ::Entity* m_applicationEntity;

    private:
        void CreateApplicationComponent();
        void SaveApplicationEntity();

        char m_applicationModule[AZ_MAX_PATH_LEN];
        int m_desiredExitCode;
        bool m_isPrimary;
        volatile bool m_abortRequested; // if you CTRL+C in a console app, this becomes true.  its up to you to check...
        char m_applicationFilePath[AZ_MAX_PATH_LEN];
        ApplicationDesc m_desc;
        AzFramework::CommandLine* m_ptrCommandLineParser;
    };
}

#endif // EDITORFRAMEWORKAPPLICATION_H
