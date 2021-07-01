/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef EditorFrameworkAPI_H
#define EditorFrameworkAPI_H

#include <AzCore/base.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/EBus/EBus.h>
#include <AzFramework/CommandLine/CommandLine.h>

#pragma once

// this file contains the API for the buses that the framework communicates on to NON-GUI-CLIENTS
// note that this does not include UI messaging, this is for non-ui parts of it!

namespace AZ
{
    class SerializeContext;
}

#ifdef AZ_PLATFORM_WINDOWS
    typedef HINSTANCE HMODULE;
#endif

namespace LegacyFramework
{
    // we agree that an entity list is a list of entity IDs
    typedef AZStd::vector<AZ::EntityId> EntityList;
    typedef AZ::u32 IPCHandleType;

    /** Core messages core messages go to whoever wants them.  If you're interested in these messages, listen here.
    * they are always broadcast (To all listeners, not by address)
    */
    class CoreMessages
        : public AZ::EBusTraits
    {
    public:
        virtual ~CoreMessages() {}
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::MultipleAndOrdered; // there are many listeners
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // this is a broadcast bus

        bool Compare(const CoreMessages* rhs) const
        {
            return this < rhs;
        }

        virtual void OnRestoreState() {} /// sent when everything is registered up and ready to go, this is what bootstraps stuff to get going.

        virtual void OnReady() {} /// sent after onrestorestate - the entire app should be up and running now.

        /** in a GUI application, this is already implemented in UIFramework in order to initialize QT and then execute the main application event loop
        * in that case, implment OnRestoreState() in your gui-related components to start them functioning.
        * on the other hand, in a Non-GUI application (such as a console app, if you initialized it without QT), you will not have a QApplication and
        * you need to implment RUN() yourself to do whatever it is you want to do (process assets, etc).
        * you can use virtual _TCHAR** FrameworkApplicationMessages::GetArguments(int& argc) = 0; to get the command line arguments.
        * if you use your own console app, its up to you to send OnRestoreState, OnGetPermissionToShutdown, OnSaveState, etc.
        */
        virtual void Run() {}


        /** Executed as part of inter-process communication during startup of a second instance.
        * this happens INSTEAD of run, if you are a secondary instance of an application instead of the primary.
        * by default, the UI Framework, when it gets this, sends the "open" message to the primary instance for every misc value on its command line
        * causing the primary to attempt to open it instead of ourselves.
        * you can also implement this yourself in order to send messages to the primary (if you wish).
        *  note:  The project is not set ever as another instance...
        */
        virtual void RunAsAnotherInstance() {}

        /** this happens when the project manager has set its project.
        * the next thing that will happen is you get an OnRestoreState and OnReady() from the UI manager - if you're a UI application
        * otherwise, you'll have to do whatever you want at this point in your non-gui application.
        */
        virtual void OnProjectSet(const char*  /*pathToProject*/) {}

        /** this is sent when the user hits the [x] button to close the entire application and the final important window is closed.
        * Components should override this if its possible for the user to abort the shutdown and deny it.  This is also your opportunity to
        * kick off any saves and caches that need to be cached.
        * If EVERYONE answers true, then everyone is polled constantly with /ref CheckOkeyToShutdown, and then when
        * all listeners return true for that, the actual shutdown occurs.
        */
        virtual bool OnGetPermissionToShutDown() { return true; }

        /** sent to everything when the app is about to shut down - do what you need to do to ensure that state is stored somewhere.
        * this happens before anyone destroys state ( /ref OnDestroyState will be called after all OnSaveState)
        */
        virtual void OnSaveState() {}

        /** this message gets sent for you to destroy state objects that depend on outside components
        * an example is components that have GUIs (such as the Lua Editor) need to destroy their Qt Objects, because we will tear down the QApplication after sending this.
        * this happens after /ref OnSaveState
        */
        virtual void OnDestroyState() {}

        /** During shutdown, this is Sent repeatedly until everyone responds TRUE.
        * Once everyone returns true from /ref OnGetPermissionToShutDown, we then start to send /ref CheckOkayToShutDown periodically until
        * everyone returns TRUE, after which /ref OnSaveState will occur, then /ref OnDestroyState , and then finally the component
        * will be stopped and destroyed.
        * After the user says its okay to shut down, this is called in a loop to give everyone time to flush their buffers / wait and cancel
        * pending operations.  So for example, if you return true to /ref OnGetPermissionToShutdown you should start saving your files
        * and cleaning up in that function.  Then check to see if your pending async calls are done in /ref CheckOkayToShutdown
        */
        virtual bool CheckOkayToShutDown() { return true; } // until everyone returns true, we can't shut down.


        /** happens when the framework goes in focus in the global OS sense.
        * i.e if the ANY window belonging to the app comes to front, this is sent to all contexts.
        */
        virtual void ApplicationDeactivated() {}

        /** happens when the framework goes out of focus in the global OS sense.
        * i.e if a different application gets focus, this is sent to all contexts.
        */
        virtual void ApplicationActivated() {}


        /** This is broadcast to ALL applications registered with AddComponentInfo
        * Each listener must check to see if the Uuid given is the UUid they registered with, in AddComponentInfo.
        * This allows listeners to react to more than one Uuid menu item.  Perhaps one context is responsible for more than one main window.
        */
        virtual void ApplicationShow(AZ::Uuid) {}

        /** This is broadcast to ALL applications registered with AddComponentInfo
        * Each listener must check to see if the Uuid given is the UUid they registered with, in AddComponentInfo.
        * This allows listeners to react to more than one Uuid menu item.  Perhaps one context is responsible for more than one main window.
        */
        virtual void ApplicationHide(AZ::Uuid) {}

        /** A request from the application framework to ask the system what root level windows may be open.
        * If you don't respond to this by issuing ApplicationCensusReply on the framwork message bus, the application will be allowed to quit
        * entirely, even if your window is open.
        * You should respond to recieving this message by calling ApplicationCensusReply if your context has a window open that counts as
        * one of the 'root' windows of an app that should stop the app from closing if the window is open.  When the last window that responded true
        * is closed, the entire editor quits.  windows like floating browsers which are NOT part of the permanent context should not respond.
        */
        virtual void ApplicationCensus() {}
    };

    typedef AZ::EBus<CoreMessages> CoreMessageBus;


    /** Retrieves the name of the app you set in your application descriptor before calling Run
    */
    const char* appName();

    /** Returns the name of the binary that this application belongs to (the full path)
    */
    const char* appModule();

    /** Returns the full path to the folder that contains this application binary.
    */
    const char* appDir();

    /** Returns true if this is the first instance of this application run.  False if another instance is already running.
    */
    bool isPrimary();

    /** Returns true if someone issued a SetAbortRequested(true) on the application bus.
    * Can be used for console applications when someone presses CTRL+C or such.
    */
    bool appAbortRequested();

    /** returns true if the application descriptor was created using GUI Mode set to true.
    */
    bool IsGUIMode();

    /** Adds a given folder to the executable (and shared dynamic library) search path for the current process.
    * Does not alter the actual system environment, only for this run of the application.
    */
    void AddToPATH(const char* folder);

    /** Returns true if the application global configuration file (appname.xml) is writeable.
    */
    bool IsAppConfigWritable();

    /** helper function which retrieves the serialize context and asserts if its not found.
    */
    AZ::SerializeContext* GetSerializeContext();

    /** Returns true if the application descriptor passed in during creation of the application had it set to true.
    * If false, it means that you have not requested that there be a "game project".
    * the project manager, which makes sure the project directory is set and can make new projects will in that case not activate
    * use this only if you're making a standalone app that needs no project (ie, no assets or anything from a project folder).
    */
    bool RequiresGameProject();

    /** Returns true if this is an app which USES assets and needs them to be ready.
    * As opposed to pipeline tools such as the Project Creator or the asset processor itself.
    */
    bool ShouldRunAssetProcessor();

    /** FrameworkApplicationMessages is how you communicate to the framework itself (instead of the above /ref CoreMessages which goes the other way).
    */
    class FrameworkApplicationMessages
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single; // its a singleton
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // its a singleton

        typedef AZ::EBus<FrameworkApplicationMessages> Bus;
        typedef Bus::Handler Handler;

        virtual ~FrameworkApplicationMessages() {}

        /** Equivalent to and used by /ref IsGUIMode
        */
        virtual bool IsRunningInGUIMode() = 0;

        /** Retrieve a Command Line Parser object that you can then use to check for values on the command line
        */
        virtual const AzFramework::CommandLine* GetCommandLineParser() = 0;

        /** (Windows) retrieves the main module of the executable.
        * This is always going to be the main executable except in the situation where the framework may be running as a DLL belonging to another process or program.
        */
#ifdef AZ_PLATFORM_WINDOWS
        virtual HMODULE GetMainModule() = 0;
#endif

        virtual const char* GetApplicationName() = 0;
        virtual const char* GetApplicationModule() = 0;
        virtual const char* GetApplicationDirectory() = 0;

        /** Internal use only.
        */
        virtual void TeardownApplicationComponent() = 0;

        /** Reads the return code set by /ref SetDesiredExitCode.
        * Since the application itself is in control, your void main() can use this function to know what to return in case you're making tools that are used in batches.
        */
        virtual int GetDesiredExitCode() = 0;

        /**
        * If you're making a command-line program built on the Application Framework, you can use /ref SetDesiredExitCode to manipulate the return code.
        */
        virtual void SetDesiredExitCode(int code) = 0;

        virtual bool GetAbortRequested() = 0; /// returns true if someone called /ref SetAbortRequested.  See /ref appAbortRequested, the helper function that calls this
        virtual void SetAbortRequested() = 0; /// Call this to indicate that something has gone wrong and we need to bail out (of a batch process)

        /** returns a path that points at the place where we can store application data that is specific to this user.
        * note that this is not the application name, its just the root of a folder that is guaranteed to not be temporary and guaranteed
        * to be writable.
        * it is USER-SPECIFIC, but not application specific
        * on windows, for example, this would be the Users/Name/AppData/Roaming/
        */
        virtual AZStd::string GetApplicationGlobalStoragePath() = 0;

        /** this is true if you are the first, 'primary' instance running.
        * if this is false it means another app of the same kind ran before you and is still running
        */
        virtual bool IsPrimary() = 0;

        virtual bool RequiresGameProject() = 0; /// see /ref RequiresGameProject above.

        virtual bool IsAppConfigWritable() = 0; /// see /ref IsAppConfigWriteable above.

        virtual bool ShouldRunAssetProcessor() = 0; /// see /ref ShouldRunAssetProcessor above.

        /** Run the asset processor on this project.
        * only valid on projects!
        */
        virtual void RunAssetProcessor() {} // not required to be implemented
    };

    /**  This bus communicates TO the log component from the outside
    * Its used to send queries to the log component itself.
    */
    class LogComponentAPI
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single; // every listener registers unordered on this bus
        typedef AZ::EBus<LogComponentAPI> Bus;
        //////////////////////////////////////////////////////////////////////////
        virtual ~LogComponentAPI() {}

        virtual void EnumWindowTypes(AZStd::vector<AZStd::string>& target) = 0; /// get me a list of all the types of windows we've seen
        virtual void RegisterWindowType(const AZStd::string& source) = 0; /// add this type to the known list of window types:
    };

    // -------------------------- IPC "COMMANDS" ------------------------------
    /** IPC command system allows commands to be sent to other instances of the same application
    */
    class IPCCommandAPI
        : public AZ::EBusTraits
    {
    public:
        typedef AZStd::function<bool(const AZStd::string& parameters)> IPCCommandHandler;

        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single; // every listener registers unordered on this bus
        //////////////////////////////////////////////////////////////////////////

        virtual ~IPCCommandAPI() {}

        /** Register a handler to handle system calls of a certain kind.  The operating system will be invoking this from the "outside" and thus will be operating system 'verbs'
        * example:  handle = RegisterIPCHandler("open", SomeOpenFunction)
        * example:  handle = RegisterIPCHandler("reprocessfile" , AZStd::bind(&someclass::somefunc, this,...)))
        * example:  handle = RegisterIPCHandler("print" , PrintSomethingForSomeReason);
        */
        virtual IPCHandleType RegisterIPCHandler(const char* commandName, IPCCommandHandler handlerFn) = 0;

        virtual void UnregisterIPCHandler(IPCHandleType handle) = 0;

        /** Called internally by the system.
        * This exists so that IPC handlers, even if they get messages in other threads, will drain their queue only in the proper thread.
        */
        virtual void ExecuteIPCHandlers() = 0;

        /** call this to send an IPC command to the primary
        * you can only do this if you're not a primary.  It can be any command you'd like the primary to perform as long as there is a handler
        * registered with /ref RegisterIPCHandler
        */
        virtual void SendIPCCommand(const char* commandName, const char* parameters) = 0;
    };

    typedef AZ::EBus<IPCCommandAPI> IPCCommandBus;
};

#endif
