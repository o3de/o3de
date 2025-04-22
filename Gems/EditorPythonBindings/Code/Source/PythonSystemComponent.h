/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <EditorPythonBindings/EditorPythonBindingsSymbols.h>

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/API/PythonLoader.h>

#include <Source/ActionManager/PythonActionManagerHandler.h>

namespace EditorPythonBindings
{
    /**
      * Manages the Python interpreter inside this Gem (Editor only)
      * - redirects the Python standard output and error streams to AZ_TracePrintf and AZ_Warning, respectively
      */      
    class PythonSystemComponent
        : public AZ::Component
        , protected AzToolsFramework::EditorPythonEventsInterface
        , protected AzToolsFramework::EditorPythonRunnerRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PythonSystemComponent, PythonSystemComponentTypeId, AZ::Component);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorPythonEventsInterface
        bool StartPython(bool silenceWarnings = false) override;
        bool StopPython(bool silenceWarnings = false) override;
        bool IsPythonActive() override;
        void WaitForInitialization() override;
        void ExecuteWithLock(AZStd::function<void()> executionCallback) override;
        bool TryExecuteWithLock(AZStd::function<void()> executionCallback) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorPythonRunnerRequestBus::Handler interface implementation
        void ExecuteByString(AZStd::string_view script, bool printResult) override;
        bool ExecuteByFilename(AZStd::string_view filename) override;
        bool ExecuteByFilenameWithArgs(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args) override;
        bool ExecuteByFilenameAsTest(AZStd::string_view filename, AZStd::string_view testCase, const AZStd::vector<AZStd::string_view>& args) override;
        ////////////////////////////////////////////////////////////////////////
        
    private:
        class SymbolLogHelper;
        class PythonGILScopedLock;

        // handle multiple Python initializers and threads
        AZStd::atomic_int m_initalizeWaiterCount {0};
        AZStd::semaphore m_initalizeWaiter;
        AZStd::recursive_mutex m_lock;
        int m_lockRecursiveCounter = 0;
        AZStd::shared_ptr<SymbolLogHelper> m_symbolLogHelper;
        PythonActionManagerHandler m_pythonActionManagerHandler;
        AzToolsFramework::EmbeddedPython::PythonLoader m_pythonLoader;
    
        enum class Result
        {
            Okay,
            Error_IsNotInitialized,
            Error_InvalidFilename,
            Error_MissingFile,
            Error_FileOpenValidation,
            Error_InternalException,
            Error_PythonException,
        };
        Result EvaluateFile(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args);

        // bootstrap logic and data
        using PythonPathStack = AZStd::vector<AZStd::string>;
        void DiscoverPythonPaths(PythonPathStack& pythonPathStack);
        void ExecuteBootstrapScripts(const PythonPathStack& pythonPathStack);
        bool ExtendSysPath(const AZStd::unordered_set<AZStd::string>& extendPaths);

        // starts the Python interpreter 
        bool StartPythonInterpreter(const PythonPathStack& pythonPathStack);
        bool StopPythonInterpreter();
    };
}
