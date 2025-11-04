/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PythonSystemComponent.h>
#include <EditorPythonBindings/EditorPythonBindingsBus.h>

#include <EditorPythonBindings/PythonCommon.h>
#include <Source/PythonSymbolsBus.h>
#include <osdefs.h> // for DELIM
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/pybind11.h>


#include <AzCore/Component/EntityId.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/CommandLine/CommandRegistrationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonScriptNotificationsBus.h>
#include <AzToolsFramework/API/PythonLoader.h>

// this is called the first time a Python script contains "import azlmbr"
PYBIND11_EMBEDDED_MODULE(azlmbr, m)
{
    EditorPythonBindings::EditorPythonBindingsNotificationBus::Broadcast(&EditorPythonBindings::EditorPythonBindingsNotificationBus::Events::OnImportModule, m.ptr());
}

namespace RedirectOutput
{
    using RedirectOutputFunc = AZStd::function<void(const char*)>;

    struct RedirectOutput
    {
        PyObject_HEAD
        RedirectOutputFunc write;
    };

    PyObject* RedirectWrite(PyObject* self, PyObject* args)
    {
        std::size_t written(0);
        RedirectOutput* selfimpl = reinterpret_cast<RedirectOutput*>(self);
        if (selfimpl->write)
        {
            char* data;
            if (!PyArg_ParseTuple(args, "s", &data))
            {
                return PyLong_FromSize_t(0);
            }
            selfimpl->write(data);
            written = strlen(data);
        }
        return PyLong_FromSize_t(written);
    }

    PyObject* RedirectFlush([[maybe_unused]] PyObject* self, [[maybe_unused]] PyObject* args)
    {
        // no-op
        return Py_BuildValue("");
    }

    PyMethodDef RedirectMethods[] =
    {
        {"write", RedirectWrite, METH_VARARGS, "sys.stdout.write"},
        {"flush", RedirectFlush, METH_VARARGS, "sys.stdout.flush"},
        {"write", RedirectWrite, METH_VARARGS, "sys.stderr.write"},
        {"flush", RedirectFlush, METH_VARARGS, "sys.stderr.flush"},
        {0, 0, 0, 0} // sentinel
    };

    PyTypeObject RedirectOutputType =
    {
        PyVarObject_HEAD_INIT(0, 0)
        "azlmbr_redirect.RedirectOutputType", // tp_name
        sizeof(RedirectOutput), /* tp_basicsize */
        0,                    /* tp_itemsize */
        0,                    /* tp_dealloc */
        0,                    /* tp_print */
        0,                    /* tp_getattr */
        0,                    /* tp_setattr */
        0,                    /* tp_reserved */
        0,                    /* tp_repr */
        0,                    /* tp_as_number */
        0,                    /* tp_as_sequence */
        0,                    /* tp_as_mapping */
        0,                    /* tp_hash  */
        0,                    /* tp_call */
        0,                    /* tp_str */
        0,                    /* tp_getattro */
        0,                    /* tp_setattro */
        0,                    /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,   /* tp_flags */
        "azlmbr_redirect objects", /* tp_doc */
        0,                    /* tp_traverse */
        0,                    /* tp_clear */
        0,                    /* tp_richcompare */
        0,                    /* tp_weaklistoffset */
        0,                    /* tp_iter */
        0,                    /* tp_iternext */
        RedirectMethods,      /* tp_methods */
        0,                    /* tp_members */
        0,                    /* tp_getset */
        0,                    /* tp_base */
        0,                    /* tp_dict */
        0,                    /* tp_descr_get */
        0,                    /* tp_descr_set */
        0,                    /* tp_dictoffset */
        0,                    /* tp_init */
        0,                    /* tp_alloc */
        0                     /* tp_new */
    };

    PyModuleDef RedirectOutputModule = { PyModuleDef_HEAD_INIT, "azlmbr_redirect", 0, -1, 0, };

    // Internal state
    PyObject* g_redirect_stdout = nullptr;
    PyObject* g_redirect_stdout_saved = nullptr;
    PyObject* g_redirect_stderr = nullptr;
    PyObject* g_redirect_stderr_saved = nullptr;

    PyMODINIT_FUNC PyInit_RedirectOutput(void)
    {
        g_redirect_stdout = nullptr;
        g_redirect_stdout_saved = nullptr;
        g_redirect_stderr = nullptr;
        g_redirect_stderr_saved = nullptr;

        RedirectOutputType.tp_new = PyType_GenericNew;
        if (PyType_Ready(&RedirectOutputType) < 0)
        {
            return 0;
        }

        PyObject* m = PyModule_Create(&RedirectOutputModule);
        if (m)
        {
            Py_INCREF(&RedirectOutputType);
            PyModule_AddObject(m, "Redirect", reinterpret_cast<PyObject*>(&RedirectOutputType));
        }
        return m;
    }

    void SetRedirection(const char* funcname, PyObject*& saved, PyObject*& current, RedirectOutputFunc func)
    {
        if (PyType_Ready(&RedirectOutputType) < 0)
        {
            AZ_Warning("python", false, "RedirectOutputType not ready!");
            return;
        }

        if (!current)
        {
            saved = PySys_GetObject(funcname); // borrowed
            current = RedirectOutputType.tp_new(&RedirectOutputType, 0, 0);
        }

        RedirectOutput* redirectOutput = reinterpret_cast<RedirectOutput*>(current);
        redirectOutput->write = func;
        PySys_SetObject(funcname, current);
    }

    void ResetRedirection(const char* funcname, PyObject*& saved, PyObject*& current)
    {
        if (current)
        {
            PySys_SetObject(funcname, saved);
        }
        Py_XDECREF(current);
        current = nullptr;
    }

    PyObject* s_RedirectModule = nullptr;

    void Intialize(PyObject* module)
    {
        using namespace AzToolsFramework;

        s_RedirectModule = module;

        SetRedirection("stdout", g_redirect_stdout_saved, g_redirect_stdout, [](const char* msg)
        {
            EditorPythonConsoleNotificationBus::Broadcast(&EditorPythonConsoleNotificationBus::Events::OnTraceMessage, msg);
        });

        SetRedirection("stderr", g_redirect_stderr_saved, g_redirect_stderr, [](const char* msg)
        {
            EditorPythonConsoleNotificationBus::Broadcast(&EditorPythonConsoleNotificationBus::Events::OnErrorMessage, msg);
        });

        PySys_WriteStdout("RedirectOutput installed");
    }

    void Shutdown()
    {
        ResetRedirection("stdout", g_redirect_stdout_saved, g_redirect_stdout);
        ResetRedirection("stderr", g_redirect_stderr_saved, g_redirect_stderr);
        Py_XDECREF(s_RedirectModule);
        s_RedirectModule = nullptr;
    }
} // namespace RedirectOutput

namespace EditorPythonBindings
{
    // A stand in bus to capture the log symbol queue events
    // so that when/if the PythonLogSymbolsComponent becomes
    // active it can write out the python symbols to disk
    class PythonSystemComponent::SymbolLogHelper final
        : public PythonSymbolEventBus::Handler
    {
    public:
        SymbolLogHelper()
        {
            PythonSymbolEventBus::Handler::BusConnect();
        }

        ~SymbolLogHelper()
        {
            PythonSymbolEventBus::ExecuteQueuedEvents();
            PythonSymbolEventBus::Handler::BusDisconnect();
        }

        void LogClass(const AZStd::string, const AZ::BehaviorClass*) override {}
        void LogClassWithName(const AZStd::string, const AZ::BehaviorClass*, const AZStd::string) override {}
        void LogClassMethod(
            const AZStd::string,
            const AZStd::string,
            const AZ::BehaviorClass*,
            const AZ::BehaviorMethod*) override {}
        void LogBus(const AZStd::string, const AZStd::string, const AZ::BehaviorEBus*) override {}
        void LogGlobalMethod(const AZStd::string, const AZStd::string, const AZ::BehaviorMethod*) override {}
        void LogGlobalProperty(const AZStd::string, const AZStd::string, const AZ::BehaviorProperty*) override {}
        void Finalize() override {}
    };

    // Manages the acquisition and release of the Python GIL (Global Interpreter Lock).
    // Used by PythonSystemComponent to lock the GIL when executing python.
    class PythonSystemComponent::PythonGILScopedLock final
    {
    public:
        PythonGILScopedLock(AZStd::recursive_mutex& lock, int& lockRecursiveCounter, bool tryLock = false);
        ~PythonGILScopedLock();

        bool IsLocked() const;

    protected:
        void Lock(bool tryLock);
        void Unlock();

        AZStd::recursive_mutex& m_lock;
        int& m_lockRecursiveCounter;
        bool m_locked = false;
        AZStd::unique_ptr<pybind11::gil_scoped_release> m_releaseGIL;
        AZStd::unique_ptr<pybind11::gil_scoped_acquire> m_acquireGIL;
    };

    PythonSystemComponent::PythonGILScopedLock::PythonGILScopedLock(AZStd::recursive_mutex& lock, int& lockRecursiveCounter, bool tryLock)
        : m_lock(lock)
        , m_lockRecursiveCounter(lockRecursiveCounter)
    {
        Lock(tryLock);
    }

    PythonSystemComponent::PythonGILScopedLock::~PythonGILScopedLock()
    {
        Unlock();
    }

    bool PythonSystemComponent::PythonGILScopedLock::IsLocked() const
    {
        return m_locked;
    }

    void PythonSystemComponent::PythonGILScopedLock::Lock(bool tryLock)
    {
        if (tryLock)
        {
            if (!m_lock.try_lock())
            {
                return;
            }
        }
        else
        {
            m_lock.lock();
        }

        m_locked = true;
        m_lockRecursiveCounter++;

        // Only Acquire the GIL when there is no recursion. If there is
        // recursion that means it's the same thread (because the mutex was able
        // to be locked) and therefore it's already got the GIL acquired.
        if (m_lockRecursiveCounter == 1)
        {
            m_releaseGIL = AZStd::make_unique<pybind11::gil_scoped_release>();
            m_acquireGIL = AZStd::make_unique<pybind11::gil_scoped_acquire>();
        }
    }

    void PythonSystemComponent::PythonGILScopedLock::Unlock()
    {
        if (!m_locked)
        {
            return;
        }

        m_acquireGIL.reset();
        m_releaseGIL.reset();
        m_lockRecursiveCounter--;
        m_locked = false;
        m_lock.unlock();
    }

    void PythonSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PythonSystemComponent, AZ::Component>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>{AZ_CRC_CE("AssetBuilder")})
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<PythonSystemComponent>("PythonSystemComponent", "The Python interpreter")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        PythonActionManagerHandler::Reflect(context);
    }

    void PythonSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(PythonEmbeddedService);
    }

    void PythonSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(PythonEmbeddedService);
    }

    void PythonSystemComponent::Activate()
    {
        AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Register(this);
        AzToolsFramework::EditorPythonRunnerRequestBus::Handler::BusConnect();
    }

    void PythonSystemComponent::Deactivate()
    {
        StopPython(true);
        AzToolsFramework::EditorPythonRunnerRequestBus::Handler::BusDisconnect();
        AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Unregister(this);
    }

    bool PythonSystemComponent::StartPython([[maybe_unused]] bool silenceWarnings)
    {
        struct ReleaseInitalizeWaiterScope final
        {
            using ReleaseFunction = AZStd::function<void(void)>;

            ReleaseInitalizeWaiterScope(ReleaseFunction releaseFunction)
            {
                m_releaseFunction = AZStd::move(releaseFunction);
            }
            ~ReleaseInitalizeWaiterScope()
            {
                m_releaseFunction();
            }
            ReleaseFunction m_releaseFunction;
        };
        ReleaseInitalizeWaiterScope scope([this]()
            {
                m_initalizeWaiter.release(m_initalizeWaiterCount);
                m_initalizeWaiterCount = 0;
            });

        if (Py_IsInitialized())
        {
            AZ_Warning("python", silenceWarnings, "Python is already active!");
            return false;
        }

        PythonPathStack pythonPathStack;
        DiscoverPythonPaths(pythonPathStack);

        EditorPythonBindingsNotificationBus::Broadcast(&EditorPythonBindingsNotificationBus::Events::OnPreInitialize);
        if (StartPythonInterpreter(pythonPathStack))
        {
            // initialize internal base module and bootstrap scripts
            ExecuteByString("import azlmbr", false);
            ExecuteBootstrapScripts(pythonPathStack);
            EditorPythonBindingsNotificationBus::Broadcast(&EditorPythonBindingsNotificationBus::Events::OnPostInitialize);
            return true;
        }
        return false;
    }

    bool PythonSystemComponent::StopPython([[maybe_unused]] bool silenceWarnings)
    {
        if (!Py_IsInitialized())
        {
            AZ_Warning("python", silenceWarnings, "Python is not active!");
            return false;
        }

        bool result = false;
        EditorPythonBindingsNotificationBus::Broadcast(&EditorPythonBindingsNotificationBus::Events::OnPreFinalize);

        result = StopPythonInterpreter();
        EditorPythonBindingsNotificationBus::Broadcast(&EditorPythonBindingsNotificationBus::Events::OnPostFinalize);
        return result;
    }

    bool PythonSystemComponent::IsPythonActive()
    {
        return Py_IsInitialized() != 0;
    }

    void PythonSystemComponent::WaitForInitialization()
    {
        m_initalizeWaiterCount++;
        m_initalizeWaiter.acquire();
    }

    void PythonSystemComponent::ExecuteWithLock(AZStd::function<void()> executionCallback)
    {
        PythonGILScopedLock lock(m_lock, m_lockRecursiveCounter);
        executionCallback();
    }

    bool PythonSystemComponent::TryExecuteWithLock(AZStd::function<void()> executionCallback)
    {
        PythonGILScopedLock lock(m_lock, m_lockRecursiveCounter, true /*tryLock*/);
        if (lock.IsLocked())
        {
            executionCallback();
            return true;
        }
        return false;
    }

    void PythonSystemComponent::DiscoverPythonPaths(PythonPathStack& pythonPathStack)
    {
        // the order of the Python paths is the order the Python bootstrap scripts will execute

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            return;
        }

        AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
        if (projectPath.empty())
        {
            return;
        }

        auto resolveScriptPath = [&pythonPathStack](AZStd::string_view path)
        {
            auto editorScriptsPath = AZ::IO::Path(path) / "Editor" / "Scripts";
            if (AZ::IO::SystemFile::Exists(editorScriptsPath.c_str()))
            {
                pythonPathStack.emplace_back(AZStd::move(editorScriptsPath.LexicallyNormal().Native()));
            }
        };

        // The discovery order will be:
        //   1 - The python venv site-packages
        //   2 - engine-root/EngineAsets
        //   3 - gems
        //   4 - project
        //   5 - user(dev)

        // 1 - The python venv site-packages
        AzToolsFramework::EmbeddedPython::PythonLoader::ReadPythonEggLinkPaths(
            AZ::Utils::GetEnginePath().c_str(),
            [&pythonPathStack](AZ::IO::PathView path)
            {
                pythonPathStack.emplace_back(path.Native());
            });

        // 2 - engine
        AZ::IO::FixedMaxPath engineRoot;
        if (settingsRegistry->Get(engineRoot.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder); !engineRoot.empty())
        {
            resolveScriptPath((engineRoot / "Assets").Native());
        }

        // 3 - gems
        AZStd::vector<AZ::IO::Path> gemSourcePaths;
        auto AppendGemPaths = [&gemSourcePaths](AZStd::string_view, AZStd::string_view gemPath)
        {
            gemSourcePaths.emplace_back(gemPath);
        };
        AZ::SettingsRegistryMergeUtils::VisitActiveGems(*settingsRegistry, AppendGemPaths);

        for (const AZ::IO::Path& gemSourcePath : gemSourcePaths)
        {
            resolveScriptPath(gemSourcePath.Native());
        }

        // 4 - project
        resolveScriptPath(AZStd::string_view{ projectPath });

        // 5 - user
        AZStd::string assetsType;
        AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, assetsType,
            AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, AzFramework::AssetSystem::Assets);
        if (!assetsType.empty())
        {
            AZ::IO::FixedMaxPath userCachePath;
            if (settingsRegistry->Get(userCachePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);
                !userCachePath.empty())
            {
                userCachePath /= "user";
                resolveScriptPath(userCachePath.Native());
            }
        }
    }

    void PythonSystemComponent::ExecuteBootstrapScripts(const PythonPathStack& pythonPathStack)
    {
        for(const auto& path : pythonPathStack)
        {
            AZStd::string bootstrapPath;
            AzFramework::StringFunc::Path::Join(path.c_str(), "bootstrap.py", bootstrapPath);
            if (AZ::IO::SystemFile::Exists(bootstrapPath.c_str()))
            {
                [[maybe_unused]] bool success = ExecuteByFilename(bootstrapPath);
                AZ_Assert(success, "Error while executing bootstrap script: %s", bootstrapPath.c_str());
            }
        }
    }

    bool PythonSystemComponent::StartPythonInterpreter(const PythonPathStack& pythonPathStack)
    {
        AZStd::unordered_set<AZStd::string> pyPackageSites(pythonPathStack.begin(), pythonPathStack.end());

        AZ::IO::FixedMaxPath engineRoot = AZ::Utils::GetEnginePath();

        // set PYTHON_HOME
        AZStd::string pyBasePath = AzToolsFramework::EmbeddedPython::PythonLoader::GetPythonHomePath(engineRoot.c_str()).StringAsPosix();
        if (!AZ::IO::SystemFile::Exists(pyBasePath.c_str()))
        {
            AZ_Warning("python", false, "Python home path must exist! path:%s", pyBasePath.c_str());
            return false;
        }
        AZStd::wstring pyHomePath;
        AZStd::to_wstring(pyHomePath, pyBasePath);
        Py_SetPythonHome(pyHomePath.c_str());

        PyImport_AppendInittab("azlmbr_redirect", RedirectOutput::PyInit_RedirectOutput);
        try
        {
            // ignore system location for sites site-packages
            Py_IsolatedFlag = 1; // -I - Also sets Py_NoUserSiteDirectory.  If removed PyNoUserSiteDirectory should be set.
            Py_IgnoreEnvironmentFlag = 1; // -E
            Py_InspectFlag = 1; // unhandled SystemExit will terminate the process unless Py_InspectFlag is set
            Py_DontWriteBytecodeFlag = 1; // Do not generate precompiled bytecode

            const bool initializeSignalHandlers = true;
            pybind11::initialize_interpreter(initializeSignalHandlers);

            // display basic Python information
            AZ_Trace("python", "Py_GetVersion=%s \n", Py_GetVersion());
            AZ_Trace("python", "Py_GetPath=%ls \n", Py_GetPath());
            AZ_Trace("python", "Py_GetExecPrefix=%ls \n", Py_GetExecPrefix());
            AZ_Trace("python", "Py_GetProgramFullPath=%ls \n", Py_GetProgramFullPath());

            // Add custom site packages after initializing the interpreter above.  Calling Py_SetPath before initialization
            // alters the behavior of the initializer to not compute default search paths. See https://docs.python.org/3/c-api/init.html#c.Py_SetPath
            if (pyPackageSites.size())
            {
                ExtendSysPath(pyPackageSites);
            }
            RedirectOutput::Intialize(PyImport_ImportModule("azlmbr_redirect"));

            // Acquire GIL before calling Python code
            PythonGILScopedLock lock(m_lock, m_lockRecursiveCounter);

            if (EditorPythonBindings::PythonSymbolEventBus::GetTotalNumOfEventHandlers() == 0)
            {
                m_symbolLogHelper = AZStd::make_shared<PythonSystemComponent::SymbolLogHelper>();
            }

            // print Python version using AZ logging
            const int verRet = PyRun_SimpleStringFlags("import sys \nprint (sys.version) \n", nullptr);
            AZ_Error("python", verRet == 0, "Error trying to fetch the version number in Python!");
            return verRet == 0 && !PyErr_Occurred();
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("python", false, "Py_Initialize() failed with %s!", e.what());
            return false;
        }
    }

    bool PythonSystemComponent::ExtendSysPath(const AZStd::unordered_set<AZStd::string>& extendPaths)
    {
        AZStd::unordered_set<AZStd::string> oldPathSet;
        auto SplitPath = [&oldPathSet](AZStd::string_view pathPart)
        {
            oldPathSet.emplace(pathPart);
        };
        AZ::StringFunc::TokenizeVisitor(Py_EncodeLocale(Py_GetPath(), nullptr), SplitPath, DELIM);
        bool appended{ false };
        AZStd::string pathAppend{ "import sys\n" };
        for (const auto& thisStr : extendPaths)
        {
            if (!oldPathSet.contains(thisStr))
            {
                pathAppend.append(AZStd::string::format("sys.path.append(r'%s')\n", thisStr.c_str()));
                appended = true;
            }
        }
        if (appended)
        {
            ExecuteByString(pathAppend.c_str(), false);
            return true;
        }
        return false;
    }

    bool PythonSystemComponent::StopPythonInterpreter()
    {
        if (Py_IsInitialized())
        {
            RedirectOutput::Shutdown();
            pybind11::finalize_interpreter();
        }
        else
        {
            AZ_Warning("python", false, "Did not finalize since Py_IsInitialized() was false.");
        }
        return true;
    }

    void PythonSystemComponent::ExecuteByString(AZStd::string_view script, bool printResult)
    {
        if (!Py_IsInitialized())
        {
            AZ_Error("python", false, "Can not ExecuteByString() since the embeded Python VM is not ready.");
            return;
        }

        if (!script.empty())
        {
            AzToolsFramework::EditorPythonScriptNotificationsBus::Broadcast(
                &AzToolsFramework::EditorPythonScriptNotificationsBus::Events::OnStartExecuteByString, script);

            // Acquire GIL before calling Python code
            PythonGILScopedLock lock(m_lock, m_lockRecursiveCounter);

            // Acquire scope for __main__ for executing our script
            pybind11::object scope = pybind11::module::import("__main__").attr("__dict__");

            bool shouldPrintValue = false;

            if (printResult)
            {
                // Attempt to compile our code to determine if it's an expression
                // i.e. a Python code object with only an rvalue
                // If it is, it can be evaled to produce a PyObject
                // If it's not, we can't evaluate it into a result and should fall back to exec
                shouldPrintValue = true;

                using namespace pybind11::literals;

                // codeop.compile_command is a thin wrapper around the Python compile builtin
                // We attempt to compile using symbol="eval" to see if the string is valid for eval
                // This is similar to what the Python REPL does internally
                pybind11::object codeop = pybind11::module::import("codeop");
                pybind11::object compileCommand = codeop.attr("compile_command");
                try
                {
                    compileCommand(script.data(), "symbol"_a="eval");
                }
                catch (const pybind11::error_already_set&)
                {
                    shouldPrintValue = false;
                }
            }

            try
            {
                if (shouldPrintValue)
                {
                    // We're an expression, run and print the result
                    pybind11::object result = pybind11::eval(script.data(), scope);
                    pybind11::print(result);
                }
                else
                {
                    // Just exec the code block
                    pybind11::exec(script.data(), scope);
                }
            }
            catch (pybind11::error_already_set& pythonError)
            {
                // Release the exception stack and let Python print it to stderr
                pythonError.restore();
                PyErr_Print();
            }
        }
    }

    bool PythonSystemComponent::ExecuteByFilename(AZStd::string_view filename)
    {
        AZStd::vector<AZStd::string_view> args;
        AzToolsFramework::EditorPythonScriptNotificationsBus::Broadcast(
            &AzToolsFramework::EditorPythonScriptNotificationsBus::Events::OnStartExecuteByFilename, filename);
        return ExecuteByFilenameWithArgs(filename, args);
    }

    bool PythonSystemComponent::ExecuteByFilenameAsTest(AZStd::string_view filename, AZStd::string_view testCase, const AZStd::vector<AZStd::string_view>& args)
    {
        AZ_TracePrintf("python", "Running automated test: %.*s (testcase %.*s)", AZ_STRING_ARG(filename), AZ_STRING_ARG(testCase))
        AzToolsFramework::EditorPythonScriptNotificationsBus::Broadcast(
            &AzToolsFramework::EditorPythonScriptNotificationsBus::Events::OnStartExecuteByFilenameAsTest, filename, testCase, args);
        const Result evalResult = EvaluateFile(filename, args);
        return evalResult == Result::Okay;
    }

    bool PythonSystemComponent::ExecuteByFilenameWithArgs(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args)
    {
        AzToolsFramework::EditorPythonScriptNotificationsBus::Broadcast(
            &AzToolsFramework::EditorPythonScriptNotificationsBus::Events::OnStartExecuteByFilenameWithArgs, filename, args);
        const Result result = EvaluateFile(filename, args);
        return result == Result::Okay;
    }

    PythonSystemComponent::Result PythonSystemComponent::EvaluateFile(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args)
    {
        if (!Py_IsInitialized())
        {
            AZ_Error("python", false, "Can not evaluate file since the embedded Python VM is not ready.");
            return Result::Error_IsNotInitialized;
        }

        if (filename.empty())
        {
            AZ_Error("python", false, "Invalid empty filename detected.");
            return Result::Error_InvalidFilename;
        }

        // support the alias version of a script such as @engroot@/Editor/Scripts/select_story_anim_objects.py
        AZStd::string theFilename(filename);
        {
            char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(theFilename.c_str(), resolvedPath, AZ_MAX_PATH_LEN);
            theFilename = resolvedPath;
        }

        if (!AZ::IO::FileIOBase::GetInstance()->Exists(theFilename.c_str()))
        {
            AZ_Error("python", false, "Missing Python file named (%s)", theFilename.c_str());
            return Result::Error_MissingFile;
        }

        FILE* file = nullptr;
        azfopen(&file, theFilename.c_str(), "rb");

        if (!file)
        {
            AZ_Error("python", false, "Missing Python file named (%s)", theFilename.c_str());
            return Result::Error_FileOpenValidation;
        }

        Result pythonScriptResult = Result::Okay;
        try
        {
            // Acquire GIL before calling Python code
            PythonGILScopedLock lock(m_lock, m_lockRecursiveCounter);

            // Create standard "argc" / "argv" command-line parameters to pass in to the Python script via sys.argv.
            // argc = number of parameters.  This will always be at least 1, since the first parameter is the script name.
            // argv = the list of parameters, in wchar format.
            // Our expectation is that the args passed into this function does *not* already contain the script name.
            int argc = aznumeric_cast<int>(args.size()) + 1;

            // Note:  This allocates from PyMem to ensure that Python has access to the memory.
            wchar_t** argv = static_cast<wchar_t**>(PyMem_Malloc(argc * sizeof(wchar_t*)));

            // Python 3.x is expecting wchar* strings for the command-line args.
            argv[0] = Py_DecodeLocale(theFilename.c_str(), nullptr);

            for (int arg = 0; arg < args.size(); arg++)
            {
                AZStd::string argString(args[arg]);
                argv[arg + 1] = Py_DecodeLocale(argString.c_str(), nullptr);
            }
            // Tell Python the command-line args.
            // Note that this has a side effect of adding the script's path to the set of directories checked for "import" commands.
            const int updatePath = 1;
            PySys_SetArgvEx(argc, argv, updatePath);

            Py_DontWriteBytecodeFlag = 1;
            PyCompilerFlags flags;
            flags.cf_flags = 0;
            const int bAutoCloseFile = true;
            const int returnCode = PyRun_SimpleFileExFlags(file, theFilename.c_str(), bAutoCloseFile, &flags);
            if (returnCode != 0)
            {
                AZStd::string message = AZStd::string::format("Detected script failure in Python script(%s); return code %d!", theFilename.c_str(), returnCode);
                AZ_Warning("python", false, message.c_str());
                using namespace AzToolsFramework;
                EditorPythonConsoleNotificationBus::Broadcast(&EditorPythonConsoleNotificationBus::Events::OnExceptionMessage, message.c_str());
                pythonScriptResult = Result::Error_PythonException;
            }

            // Free any memory allocated for the command-line args.
            for (int arg = 0; arg < argc; arg++)
            {
                PyMem_RawFree(argv[arg]);
            }
            PyMem_Free(argv);
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("python", false, "Detected an internal exception %s while running script (%s)!", e.what(), theFilename.c_str());
            return Result::Error_InternalException;
        }
        return pythonScriptResult;
    }

} // namespace EditorPythonBindings
