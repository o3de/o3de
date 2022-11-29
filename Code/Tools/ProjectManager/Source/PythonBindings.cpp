/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PythonBindings.h>

#include <ProjectManagerDefs.h>

// Qt defines slots, which interferes with the use here.
#pragma push_macro("slots")
#undef slots
#include <pybind11/functional.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/stl.h>
#pragma pop_macro("slots")

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/numeric.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/sort.h>

#include <QDir>

namespace Platform
{
    bool InsertPythonLibraryPath(
        AZStd::unordered_set<AZStd::string>& paths, const char* pythonPackage, const char* engineRoot, const char* subPath)
    {
        // append lib path to Python paths
        AZ::IO::FixedMaxPath libPath = engineRoot;
        libPath /= AZ::IO::FixedMaxPathString::format(subPath, pythonPackage);
        libPath = libPath.LexicallyNormal();
        if (AZ::IO::SystemFile::Exists(libPath.c_str()))
        {
            paths.insert(libPath.c_str());
            return true;
        }

        AZ_Warning("python", false, "Python library path should exist. path:%s", libPath.c_str());
        return false;
    }

    // Implemented in each different platform's PAL implementation files, as it differs per platform.
    AZStd::string GetPythonHomePath(const char* pythonPackage, const char* engineRoot);

} // namespace Platform

#define Py_To_String(obj) pybind11::str(obj).cast<std::string>().c_str()
#define Py_To_String_Optional(dict, key, default_string) dict.contains(key) ? Py_To_String(dict[key]) : default_string
#define Py_To_Int(obj) obj.cast<int>()
#define Py_To_Int_Optional(dict, key, default_int) dict.contains(key) ? Py_To_Int(dict[key]) : default_int
#define QString_To_Py_String(value) pybind11::str(value.toStdString())
#define QString_To_Py_Path(value) m_pathlib.attr("Path")(value.toStdString())

pybind11::list QStringList_To_Py_List(const QStringList& values)
{
    std::list<std::string> newValues;
    for (const auto& i : values)
    {
        newValues.push_back(i.toStdString());
    }
    return pybind11::list(pybind11::cast(newValues));
};

namespace RedirectOutput
{
    using RedirectOutputFunc = AZStd::function<void(const char*)>;

    struct RedirectOutput
    {
        PyObject_HEAD RedirectOutputFunc write;
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

    PyObject* RedirectFlush([[maybe_unused]] PyObject* self,[[maybe_unused]] PyObject* args)
    {
        // no-op
        return Py_BuildValue("");
    }

    PyMethodDef RedirectMethods[] = {
        {"write", RedirectWrite, METH_VARARGS, "sys.stdout.write"},
        {"flush", RedirectFlush, METH_VARARGS, "sys.stdout.flush"},
        {"write", RedirectWrite, METH_VARARGS, "sys.stderr.write"},
        {"flush", RedirectFlush, METH_VARARGS, "sys.stderr.flush"},
        {0, 0, 0, 0} // sentinel
    };

    PyTypeObject RedirectOutputType = {
        PyVarObject_HEAD_INIT(0, 0) "azlmbr_redirect.RedirectOutputType", // tp_name
        sizeof(RedirectOutput), /* tp_basicsize */
        0, /* tp_itemsize */
        0, /* tp_dealloc */
        0, /* tp_print */
        0, /* tp_getattr */
        0, /* tp_setattr */
        0, /* tp_reserved */
        0, /* tp_repr */
        0, /* tp_as_number */
        0, /* tp_as_sequence */
        0, /* tp_as_mapping */
        0, /* tp_hash  */
        0, /* tp_call */
        0, /* tp_str */
        0, /* tp_getattro */
        0, /* tp_setattro */
        0, /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT, /* tp_flags */
        "azlmbr_redirect objects", /* tp_doc */
        0, /* tp_traverse */
        0, /* tp_clear */
        0, /* tp_richcompare */
        0, /* tp_weaklistoffset */
        0, /* tp_iter */
        0, /* tp_iternext */
        RedirectMethods, /* tp_methods */
        0, /* tp_members */
        0, /* tp_getset */
        0, /* tp_base */
        0, /* tp_dict */
        0, /* tp_descr_get */
        0, /* tp_descr_set */
        0, /* tp_dictoffset */
        0, /* tp_init */
        0, /* tp_alloc */
        0 /* tp_new */
    };

    PyModuleDef RedirectOutputModule = {
        PyModuleDef_HEAD_INIT, "azlmbr_redirect", 0, -1, 0,
    };

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

        PyObject* redirectModule = PyModule_Create(&RedirectOutputModule);
        if (redirectModule)
        {
            Py_INCREF(&RedirectOutputType);
            PyModule_AddObject(redirectModule, "Redirect", reinterpret_cast<PyObject*>(&RedirectOutputType));
        }
        return redirectModule;
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

    void Intialize(PyObject* module, RedirectOutputFunc stdoutFunction, RedirectOutputFunc stderrFunction)
    {
        s_RedirectModule = module;

        SetRedirection("stdout", g_redirect_stdout_saved, g_redirect_stdout, stdoutFunction);
        SetRedirection("stderr", g_redirect_stderr_saved, g_redirect_stderr, stderrFunction);

        PySys_WriteStdout("RedirectOutput installed\n");
    }

    void Shutdown()
    {
        ResetRedirection("stdout", g_redirect_stdout_saved, g_redirect_stdout);
        ResetRedirection("stderr", g_redirect_stderr_saved, g_redirect_stderr);
        Py_XDECREF(s_RedirectModule);
        s_RedirectModule = nullptr;
    }
} // namespace RedirectOutput


namespace O3DE::ProjectManager
{
    PythonBindings::PythonBindings(const AZ::IO::PathView& enginePath)
        : m_enginePath(enginePath)
    {
        m_pythonStarted = StartPython();
    }

    PythonBindings::~PythonBindings()
    {
        if (m_pythonStarted)
        {
            StopPython();
        }
    }

    void PythonBindings::OnStdOut(const char* msg)
    {
        AZ::Debug::Trace::Instance().Output("Python", msg);
    }

    void PythonBindings::OnStdError(const char* msg)
    {
        AZStd::string_view lastPythonError{ msg };
        if (constexpr AZStd::string_view pythonErrorPrefix = "ERROR:root:";
            lastPythonError.starts_with(pythonErrorPrefix))
        {
            lastPythonError = lastPythonError.substr(pythonErrorPrefix.size());
        }

        PythonBindingsInterface::Get()->AddErrorString(lastPythonError);

        AZ::Debug::Trace::Instance().Output("Python", msg);
    }

    bool PythonBindings::PythonStarted()
    {
        return m_pythonStarted && Py_IsInitialized();
    }

    bool PythonBindings::StartPython()
    {
        if (Py_IsInitialized())
        {
            AZ_Warning("python", false, "Python is already active");
            return m_pythonStarted;
        }

        m_pythonStarted = false;

        // set PYTHON_HOME
        AZStd::string pyBasePath = Platform::GetPythonHomePath(PY_PACKAGE, m_enginePath.c_str());
        if (!AZ::IO::SystemFile::Exists(pyBasePath.c_str()))
        {
            AZ_Error("python", false, "Python home path does not exist: %s", pyBasePath.c_str());
            return false;
        }

        AZStd::wstring pyHomePath;
        AZStd::to_wstring(pyHomePath, pyBasePath);
        Py_SetPythonHome(pyHomePath.c_str());

        // display basic Python information
        AZ_TracePrintf("python", "Py_GetVersion=%s \n", Py_GetVersion());
        AZ_TracePrintf("python", "Py_GetPath=%ls \n", Py_GetPath());
        AZ_TracePrintf("python", "Py_GetExecPrefix=%ls \n", Py_GetExecPrefix());
        AZ_TracePrintf("python", "Py_GetProgramFullPath=%ls \n", Py_GetProgramFullPath());

        PyImport_AppendInittab("azlmbr_redirect", RedirectOutput::PyInit_RedirectOutput);

        try
        {
            // ignore system location for sites site-packages
            Py_IsolatedFlag = 1; // -I - Also sets Py_NoUserSiteDirectory.  If removed PyNoUserSiteDirectory should be set.
            Py_IgnoreEnvironmentFlag = 1; // -E

            const bool initializeSignalHandlers = true;
            pybind11::initialize_interpreter(initializeSignalHandlers);

            RedirectOutput::Intialize(PyImport_ImportModule("azlmbr_redirect"), &PythonBindings::OnStdOut, &PythonBindings::OnStdError);

            // Acquire GIL before calling Python code
            AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
            pybind11::gil_scoped_acquire acquire;

            // sanity import check
            if (PyRun_SimpleString("import sys") != 0)
            {
                AZ_Assert(false, "Import sys failed");
                return false;
            }

            // import required modules
            m_cmake = pybind11::module::import("o3de.cmake");
            m_register = pybind11::module::import("o3de.register");
            m_manifest = pybind11::module::import("o3de.manifest");
            m_engineTemplate = pybind11::module::import("o3de.engine_template");
            m_gemProperties = pybind11::module::import("o3de.gem_properties");
            m_engineProperties = pybind11::module::import("o3de.engine_properties");
            m_enableGemProject = pybind11::module::import("o3de.enable_gem");
            m_disableGemProject = pybind11::module::import("o3de.disable_gem");
            m_editProjectProperties = pybind11::module::import("o3de.project_properties");
            m_download = pybind11::module::import("o3de.download");
            m_repo = pybind11::module::import("o3de.repo");
            m_pathlib = pybind11::module::import("pathlib");

            m_pythonStarted = !PyErr_Occurred();
            return m_pythonStarted;
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Assert(false, "Py_Initialize() failed with %s", e.what());
            return false;
        }
    }

    bool PythonBindings::StopPython()
    {
        if (Py_IsInitialized())
        {
            RedirectOutput::Shutdown();
            pybind11::finalize_interpreter();
        }

        return Py_IsInitialized() == 0;
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::ExecuteWithLockErrorHandling(AZStd::function<void()> executionCallback) const
    {
        if (!Py_IsInitialized())
        {
            return AZ::Failure<AZStd::string>("Python is not initialized");
        }

        AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
        pybind11::gil_scoped_release release;
        pybind11::gil_scoped_acquire acquire;

        m_pythonErrorStrings.clear();

        try
        {
            executionCallback();
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("PythonBindings", false, "Python exception %s", e.what());
            return AZ::Failure<AZStd::string>(e.what());
        }

        return AZ::Success();
    }

    bool PythonBindings::ExecuteWithLock(AZStd::function<void()> executionCallback) const
    {
        return ExecuteWithLockErrorHandling(executionCallback).IsSuccess();
    }

    EngineInfo PythonBindings::EngineInfoFromPath(pybind11::handle enginePath)
    {
        EngineInfo engineInfo;
        try
        {
            auto engineData = m_manifest.attr("get_engine_json_data")(pybind11::none(), enginePath);
            if (pybind11::isinstance<pybind11::dict>(engineData))
            {
                engineInfo.m_version = Py_To_String_Optional(engineData, "O3DEVersion", "0.0.0.0");
                engineInfo.m_name = Py_To_String_Optional(engineData, "engine_name", "O3DE");
                engineInfo.m_path = Py_To_String(enginePath);

                auto thisEnginePath = m_manifest.attr("get_this_engine_path")();
                if (pybind11::isinstance(thisEnginePath, m_pathlib.attr("Path")().get_type()) &&
                    thisEnginePath.attr("samefile")(enginePath).cast<bool>())
                {
                    engineInfo.m_thisEngine = true;
                }
            }

            auto o3deData = m_manifest.attr("load_o3de_manifest")();
            if (pybind11::isinstance<pybind11::dict>(o3deData))
            {
                auto defaultGemsFolder = m_manifest.attr("get_o3de_gems_folder")();
                engineInfo.m_defaultGemsFolder = Py_To_String_Optional(o3deData, "default_gems_folder", Py_To_String(defaultGemsFolder));

                auto defaultProjectsFolder = m_manifest.attr("get_o3de_projects_folder")();
                engineInfo.m_defaultProjectsFolder = Py_To_String_Optional(o3deData, "default_projects_folder", Py_To_String(defaultProjectsFolder));

                auto defaultRestrictedFolder = m_manifest.attr("get_o3de_restricted_folder")();
                engineInfo.m_defaultRestrictedFolder = Py_To_String_Optional(o3deData, "default_restricted_folder", Py_To_String(defaultRestrictedFolder));

                auto defaultTemplatesFolder = m_manifest.attr("get_o3de_templates_folder")();
                engineInfo.m_defaultTemplatesFolder = Py_To_String_Optional(o3deData, "default_templates_folder", Py_To_String(defaultTemplatesFolder));

                auto defaultThirdPartyFolder = m_manifest.attr("get_o3de_third_party_folder")();
                engineInfo.m_thirdPartyPath = Py_To_String_Optional(o3deData, "default_third_party_folder", Py_To_String(defaultThirdPartyFolder));
            }


            // check if engine path is registered
            auto allEngines = m_manifest.attr("get_manifest_engines")();
            if (pybind11::isinstance<pybind11::list>(allEngines))
            {
                const AZ::IO::FixedMaxPath enginePathFixed(Py_To_String(enginePath));
                for (auto engine : allEngines)
                {
                    AZ::IO::FixedMaxPath otherEnginePath(Py_To_String(engine));
                    if (otherEnginePath.Compare(enginePathFixed) == 0)
                    {
                        engineInfo.m_registered = true;
                        break;
                    }
                }
            }
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("PythonBindings", false, "Failed to get EngineInfo from %s", Py_To_String(enginePath));
        }
        return engineInfo;
    }

    AZ::Outcome<QVector<EngineInfo>> PythonBindings::GetAllEngineInfos()
    {
        QVector<EngineInfo> engines;

        auto result = ExecuteWithLockErrorHandling([&]
        {
            for (auto path : m_manifest.attr("get_manifest_engines")())
            {
                engines.push_back(EngineInfoFromPath(path));
            }
        });

        if (!result.IsSuccess())
        {
            return AZ::Failure();
        }

        AZStd::sort(engines.begin(), engines.end());
        return AZ::Success(AZStd::move(engines));
    }

    AZ::Outcome<EngineInfo> PythonBindings::GetEngineInfo()
    {
        EngineInfo engineInfo;

        bool result = ExecuteWithLock([&] {
            auto enginePath = m_manifest.attr("get_this_engine_path")();
            engineInfo = EngineInfoFromPath(enginePath);
        });

        if (!result || !engineInfo.IsValid())
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(engineInfo));
        }
    }

    AZ::Outcome<EngineInfo> PythonBindings::GetEngineInfo(const QString& engineName)
    {
        EngineInfo engineInfo;
        bool result = ExecuteWithLock([&] {
            auto enginePathResult = m_manifest.attr("get_registered")(QString_To_Py_String(engineName));

            // if a valid registered object is not found None is returned
            if (!pybind11::isinstance<pybind11::none>(enginePathResult))
            {
                engineInfo = EngineInfoFromPath(enginePathResult);

                // it is possible an engine is registered in o3de_manifest.json but the engine.json is
                // missing or corrupt in which case we do not consider it a registered engine
            }
        });

        if (!result || !engineInfo.IsValid())
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(engineInfo));
        }
    }

    AZ::Outcome<EngineInfo> PythonBindings::GetProjectEngine(const QString& projectPath)
    {
        EngineInfo engineInfo;
        bool result = ExecuteWithLock([&] {
            auto enginePathResult = m_manifest.attr("get_project_engine_path")(QString_To_Py_Path(projectPath));

            // if a valid registered object is not found None is returned
            if (!pybind11::isinstance<pybind11::none>(enginePathResult))
            {
                // request a posix path so it looks like what is in o3de_manifest.json
                engineInfo = EngineInfoFromPath(enginePathResult.attr("as_posix")());
            }
        });

        if (!result || !engineInfo.IsValid())
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(engineInfo));
        }
    }

    IPythonBindings::DetailedOutcome PythonBindings::SetEngineInfo(const EngineInfo& engineInfo, bool force)
    {
        using namespace pybind11::literals;

        bool registrationSuccess = false;
        bool pythonSuccess = ExecuteWithLock([&] {

            EngineInfo currentEngine = EngineInfoFromPath(QString_To_Py_Path(engineInfo.m_path));

            // be kind to source control and avoid needlessly updating engine.json
            if (currentEngine.IsValid() &&
                (currentEngine.m_name.compare(engineInfo.m_name) != 0 || currentEngine.m_version.compare(engineInfo.m_version) != 0))
            {
                auto enginePropsResult = m_engineProperties.attr("edit_engine_props")(
                    QString_To_Py_Path(engineInfo.m_path),
                    pybind11::none(), // existing engine_name
                    QString_To_Py_String(engineInfo.m_name),
                    QString_To_Py_String(engineInfo.m_version)
                );

                if (enginePropsResult.cast<int>() != 0)
                {
                    // do not proceed with registration
                    return;
                }
            }

            auto result = m_register.attr("register")(
                "engine_path"_a                  = QString_To_Py_Path(engineInfo.m_path),
                "project_path"_a                 = pybind11::none(),
                "gem_path"_a                     = pybind11::none(),
                "external_subdir_path"_a         = pybind11::none(),
                "template_path"_a                = pybind11::none(),
                "restricted_path"_a              = pybind11::none(),
                "repo_uri"_a                     = pybind11::none(),
                "default_engines_folder"_a       = pybind11::none(),
                "default_projects_folder"_a      = QString_To_Py_Path(engineInfo.m_defaultProjectsFolder),
                "default_gems_folder"_a          = QString_To_Py_Path(engineInfo.m_defaultGemsFolder),
                "default_templates_folder"_a     = QString_To_Py_Path(engineInfo.m_defaultTemplatesFolder),
                "default_restricted_folder"_a    = pybind11::none(),
                "default_third_party_folder"_a   = QString_To_Py_Path(engineInfo.m_thirdPartyPath),
                "external_subdir_engine_path"_a  = pybind11::none(),
                "external_subdir_project_path"_a = pybind11::none(),
                "external_subdir_gem_path"_a     = pybind11::none(),
                "remove"_a                       = false,
                "force"_a                        = force
                );

            registrationSuccess = result.cast<int>() == 0;
        });

        if (pythonSuccess && registrationSuccess)
        {
            return AZ::Success();
        }

        return AZ::Failure<ErrorPair>(GetErrorPair());
    }

    bool PythonBindings::ValidateRepository(const QString& repoUri)
    {
        bool validateResult = false;
        bool result = ExecuteWithLock(
            [&]
            {
                auto pythonRepoUri = QString_To_Py_String(repoUri);
                auto pythonValidationResult = m_repo.attr("validate_remote_repo")(pythonRepoUri, true);

                validateResult = pythonValidationResult.cast<bool>();
            });

        return result && validateResult;
    }

    AZ::Outcome<GemInfo> PythonBindings::GetGemInfo(const QString& path, const QString& projectPath)
    {
        GemInfo gemInfo = GemInfoFromPath(QString_To_Py_String(path), QString_To_Py_Path(projectPath));
        if (gemInfo.IsValid())
        {
            return AZ::Success(AZStd::move(gemInfo));
        }
        else
        {
            return AZ::Failure();
        }
    }

    AZ::Outcome<QVector<GemInfo>, AZStd::string> PythonBindings::GetEngineGemInfos()
    {
        QVector<GemInfo> gems;

        auto result = ExecuteWithLockErrorHandling([&]
        {
            for (auto path : m_manifest.attr("get_engine_gems")())
            {
                gems.push_back(GemInfoFromPath(path, pybind11::none()));
            }
        });
        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }

        AZStd::sort(gems.begin(), gems.end());
        return AZ::Success(AZStd::move(gems));
    }

    AZ::Outcome<QVector<GemInfo>, AZStd::string> PythonBindings::GetAllGemInfos(const QString& projectPath)
    {
        QVector<GemInfo> gems;

        auto result = ExecuteWithLockErrorHandling([&]
        {
            auto pyProjectPath = QString_To_Py_Path(projectPath);
            for (auto path : m_manifest.attr("get_all_gems")(pyProjectPath))
            {
                GemInfo gemInfo = GemInfoFromPath(path, pyProjectPath);
                // Mark as downloaded because this gem was registered with an existing directory
                gemInfo.m_downloadStatus = GemInfo::DownloadStatus::Downloaded;

                gems.push_back(AZStd::move(gemInfo));
            }
        });
        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }

        AZStd::sort(gems.begin(), gems.end());
        return AZ::Success(AZStd::move(gems));
    }

    AZ::Outcome<QVector<AZStd::string>, AZStd::string> PythonBindings::GetEnabledGemNames(const QString& projectPath) const
    {
        // Retrieve the path to the cmake file that lists the enabled gems.
        pybind11::str enabledGemsFilename;
        auto result = ExecuteWithLockErrorHandling([&]
        {
            enabledGemsFilename = m_cmake.attr("get_enabled_gem_cmake_file")(
                pybind11::none(), // project_name
                QString_To_Py_Path(projectPath)); // project_path
        });
        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }

        // Retrieve the actual list of names from the cmake file.
        QVector<AZStd::string> gemNames;
        result = ExecuteWithLockErrorHandling([&]
        {
            const auto pyGemNames = m_cmake.attr("get_enabled_gems")(enabledGemsFilename);
            for (auto gemName : pyGemNames)
            {
                gemNames.push_back(Py_To_String(gemName));
            }
        });
        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }

        return AZ::Success(AZStd::move(gemNames));
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::GemRegistration(const QString& gemPath, const QString& projectPath, bool remove)
    {
        using namespace pybind11::literals;

        bool registrationResult = false;
        auto result = ExecuteWithLockErrorHandling(
            [&]
        {
            auto externalProjectPath = projectPath.isEmpty() ? pybind11::none() : QString_To_Py_Path(projectPath);
            auto pythonRegistrationResult = m_register.attr("register")(
                "engine_path"_a                  = pybind11::none(),
                "project_path"_a                 = pybind11::none(),
                "gem_path"_a                     = QString_To_Py_Path(gemPath),
                "external_subdir_path"_a         = pybind11::none(),
                "template_path"_a                = pybind11::none(),
                "restricted_path"_a              = pybind11::none(),
                "repo_uri"_a                     = pybind11::none(),
                "default_engines_folder"_a       = pybind11::none(),
                "default_projects_folder"_a      = pybind11::none(),
                "default_gems_folder"_a          = pybind11::none(),
                "default_templates_folder"_a     = pybind11::none(),
                "default_restricted_folder"_a    = pybind11::none(),
                "default_third_party_folder"_a   = pybind11::none(),
                "external_subdir_engine_path"_a  = pybind11::none(),
                "external_subdir_project_path"_a = externalProjectPath,
                "external_subdir_gem_path"_a     = pybind11::none(),
                "remove"_a = remove
                );



            // Returns an exit code so boolify it then invert result
            registrationResult = !pythonRegistrationResult.cast<bool>();
        });

        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }
        else if (!registrationResult)
        {
            return AZ::Failure<AZStd::string>(AZStd::string::format(
                "Failed to %s gem path %s", remove ? "unregister" : "register", gemPath.toUtf8().constData()));
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::RegisterGem(const QString& gemPath, const QString& projectPath)
    {
        return GemRegistration(gemPath, projectPath);
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::UnregisterGem(const QString& gemPath, const QString& projectPath)
    {
        return GemRegistration(gemPath, projectPath, /*remove*/true);
    }

    bool PythonBindings::AddProject(const QString& path)
    {
        bool registrationResult = false;
        bool result = ExecuteWithLock(
            [&]
        {
            auto projectPath = QString_To_Py_Path(path);
            auto pythonRegistrationResult = m_register.attr("register")(pybind11::none(), projectPath);

            // Returns an exit code so boolify it then invert result
            registrationResult = !pythonRegistrationResult.cast<bool>();
        });

        return result && registrationResult;
    }

    bool PythonBindings::RemoveProject(const QString& path)
    {
        using namespace pybind11::literals;

        bool registrationResult = false;
        bool result = ExecuteWithLock(
            [&]
        {
            auto pythonRegistrationResult = m_register.attr("register")(
                "engine_path"_a                  = pybind11::none(),
                "project_path"_a                 = QString_To_Py_Path(path),
                "gem_path"_a                     = pybind11::none(),
                "external_subdir_path"_a         = pybind11::none(),
                "template_path"_a                = pybind11::none(),
                "restricted_path"_a              = pybind11::none(),
                "repo_uri"_a                     = pybind11::none(),
                "default_engines_folder"_a       = pybind11::none(),
                "default_projects_folder"_a      = pybind11::none(),
                "default_gems_folder"_a          = pybind11::none(),
                "default_templates_folder"_a     = pybind11::none(),
                "default_restricted_folder"_a    = pybind11::none(),
                "default_third_party_folder"_a   = pybind11::none(),
                "external_subdir_engine_path"_a  = pybind11::none(),
                "external_subdir_project_path"_a = pybind11::none(),
                "external_subdir_gem_path"_a     = pybind11::none(),
                "remove"_a                       = true,
                "force"_a                        = false
                );

            // Returns an exit code so boolify it then invert result
            registrationResult = !pythonRegistrationResult.cast<bool>();
        });

        return result && registrationResult;
    }

    AZ::Outcome<ProjectInfo> PythonBindings::CreateProject(const QString& projectTemplatePath, const ProjectInfo& projectInfo, bool registerProject)
    {
        using namespace pybind11::literals;

        ProjectInfo createdProjectInfo;
        bool result = ExecuteWithLock([&] {
            auto projectPath = QString_To_Py_Path(projectInfo.m_path);

            auto createProjectResult = m_engineTemplate.attr("create_project")(
                "project_path"_a = projectPath,
                "project_name"_a = QString_To_Py_String(projectInfo.m_projectName),
                "template_path"_a = QString_To_Py_Path(projectTemplatePath),
                "no_register"_a = !registerProject
            );
            if (createProjectResult.cast<int>() == 0)
            {
                createdProjectInfo = ProjectInfoFromPath(projectPath);
            }
        });

        if (!result || !createdProjectInfo.IsValid())
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(createdProjectInfo));
        }
    }

    AZ::Outcome<GemInfo> PythonBindings::CreateGem(const QString& templatePath, const GemInfo& gemInfo, bool registerGem/*=true*/)
    {
        using namespace pybind11::literals;

        GemInfo gemInfoResult;
        bool result = ExecuteWithLock(
            [&]
            {
                auto gemPath = QString_To_Py_Path(gemInfo.m_path);

                auto createGemResult = m_engineTemplate.attr("create_gem")(
                    "gem_path"_a = gemPath,
                    "template_path"_a = QString_To_Py_Path(templatePath),
                    "gem_name"_a = QString_To_Py_String(gemInfo.m_name),
                    "display_name"_a = QString_To_Py_String(gemInfo.m_displayName),
                    "summary"_a = QString_To_Py_String(gemInfo.m_summary),
                    "requirements"_a = QString_To_Py_String(gemInfo.m_requirement),
                    "license"_a = QString_To_Py_String(gemInfo.m_licenseText),
                    "license_url"_a = QString_To_Py_String(gemInfo.m_licenseLink),
                    "origin"_a = QString_To_Py_String(gemInfo.m_origin),
                    "origin_url"_a = QString_To_Py_String(gemInfo.m_originURL),
                    "user_tags"_a = QString_To_Py_String(gemInfo.m_features.join(",")),
                    "platforms"_a = QStringList_To_Py_List(gemInfo.GetPlatformsAsStringList()),
                    "icon_path"_a = QString_To_Py_Path(gemInfo.m_iconPath),
                    "documentation_url"_a = QString_To_Py_String(gemInfo.m_documentationLink),
                    "repo_uri"_a = QString_To_Py_String(gemInfo.m_repoUri),
                    "no_register"_a = !registerGem)
                    ;
                if (createGemResult.cast<int>() == 0)
                {
                    gemInfoResult = GemInfoFromPath(gemPath, pybind11::none());
                }

            });

        if (!result || !gemInfoResult.IsValid())
        {
            return AZ::Failure();
        }
        else
        {
            //Make sure directory link is a normalized path that can be rendered in "View Directory" dialog
            gemInfoResult.m_directoryLink = QDir::cleanPath(gemInfoResult.m_directoryLink);
            return AZ::Success(AZStd::move(gemInfoResult));
        }
    }

    AZ::Outcome<GemInfo> PythonBindings::EditGem(const QString& oldGemName, const GemInfo& newGemInfo)
    {
        using namespace pybind11::literals;

        GemInfo gemInfoResult;
        bool result = ExecuteWithLock(
            [&]
            {
                auto gemPath = QString_To_Py_Path(newGemInfo.m_path);

                auto editGemResult = m_gemProperties.attr("edit_gem_props")(
                    "gem_path"_a = gemPath,
                    "gem_name"_a = QString_To_Py_String(oldGemName),
                    "new_name"_a = QString_To_Py_String(newGemInfo.m_name),
                    "new_display"_a = QString_To_Py_String(newGemInfo.m_displayName),
                    "new_origin"_a = QString_To_Py_String(newGemInfo.m_origin),
                    "new_summary"_a = QString_To_Py_String(newGemInfo.m_summary),
                    "new_icon"_a = QString_To_Py_String(newGemInfo.m_iconPath),
                    "new_requirements"_a = QString_To_Py_String(newGemInfo.m_requirement),
                    "new_documentation_url"_a = QString_To_Py_String(newGemInfo.m_documentationLink),
                    "new_license"_a = QString_To_Py_String(newGemInfo.m_licenseText),
                    "new_license_url"_a = QString_To_Py_String(newGemInfo.m_licenseLink),
                    "new_repo_uri"_a = QString_To_Py_String(newGemInfo.m_repoUri),
                    "replace_tags"_a = QStringList_To_Py_List(newGemInfo.m_features), //the python code seems to interpret these lists as space separated
                    "replace_platforms"_a = QStringList_To_Py_List(newGemInfo.GetPlatformsAsStringList()))
                    ;
                
                if (editGemResult.cast<int>() == 0)
                {
                    gemInfoResult = GemInfoFromPath(gemPath, pybind11::none());
                }
            });
        
        if(!result || !gemInfoResult.IsValid())
        {
            return AZ::Failure();
        }
        else
        {
            //Make sure directory link is a normalized path that can be rendered in "View Directory" dialog
            gemInfoResult.m_directoryLink = QDir::cleanPath(gemInfoResult.m_directoryLink);
            return AZ::Success(AZStd::move(gemInfoResult));
        }
    }

    AZ::Outcome<ProjectInfo> PythonBindings::GetProject(const QString& path)
    {
        ProjectInfo projectInfo = ProjectInfoFromPath(QString_To_Py_Path(path));
        if (projectInfo.IsValid())
        {
            return AZ::Success(AZStd::move(projectInfo));
        }
        else
        {
            return AZ::Failure();
        }
    }

    GemInfo PythonBindings::GemInfoFromPath(pybind11::handle path, pybind11::handle pyProjectPath)
    {
        GemInfo gemInfo;
        gemInfo.m_path = Py_To_String(path);
        gemInfo.m_directoryLink = gemInfo.m_path;

        auto data = m_manifest.attr("get_gem_json_data")(pybind11::none(), path, pyProjectPath);
        if (pybind11::isinstance<pybind11::dict>(data))
        {
            try
            {
                // required
                gemInfo.m_name = Py_To_String(data["gem_name"]);

                // optional
                gemInfo.m_displayName = Py_To_String_Optional(data, "display_name", gemInfo.m_name);
                gemInfo.m_summary = Py_To_String_Optional(data, "summary", "");
                gemInfo.m_version = Py_To_String_Optional(data, "version", gemInfo.m_version);
                gemInfo.m_lastUpdatedDate = Py_To_String_Optional(data, "last_updated", gemInfo.m_lastUpdatedDate);
                gemInfo.m_binarySizeInKB = Py_To_Int_Optional(data, "binary_size", gemInfo.m_binarySizeInKB);
                gemInfo.m_requirement = Py_To_String_Optional(data, "requirements", "");
                gemInfo.m_origin = Py_To_String_Optional(data, "origin", "");
                gemInfo.m_originURL = Py_To_String_Optional(data, "origin_url", "");
                gemInfo.m_documentationLink = Py_To_String_Optional(data, "documentation_url", "");
                gemInfo.m_iconPath = Py_To_String_Optional(data, "icon_path", "preview.png");
                gemInfo.m_licenseText = Py_To_String_Optional(data, "license", "Unspecified License");
                gemInfo.m_licenseLink = Py_To_String_Optional(data, "license_url", "");
                gemInfo.m_repoUri = Py_To_String_Optional(data, "repo_uri", "");

                if (gemInfo.m_origin.contains("Open 3D Engine"))
                {
                    gemInfo.m_gemOrigin = GemInfo::GemOrigin::Open3DEngine;
                }
                else if (data.contains("origin"))
                {
                    gemInfo.m_gemOrigin = GemInfo::GemOrigin::Remote;
                }
                // If no origin was provided this cannot be remote and would be specified if O3DE so it should be local
                else
                {
                    gemInfo.m_gemOrigin = GemInfo::GemOrigin::Local;
                }

                // As long Base Open3DEngine gems are installed before first startup non-remote gems will be downloaded
                if (gemInfo.m_gemOrigin != GemInfo::GemOrigin::Remote)
                {
                    gemInfo.m_downloadStatus = GemInfo::DownloadStatus::Downloaded;
                }

                if (data.contains("user_tags"))
                {
                    for (auto tag : data["user_tags"])
                    {
                        gemInfo.m_features.push_back(Py_To_String(tag));
                    }
                }

                if (data.contains("platforms"))
                {
                    for (auto platform : data["platforms"])
                    {
                        gemInfo.m_platforms |= GemInfo::GetPlatformFromString(Py_To_String(platform));
                    }
                }

                if (data.contains("dependencies"))
                {
                    for (auto dependency : data["dependencies"])
                    {
                        gemInfo.m_dependencies.push_back(Py_To_String(dependency));
                    }
                }

                QString gemType = Py_To_String_Optional(data, "type", "");
                if (gemType == "Asset")
                {
                    gemInfo.m_types |= GemInfo::Type::Asset;
                }
                if (gemType == "Code")
                {
                    gemInfo.m_types |= GemInfo::Type::Code;
                }
                if (gemType == "Tool")
                {
                    gemInfo.m_types |= GemInfo::Type::Tool;
                }
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                AZ_Warning("PythonBindings", false, "Failed to get GemInfo for gem %s", Py_To_String(path));
            }
        }

        return gemInfo;
    }

    ProjectInfo PythonBindings::ProjectInfoFromPath(pybind11::handle path)
    {
        ProjectInfo projectInfo;
        projectInfo.m_path = Py_To_String(path);
        projectInfo.m_needsBuild = false;

        auto projectData = m_manifest.attr("get_project_json_data")(pybind11::none(), path);
        if (pybind11::isinstance<pybind11::dict>(projectData))
        {
            try
            {
                projectInfo.m_projectName = Py_To_String(projectData["project_name"]);
                projectInfo.m_displayName = Py_To_String_Optional(projectData, "display_name", projectInfo.m_projectName);
                projectInfo.m_id = Py_To_String_Optional(projectData, "project_id", projectInfo.m_id);
                projectInfo.m_origin = Py_To_String_Optional(projectData, "origin", projectInfo.m_origin);
                projectInfo.m_summary = Py_To_String_Optional(projectData, "summary", projectInfo.m_summary);
                projectInfo.m_requirements = Py_To_String_Optional(projectData, "requirements", projectInfo.m_requirements);
                projectInfo.m_license = Py_To_String_Optional(projectData, "license", projectInfo.m_license);
                projectInfo.m_iconPath = Py_To_String_Optional(projectData, "icon", ProjectPreviewImagePath);
                projectInfo.m_engineName = Py_To_String_Optional(projectData, "engine", projectInfo.m_engineName);
                if (projectData.contains("user_tags"))
                {
                    for (auto tag : projectData["user_tags"])
                    {
                        projectInfo.m_userTags.append(Py_To_String(tag));
                    }
                }

                auto enginePathResult = m_manifest.attr("get_project_engine_path")(path);
                if (!pybind11::isinstance<pybind11::none>(enginePathResult))
                {
                    // request a posix path so it looks like what is in o3de_manifest.json
                    projectInfo.m_enginePath = Py_To_String(enginePathResult.attr("as_posix")());
                }
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                AZ_Warning("PythonBindings", false, "Failed to get ProjectInfo for project %s", Py_To_String(path));
            }
        }

        return projectInfo;
    }

    AZ::Outcome<QVector<ProjectInfo>> PythonBindings::GetProjects()
    {
        QVector<ProjectInfo> projects;

        bool result = ExecuteWithLock([&] {
            // external projects
            for (auto path : m_manifest.attr("get_manifest_projects")())
            {
                projects.push_back(ProjectInfoFromPath(path));
            }

            // projects from the engine
            for (auto path : m_manifest.attr("get_engine_projects")())
            {
                projects.push_back(ProjectInfoFromPath(path));
            }
        });

        if (!result)
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(projects));
        }
    }

    AZ::Outcome<QVector<ProjectInfo>, AZStd::string> PythonBindings::GetProjectsForRepo(const QString& repoUri)
    {
        QVector<ProjectInfo> projects;

        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto pyUri = QString_To_Py_String(repoUri);
                auto projectPaths = m_repo.attr("get_project_json_paths_from_cached_repo")(pyUri);

                if (pybind11::isinstance<pybind11::set>(projectPaths))
                {
                    for (auto path : projectPaths)
                    {
                        ProjectInfo projectInfo = ProjectInfoFromPath(path);
                        projectInfo.m_remote = true;
                        projects.push_back(projectInfo);
                    }
                }
            });

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError());
        }

        return AZ::Success(AZStd::move(projects));
    }

    AZ::Outcome<QVector<ProjectInfo>, AZStd::string> PythonBindings::GetProjectsForAllRepos()
    {
        QVector<ProjectInfo> projectInfos;
        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto projectPaths = m_repo.attr("get_project_json_paths_from_all_cached_repos")();

                if (pybind11::isinstance<pybind11::set>(projectPaths))
                {
                    for (auto path : projectPaths)
                    {
                        ProjectInfo projectInfo = ProjectInfoFromPath(path);
                        projectInfo.m_remote = true;
                        projectInfos.push_back(projectInfo);
                    }
                }
            });

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError());
        }

        return AZ::Success(AZStd::move(projectInfos));
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::AddGemToProject(const QString& gemPath, const QString& projectPath)
    {
        return ExecuteWithLockErrorHandling([&]
        {
            m_enableGemProject.attr("enable_gem_in_project")(
                pybind11::none(), // gem name not needed as path is provided
                QString_To_Py_Path(gemPath),
                pybind11::none(), // project name not needed as path is provided
                QString_To_Py_Path(projectPath)
                );
        });
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::RemoveGemFromProject(const QString& gemPath, const QString& projectPath)
    {
        return ExecuteWithLockErrorHandling([&]
        {
            m_disableGemProject.attr("disable_gem_in_project")(
                pybind11::none(), // gem name not needed as path is provided
                QString_To_Py_Path(gemPath),
                pybind11::none(), // project name not needed as path is provided
                QString_To_Py_Path(projectPath)
                );
        });
    }

    bool PythonBindings::RemoveInvalidProjects()
    {
        bool removalResult = false;
        bool result = ExecuteWithLock(
            [&]
            {
                auto pythonRemovalResult = m_register.attr("remove_invalid_o3de_projects")();

                // Returns an exit code so boolify it then invert result
                removalResult = !pythonRemovalResult.cast<bool>();
            });

        return result && removalResult;
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::UpdateProject(const ProjectInfo& projectInfo)
    {
        bool updateProjectSucceeded = false;
        auto result = ExecuteWithLockErrorHandling([&]
            {
                using namespace pybind11::literals;

                std::list<std::string> newTags;
                for (const auto& i : projectInfo.m_userTags)
                {
                    newTags.push_back(i.toStdString());
                }

                auto editResult = m_editProjectProperties.attr("edit_project_props")(
                    "proj_path"_a = QString_To_Py_Path(projectInfo.m_path),
                    "new_name"_a = QString_To_Py_String(projectInfo.m_projectName),
                    "new_id"_a = QString_To_Py_String(projectInfo.m_id),
                    "new_origin"_a = QString_To_Py_String(projectInfo.m_origin),
                    "new_display"_a = QString_To_Py_String(projectInfo.m_displayName),
                    "new_summary"_a = QString_To_Py_String(projectInfo.m_summary),
                    "new_icon"_a = QString_To_Py_String(projectInfo.m_iconPath),
                    "replace_tags"_a = pybind11::list(pybind11::cast(newTags)),
                    "new_engine_name"_a = QString_To_Py_String(projectInfo.m_engineName)
                    );
                updateProjectSucceeded = (editResult.cast<int>() == 0);
            });

        if (!result.IsSuccess())
        {
            return result;
        }
        else if (!updateProjectSucceeded)
        {
            return AZ::Failure<AZStd::string>("Failed to update project.");
        }

        return AZ::Success();
    }

    ProjectTemplateInfo PythonBindings::ProjectTemplateInfoFromPath(pybind11::handle path) const
    {
        ProjectTemplateInfo templateInfo(TemplateInfoFromPath(path));
        if (templateInfo.IsValid())
        {
            QString templateProjectPath = QDir(templateInfo.m_path).filePath("Template");
            auto enabledGemNames = GetEnabledGemNames(templateProjectPath);
            if (enabledGemNames)
            {
                for (auto gem : enabledGemNames.GetValue())
                {
                    // Exclude the template ${Name} placeholder for the list of included gems
                    // That Gem gets created with the project
                    if (!gem.contains("${Name}"))
                    {
                        templateInfo.m_includedGems.push_back(Py_To_String(gem.c_str()));
                    }
                }
            }
        }

        return templateInfo;
    }

    TemplateInfo PythonBindings::TemplateInfoFromPath(pybind11::handle path) const
    {
        TemplateInfo templateInfo;
        templateInfo.m_path = Py_To_String(path);

        using namespace pybind11::literals;

        // no project path is needed when a template path is provided
        auto data = m_manifest.attr("get_template_json_data")("template_path"_a = path);
        if (pybind11::isinstance<pybind11::dict>(data))
        {
            try
            {
                templateInfo.m_displayName = Py_To_String(data["display_name"]);
                templateInfo.m_name = Py_To_String(data["template_name"]);
                templateInfo.m_summary = Py_To_String(data["summary"]);

                if (data.contains("canonical_tags"))
                {
                    for (auto tag : data["canonical_tags"])
                    {
                        templateInfo.m_canonicalTags.push_back(Py_To_String(tag));
                    }
                }

                if (data.contains("user_tags"))
                {
                    for (auto tag : data["user_tags"])
                    {
                        templateInfo.m_userTags.push_back(Py_To_String(tag));
                    }
                }


                templateInfo.m_requirements = Py_To_String_Optional(data, "requirements", "");
                templateInfo.m_license = Py_To_String_Optional(data, "license", "");
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                AZ_Warning("PythonBindings", false, "Failed to get TemplateInfo for %s", Py_To_String(path));
            }
        }

        return templateInfo;
    }

    AZ::Outcome<QVector<ProjectTemplateInfo>> PythonBindings::GetProjectTemplates()
    {
        QVector<ProjectTemplateInfo> templates;

        bool result = ExecuteWithLock([&] {
            for (auto path : m_manifest.attr("get_templates_for_project_creation")())
            {
                templates.push_back(ProjectTemplateInfoFromPath(path));
            }
        });

        if (!result)
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(templates));
        }
    }

    AZ::Outcome<QVector<TemplateInfo>> PythonBindings::GetGemTemplates()
    {
        QVector<TemplateInfo> templates;

        bool result = ExecuteWithLock(
            [&]
            {
                for (auto path : m_manifest.attr("get_templates_for_gem_creation")())
                {
                    templates.push_back(TemplateInfoFromPath(path));
                }
            });

        if (!result)
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(templates));
        }
    }

    AZ::Outcome<QVector<ProjectTemplateInfo>> PythonBindings::GetProjectTemplatesForRepo(const QString& repoUri) const
    {
        QVector<ProjectTemplateInfo> templates;

        bool result = ExecuteWithLock(
            [&]
            {
                using namespace pybind11::literals;

                auto templatePaths = m_repo.attr("get_template_json_paths_from_cached_repo")(
                    "repo_uri"_a = QString_To_Py_String(repoUri)
                    );

                if (pybind11::isinstance<pybind11::set>(templatePaths))
                {
                    for (auto path : templatePaths)
                    {
                        ProjectTemplateInfo remoteTemplate = ProjectTemplateInfoFromPath(path);
                        remoteTemplate.m_isRemote = true;
                        templates.push_back(remoteTemplate);
                    }
                }
            });

        if (!result)
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(templates));
        }
    }

    AZ::Outcome<QVector<ProjectTemplateInfo>> PythonBindings::GetProjectTemplatesForAllRepos() const
    {
        QVector<ProjectTemplateInfo> templates;

        bool result = ExecuteWithLock(
            [&]
            {
                auto templatePaths = m_repo.attr("get_template_json_paths_from_all_cached_repos")();

                if (pybind11::isinstance<pybind11::set>(templatePaths))
                {
                    for (auto path : templatePaths)
                    {
                        ProjectTemplateInfo remoteTemplate = ProjectTemplateInfoFromPath(path);
                        remoteTemplate.m_isRemote = true;
                        templates.push_back(remoteTemplate);
                    }
                }
            });

        if (!result)
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(templates));
        }
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::RefreshGemRepo(const QString& repoUri)
    {
        bool refreshResult = false;
        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto pyUri = QString_To_Py_String(repoUri);
                auto pythonRefreshResult = m_repo.attr("refresh_repo")(pyUri);

                // Returns an exit code so boolify it then invert result
                refreshResult = !pythonRefreshResult.cast<bool>();
            });

        if (!result.IsSuccess())
        {
            return result;
        }
        else if (!refreshResult)
        {
            return AZ::Failure<AZStd::string>("Failed to refresh repo.");
        }

        return AZ::Success();
    }

    bool PythonBindings::RefreshAllGemRepos()
    {
        bool refreshResult = false;
        bool result = ExecuteWithLock(
            [&]
            {
                auto pythonRefreshResult = m_repo.attr("refresh_repos")();

                // Returns an exit code so boolify it then invert result
                refreshResult = !pythonRefreshResult.cast<bool>();
            });

        return result && refreshResult;
    }

    IPythonBindings::DetailedOutcome PythonBindings::AddGemRepo(const QString& repoUri)
    {
        bool registrationResult = false;
        bool result = ExecuteWithLock(
            [&]
            {
                auto pyUri = QString_To_Py_String(repoUri);
                auto pythonRegistrationResult = m_register.attr("register")(
                    pybind11::none(), pybind11::none(), pybind11::none(), pybind11::none(), pybind11::none(), pybind11::none(), pyUri);

                // Returns an exit code so boolify it then invert result
                registrationResult = !pythonRegistrationResult.cast<bool>();
            });

        if (!result || !registrationResult)
        {
            return AZ::Failure<IPythonBindings::ErrorPair>(GetErrorPair());
        }

        return AZ::Success();
    }

    bool PythonBindings::RemoveGemRepo(const QString& repoUri)
    {
        bool registrationResult = false;
        bool result = ExecuteWithLock(
            [&]
            {
                using namespace pybind11::literals;

                auto pythonRegistrationResult = m_register.attr("register")(
                    "engine_path"_a                  = pybind11::none(),
                    "project_path"_a                 = pybind11::none(),
                    "gem_path"_a                     = pybind11::none(),
                    "external_subdir_path"_a         = pybind11::none(),
                    "template_path"_a                = pybind11::none(),
                    "restricted_path"_a              = pybind11::none(),
                    "repo_uri"_a                     = QString_To_Py_String(repoUri),
                    "default_engines_folder"_a       = pybind11::none(),
                    "default_projects_folder"_a      = pybind11::none(),
                    "default_gems_folder"_a          = pybind11::none(),
                    "default_templates_folder"_a     = pybind11::none(),
                    "default_restricted_folder"_a    = pybind11::none(),
                    "default_third_party_folder"_a   = pybind11::none(),
                    "external_subdir_engine_path"_a  = pybind11::none(),
                    "external_subdir_project_path"_a = pybind11::none(),
                    "external_subdir_gem_path"_a     = pybind11::none(),
                    "remove"_a                       = true,
                    "force"_a                        = false
                    );

                // Returns an exit code so boolify it then invert result
                registrationResult = !pythonRegistrationResult.cast<bool>();
            });

        return result && registrationResult;
    }

    GemRepoInfo PythonBindings::GetGemRepoInfo(pybind11::handle repoUri)
    {
        GemRepoInfo gemRepoInfo;
        gemRepoInfo.m_repoUri = Py_To_String(repoUri);

        auto data = m_manifest.attr("get_repo_json_data")(repoUri);
        if (pybind11::isinstance<pybind11::dict>(data))
        {
            try
            {
                // required
                gemRepoInfo.m_name = Py_To_String(data["repo_name"]);
                gemRepoInfo.m_origin = Py_To_String(data["origin"]);

                // optional
                gemRepoInfo.m_summary = Py_To_String_Optional(data, "summary", "No summary provided.");
                gemRepoInfo.m_additionalInfo = Py_To_String_Optional(data, "additional_info", "");

                auto repoPath = m_manifest.attr("get_repo_path")(repoUri);
                gemRepoInfo.m_path = gemRepoInfo.m_directoryLink = Py_To_String(repoPath);

                QString lastUpdated = Py_To_String_Optional(data, "last_updated", "");
                gemRepoInfo.m_lastUpdated = QDateTime::fromString(lastUpdated, RepoTimeFormat);

                if (data.contains("enabled"))
                {
                    gemRepoInfo.m_isEnabled = data["enabled"].cast<bool>();
                }
                else
                {
                    gemRepoInfo.m_isEnabled = false;
                }

                if (data.contains("gems"))
                {
                    for (auto gemPath : data["gems"])
                    {
                        gemRepoInfo.m_includedGemUris.push_back(Py_To_String(gemPath));
                    }
                }
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                AZ_Warning("PythonBindings", false, "Failed to get GemRepoInfo for repo %s", Py_To_String(repoUri));
            }
        }

        return gemRepoInfo;
    }

    AZ::Outcome<QVector<GemRepoInfo>, AZStd::string> PythonBindings::GetAllGemRepoInfos()
    {
        QVector<GemRepoInfo> gemRepos;

        auto result = ExecuteWithLockErrorHandling(
            [&]
            {
                for (auto repoUri : m_manifest.attr("get_manifest_repos")())
                {
                    gemRepos.push_back(GetGemRepoInfo(repoUri));
                }
            });
        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }

        AZStd::sort(gemRepos.begin(), gemRepos.end());
        return AZ::Success(AZStd::move(gemRepos));
    }

    AZ::Outcome<QVector<GemInfo>, AZStd::string> PythonBindings::GetGemInfosForRepo(const QString& repoUri)
    {
        QVector<GemInfo> gemInfos;
        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto pyUri = QString_To_Py_String(repoUri);
                auto gemPaths = m_repo.attr("get_gem_json_paths_from_cached_repo")(pyUri);

                if (pybind11::isinstance<pybind11::set>(gemPaths))
                {
                    for (auto path : gemPaths)
                    {
                        GemInfo gemInfo = GemInfoFromPath(path, pybind11::none());
                        gemInfo.m_downloadStatus = GemInfo::DownloadStatus::NotDownloaded;
                        gemInfos.push_back(gemInfo);
                    }
                }
            });

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError());
        }

        return AZ::Success(AZStd::move(gemInfos));
    }

    AZ::Outcome<QVector<GemInfo>, AZStd::string> PythonBindings::GetGemInfosForAllRepos()
    {
        QVector<GemInfo> gemInfos;
        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto gemPaths = m_repo.attr("get_gem_json_paths_from_all_cached_repos")();

                if (pybind11::isinstance<pybind11::set>(gemPaths))
                {
                    for (auto path : gemPaths)
                    {
                        GemInfo gemInfo = GemInfoFromPath(path, pybind11::none());
                        gemInfo.m_downloadStatus = GemInfo::DownloadStatus::NotDownloaded;
                        gemInfos.push_back(gemInfo);
                    }
                }
            });

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError());
        }

        return AZ::Success(AZStd::move(gemInfos));
    }

    IPythonBindings::DetailedOutcome  PythonBindings::DownloadGem(
        const QString& gemName, const QString& path, std::function<void(int, int)> gemProgressCallback, bool force)
    {
        // This process is currently limited to download a single object at a time.
        bool downloadSucceeded = false;

        m_requestCancelDownload = false;
        auto result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto downloadResult = m_download.attr("download_gem")(
                    QString_To_Py_String(gemName), // gem name
                    path.isEmpty() ? pybind11::none() : QString_To_Py_Path(path), // destination path
                    false, // skip auto register
                    force, // force overwrite
                    pybind11::cpp_function(
                        [this, gemProgressCallback](int bytesDownloaded, int totalBytes)
                        {
                            gemProgressCallback(bytesDownloaded, totalBytes);

                            return m_requestCancelDownload;
                        }) // Callback for download progress and cancelling
                    );
                downloadSucceeded = (downloadResult.cast<int>() == 0);
            });


        if (!result.IsSuccess())
        {
            IPythonBindings::ErrorPair pythonRunError(result.GetError(), result.GetError());
            return AZ::Failure<IPythonBindings::ErrorPair>(AZStd::move(pythonRunError));
        }
        else if (!downloadSucceeded)
        {
            return AZ::Failure<IPythonBindings::ErrorPair>(GetErrorPair());
        }

        return AZ::Success();
    }

    IPythonBindings::DetailedOutcome PythonBindings::DownloadProject(
        const QString& projectName, const QString& path, std::function<void(int, int)> projectProgressCallback, bool force)
    {
        // This process is currently limited to download a single object at a time.
        bool downloadSucceeded = false;

        m_requestCancelDownload = false;
        auto result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto downloadResult = m_download.attr("download_project")(
                    QString_To_Py_String(projectName), // gem name
                    path.isEmpty() ? pybind11::none() : QString_To_Py_Path(path), // destination path
                    false, // skip auto register
                    force, // force overwrite
                    pybind11::cpp_function(
                        [this, projectProgressCallback](int bytesDownloaded, int totalBytes)
                        {
                            projectProgressCallback(bytesDownloaded, totalBytes);

                            return m_requestCancelDownload;
                        }) // Callback for download progress and cancelling
                );
                downloadSucceeded = (downloadResult.cast<int>() == 0);
            });

        if (!result.IsSuccess())
        {
            IPythonBindings::ErrorPair pythonRunError(result.GetError(), result.GetError());
            return AZ::Failure<IPythonBindings::ErrorPair>(AZStd::move(pythonRunError));
        }
        else if (!downloadSucceeded)
        {
            return AZ::Failure<IPythonBindings::ErrorPair>(GetErrorPair());
        }

        return AZ::Success();
    }

    IPythonBindings::DetailedOutcome PythonBindings::DownloadTemplate(
        const QString& templateName, const QString& path, std::function<void(int, int)> templateProgressCallback, bool force)
    {
        // This process is currently limited to download a single object at a time.
        bool downloadSucceeded = false;

        m_requestCancelDownload = false;
        auto result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto downloadResult = m_download.attr("download_template")(
                    QString_To_Py_String(templateName), // gem name
                    path.isEmpty() ? pybind11::none() : QString_To_Py_Path(path), // destination path
                    false, // skip auto register
                    force, // force overwrite
                    pybind11::cpp_function(
                        [this, templateProgressCallback](int bytesDownloaded, int totalBytes)
                        {
                            templateProgressCallback(bytesDownloaded, totalBytes);

                            return m_requestCancelDownload;
                        }) // Callback for download progress and cancelling
                );
                downloadSucceeded = (downloadResult.cast<int>() == 0);
            });

        if (!result.IsSuccess())
        {
            IPythonBindings::ErrorPair pythonRunError(result.GetError(), result.GetError());
            return AZ::Failure<IPythonBindings::ErrorPair>(AZStd::move(pythonRunError));
        }
        else if (!downloadSucceeded)
        {
            return AZ::Failure<IPythonBindings::ErrorPair>(GetErrorPair());
        }

        return AZ::Success();
    }

    void PythonBindings::CancelDownload()
    {
        m_requestCancelDownload = true;
    }

    bool PythonBindings::IsGemUpdateAvaliable(const QString& gemName, const QString& lastUpdated)
    {
        bool updateAvaliableResult = false;
        bool result = ExecuteWithLock(
            [&]
            {
                auto pyGemName = QString_To_Py_String(gemName);
                auto pyLastUpdated = QString_To_Py_String(lastUpdated);
                auto pythonUpdateAvaliableResult = m_download.attr("is_o3de_gem_update_available")(pyGemName, pyLastUpdated);

                updateAvaliableResult = pythonUpdateAvaliableResult.cast<bool>();
            });

        return result && updateAvaliableResult;
    }

    IPythonBindings::ErrorPair PythonBindings::GetErrorPair()
    {
        if (const size_t errorSize = m_pythonErrorStrings.size())
        {
            AZStd::string detailedString =
                errorSize == 1 ? "" : AZStd::accumulate(m_pythonErrorStrings.begin(), m_pythonErrorStrings.end(), AZStd::string(""));

            return IPythonBindings::ErrorPair(m_pythonErrorStrings.front(), detailedString);
        }
        // If no error was found
        else
        {
            return IPythonBindings::ErrorPair(AZStd::string("Unknown Python Bindings Error"), AZStd::string(""));
        }
    }

    void PythonBindings::AddErrorString(AZStd::string errorString)
    {
        m_pythonErrorStrings.push_back(errorString);
    }
} // namespace O3DE::ProjectManager
