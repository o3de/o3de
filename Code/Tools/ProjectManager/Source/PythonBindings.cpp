/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PythonBindings.h>

#include <ProjectManagerDefs.h>
#include <osdefs.h> // for DELIM

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
#include <AzToolsFramework/API/PythonLoader.h>


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
} // namespace Platform


#define Py_To_String(obj) pybind11::str(obj).cast<std::string>().c_str()
#define Py_To_String_Optional(dict, key, default_string) dict.contains(key) ? Py_To_String(dict[key]) : default_string
#define Py_To_Int(obj) obj.cast<int>()
#define Py_To_Int_Optional(dict, key, default_int) dict.contains(key) ? Py_To_Int(dict[key]) : default_int
#define QString_To_Py_String(value) pybind11::str(value.toStdString())
#define QString_To_Py_Path(value) value.isEmpty() ? pybind11::none() : m_pathlib.attr("Path")(value.toStdString())

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
        AZStd::string pyBasePath = AzToolsFramework::EmbeddedPython::PythonLoader::GetPythonHomePath(m_enginePath).StringAsPosix();
        if (!AZ::IO::SystemFile::Exists(pyBasePath.c_str()))
        {
            AZ_Error("python", false, "Python home path does not exist: %s", pyBasePath.c_str());
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
            Py_DontWriteBytecodeFlag = 1; // Do not generate precompiled bytecode

            const bool initializeSignalHandlers = true;
            pybind11::initialize_interpreter(initializeSignalHandlers);

            // display basic Python information
            AZ_Trace("python", "Py_GetVersion=%s \n", Py_GetVersion());
            AZ_Trace("python", "Py_GetPath=%ls \n", Py_GetPath());
            AZ_Trace("python", "Py_GetExecPrefix=%ls \n", Py_GetExecPrefix());
            AZ_Trace("python", "Py_GetProgramFullPath=%ls \n", Py_GetProgramFullPath());

            RedirectOutput::Intialize(PyImport_ImportModule("azlmbr_redirect"), &PythonBindings::OnStdOut, &PythonBindings::OnStdError);

            // Add custom site packages after initializing the interpreter above.  Calling Py_SetPath before initialization
            // alters the behavior of the initializer to not compute default search paths. See
            // https://docs.python.org/3/c-api/init.html#c.Py_SetPath

            AZStd::vector<AZ::IO::Path> extendedPaths;
            AzToolsFramework::EmbeddedPython::PythonLoader::ReadPythonEggLinkPaths(
                m_enginePath.c_str(), [&extendedPaths](AZ::IO::PathView path)
                {
                    extendedPaths.emplace_back(path);
                });

            if (!extendedPaths.empty())
            {
                ExtendSysPath(extendedPaths);
            }

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
            m_register = pybind11::module::import("o3de.register");
            m_manifest = pybind11::module::import("o3de.manifest");
            m_engineTemplate = pybind11::module::import("o3de.engine_template");
            m_gemProperties = pybind11::module::import("o3de.gem_properties");
            m_engineProperties = pybind11::module::import("o3de.engine_properties");
            m_enableGemProject = pybind11::module::import("o3de.enable_gem");
            m_disableGemProject = pybind11::module::import("o3de.disable_gem");
            m_editProjectProperties = pybind11::module::import("o3de.project_properties");
            m_projectManagerInterface = pybind11::module::import("o3de.project_manager_interface");
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
            return AZ::Failure("Python is not initialized");
        }

        AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
        pybind11::gil_scoped_release release;
        pybind11::gil_scoped_acquire acquire;

        m_pythonErrorStrings.clear();

        try
        {
            executionCallback();
        }
        catch (const std::exception& e)
        {
            AZ_Warning("PythonBindings", false, "Python exception %s", e.what());
            m_pythonErrorStrings.push_back(e.what());
            return AZ::Failure(e.what());
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
                if (engineData.contains("version"))
                {
                    engineInfo.m_version = Py_To_String_Optional(engineData, "version", "0.0.0");
                    engineInfo.m_displayVersion = Py_To_String_Optional(engineData, "display_version", "0.0.0");
                }
                else
                {
                    // maintain for backwards compatibility with older file formats
                    engineInfo.m_version = Py_To_String_Optional(engineData, "O3DEVersion", "0.0.0");
                    engineInfo.m_displayVersion = engineInfo.m_version;
                }

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

        return AZ::Failure(GetErrorPair());
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

    void GetGemInfoFromPyDict(GemInfo& gemInfo, pybind11::dict data)
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
        gemInfo.m_downloadSourceUri = Py_To_String_Optional(data, "origin_uri", "");
        gemInfo.m_downloadSourceUri = Py_To_String_Optional(data, "download_source_uri", gemInfo.m_downloadSourceUri);
        gemInfo.m_sourceControlUri = Py_To_String_Optional(data, "source_control_uri", "");
        gemInfo.m_sourceControlRef = Py_To_String_Optional(data, "source_control_ref", "");
        gemInfo.m_documentationLink = Py_To_String_Optional(data, "documentation_url", "");
        gemInfo.m_iconPath = Py_To_String_Optional(data, "icon_path", "preview.png");
        gemInfo.m_licenseText = Py_To_String_Optional(data, "license", "Unspecified License");
        gemInfo.m_licenseLink = Py_To_String_Optional(data, "license_url", "");
        gemInfo.m_repoUri = Py_To_String_Optional(data, "repo_uri", "");
        gemInfo.m_path = Py_To_String_Optional(data, "path", gemInfo.m_path);
        gemInfo.m_directoryLink = QDir::cleanPath(gemInfo.m_path);
        gemInfo.m_isEngineGem = Py_To_Int_Optional(data, "engine_gem", 0);
        gemInfo.m_isProjectGem = Py_To_Int_Optional(data, "project_gem", 0);

        if (gemInfo.m_isEngineGem || gemInfo.m_origin.contains("Open 3D Engine"))
        {
            gemInfo.m_gemOrigin = GemInfo::GemOrigin::Open3DEngine;
        }
        else if (QUrl(gemInfo.m_repoUri, QUrl::StrictMode).isValid())
        {
            // this gem has a valid remote repo 
            gemInfo.m_gemOrigin = GemInfo::GemOrigin::Remote;
        }
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

        if (data.contains("compatible_engines"))
        {
            for (auto compatible_engine : data["compatible_engines"])
            {
                gemInfo.m_compatibleEngines.push_back(Py_To_String(compatible_engine));
            }
        }

        if (data.contains("incompatible_engine_dependencies"))
        {
            for (auto incompatible_dependency : data["incompatible_engine_dependencies"])
            {
                gemInfo.m_incompatibleEngineDependencies.push_back(Py_To_String(incompatible_dependency));
            }
        }

        if (data.contains("incompatible_gem_dependencies"))
        {
            for (auto incompatible_dependency : data["incompatible_gem_dependencies"])
            {
                gemInfo.m_incompatibleGemDependencies.push_back(Py_To_String(incompatible_dependency));
            }
        }
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

    AZ::Outcome<QVector<GemInfo>, AZStd::string> PythonBindings::GetAllGemInfos(const QString& projectPath)
    {
        QVector<GemInfo> gems;

        auto result = ExecuteWithLockErrorHandling([&]
        {
            auto pyProjectPath = QString_To_Py_Path(projectPath);
            for (pybind11::handle pyGemJsonData : m_projectManagerInterface.attr("get_all_gem_infos")(pyProjectPath))
            {
                GemInfo gemInfo;
                GetGemInfoFromPyDict(gemInfo, pyGemJsonData.cast<pybind11::dict>());

                // Mark as downloaded because this gem was registered with an existing directory
                gemInfo.m_downloadStatus = GemInfo::DownloadStatus::Downloaded;

                gems.push_back(AZStd::move(gemInfo));
            }
        });
        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError().c_str());
        }

        // this sorts by gem name and version
        AZStd::sort(gems.begin(), gems.end());
        return AZ::Success(AZStd::move(gems));
    }

    AZ::Outcome<QHash<QString, QString>, AZStd::string> PythonBindings::GetEnabledGems(const QString& projectPath, bool includeDependencies) const
    {
        QHash<QString, QString> enabledGems;
        auto result = ExecuteWithLockErrorHandling([&]
        {
            auto enabledGemsData = m_projectManagerInterface.attr("get_enabled_gems")(QString_To_Py_Path(projectPath), includeDependencies);
            if (pybind11::isinstance<pybind11::dict>(enabledGemsData))
            {
                for (auto item : pybind11::dict(enabledGemsData))
                {
                    // check for missing gem paths here otherwise case will convert the None type to "None"
                    // which looks like an incorrect path instead of a missing path
                    enabledGems.insert(Py_To_String(item.first), pybind11::isinstance<pybind11::none>(item.second) ? "" : Py_To_String(item.second));
                }
            }
            else
            {
                throw std::runtime_error("Failed to get the active gems for project");
            }
        });

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError().c_str());
        }
        else
        {
            return AZ::Success(AZStd::move(enabledGems));
        }
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
            return AZ::Failure(result.GetError().c_str());
        }
        else if (!registrationResult)
        {
            return AZ::Failure(AZStd::string::format(
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

    IPythonBindings::DetailedOutcome PythonBindings::AddProject(const QString& path, bool force)
    {
        using namespace pybind11::literals;
        bool registrationResult = false;
        bool result = ExecuteWithLock([&] {
            auto pythonRegistrationResult = m_projectManagerInterface.attr("register_project")(
                "project_path"_a = QString_To_Py_Path(path),
                "force"_a = force
                );

            // Returns an exit code so boolify it then invert result
            registrationResult = !pythonRegistrationResult.cast<bool>();
        });

        if (!result || !registrationResult)
        {
            return AZ::Failure(GetErrorPair());
        }

        return AZ::Success();
    }

    IPythonBindings::DetailedOutcome PythonBindings::RemoveProject(const QString& path)
    {
        using namespace pybind11::literals;
        bool registrationResult = false;
        bool result = ExecuteWithLock([&] {
            auto pythonRegistrationResult = m_register.attr("register")(
                "project_path"_a = QString_To_Py_Path(path),
                "remove"_a       = true
                );

            // Returns an exit code so boolify it then invert result
            registrationResult = !pythonRegistrationResult.cast<bool>();
        });

        if (!result || !registrationResult)
        {
            return AZ::Failure(GetErrorPair());
        }

        return AZ::Success();
    }

    AZ::Outcome<ProjectInfo, IPythonBindings::ErrorPair> PythonBindings::CreateProject(const QString& projectTemplatePath, const ProjectInfo& projectInfo, bool registerProject)
    {
        using namespace pybind11::literals;

        ProjectInfo createdProjectInfo;
        bool result = ExecuteWithLock([&] {
            auto projectPath = QString_To_Py_Path(projectInfo.m_path);

            auto createProjectResult = m_engineTemplate.attr("create_project")(
                "project_path"_a = projectPath,
                "project_name"_a = QString_To_Py_String(projectInfo.m_projectName),
                "template_path"_a = QString_To_Py_Path(projectTemplatePath),
                "version"_a = QString_To_Py_String(projectInfo.m_version),
                "no_register"_a = !registerProject
            );
            if (createProjectResult.cast<int>() == 0)
            {
                createdProjectInfo = ProjectInfoFromPath(projectPath);
            }
        });

        if (!result || !createdProjectInfo.IsValid())
        {
            return AZ::Failure(GetErrorPair());
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
        gemInfo.m_directoryLink = QDir::cleanPath(gemInfo.m_path);

        auto data = m_manifest.attr("get_gem_json_data")(pybind11::none(), path, pyProjectPath);
        if (pybind11::isinstance<pybind11::dict>(data))
        {
            try
            {
                GetGemInfoFromPyDict(gemInfo, data);
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                AZ_Warning("PythonBindings", false, "Failed to get GemInfo for gem %s", Py_To_String(path));
            }
        }

        return gemInfo;
    }

    ProjectInfo PythonBindings::ProjectInfoFromDict(pybind11::handle projectData, const QString& path)
    {
        ProjectInfo projectInfo;
        projectInfo.m_needsBuild = false;

        if (!path.isEmpty())
        {
            projectInfo.m_path = path;
        }
        else
        {
            projectInfo.m_path = Py_To_String_Optional(projectData, "path", projectInfo.m_path);
        }

        projectInfo.m_projectName = Py_To_String(projectData["project_name"]);
        projectInfo.m_displayName = Py_To_String_Optional(projectData, "display_name", projectInfo.m_projectName);
        projectInfo.m_version = Py_To_String_Optional(projectData, "version", projectInfo.m_version);
        projectInfo.m_id = Py_To_String_Optional(projectData, "project_id", projectInfo.m_id);
        projectInfo.m_origin = Py_To_String_Optional(projectData, "origin", projectInfo.m_origin);
        projectInfo.m_summary = Py_To_String_Optional(projectData, "summary", projectInfo.m_summary);
        projectInfo.m_requirements = Py_To_String_Optional(projectData, "requirements", projectInfo.m_requirements);
        projectInfo.m_license = Py_To_String_Optional(projectData, "license", projectInfo.m_license);
        projectInfo.m_iconPath = Py_To_String_Optional(projectData, "icon", ProjectPreviewImagePath);
        projectInfo.m_engineName = Py_To_String_Optional(projectData, "engine", projectInfo.m_engineName);
        projectInfo.m_restricted = Py_To_String_Optional(projectData, "restricted", projectInfo.m_restricted);
        projectInfo.m_isScriptOnly = Py_To_Int_Optional(projectData, "script_only", 0);
        if (projectData.contains("user_tags"))
        {
            for (auto tag : projectData["user_tags"])
            {
                projectInfo.m_userTags.append(Py_To_String(tag));
            }
        }

        if (projectData.contains("gem_names"))
        {
            for (auto gem : projectData["gem_names"])
            {
                if (pybind11::isinstance<pybind11::dict>(gem))
                {
                    if (gem["optional"].cast<bool>())
                    {
                        projectInfo.m_optionalGemDependencies.append(Py_To_String(gem["name"]));
                    }
                    else
                    {
                        projectInfo.m_requiredGemDependencies.append(Py_To_String(gem["name"]));
                    }
                }
                else
                {
                    projectInfo.m_requiredGemDependencies.append(Py_To_String(gem));
                }
            }
        }

        if (projectData.contains("engine_path"))
        {
            // Python looked for an engine path so we don't need to, but be careful
            // not to add 'None' in case no path was found
            if (!pybind11::isinstance<pybind11::none>(projectData["engine_path"]))
            {
                projectInfo.m_enginePath = Py_To_String(projectData["engine_path"]);
            }
        }
        else if (!projectInfo.m_path.isEmpty())
        {
            auto enginePathResult = m_manifest.attr("get_project_engine_path")(QString_To_Py_Path(projectInfo.m_path));
            if (!pybind11::isinstance<pybind11::none>(enginePathResult))
            {
                // request a posix path so it looks like what is in o3de_manifest.json
                projectInfo.m_enginePath = Py_To_String(enginePathResult.attr("as_posix")());
            }
        }

        return projectInfo;
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
                projectInfo = ProjectInfoFromDict(projectData, projectInfo.m_path);
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
            for (auto projectData : m_projectManagerInterface.attr("get_all_project_infos")())
            {
                if (pybind11::isinstance<pybind11::dict>(projectData))
                {
                    projects.push_back(ProjectInfoFromDict(projectData));
                }
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

    AZ::Outcome<QVector<ProjectInfo>, AZStd::string> PythonBindings::GetProjectsForRepo(const QString& repoUri, bool enabledOnly)
    {
        QVector<ProjectInfo> projects;

        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto pyUri = QString_To_Py_String(repoUri);
                auto pyProjects = m_repo.attr("get_project_json_data_from_cached_repo")(pyUri, enabledOnly);
                if (pybind11::isinstance<pybind11::list>(pyProjects))
                {
                    for (auto pyProjectJsonData : pyProjects)
                    {
                        ProjectInfo projectInfo = ProjectInfoFromDict(pyProjectJsonData);
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

    AZ::Outcome<QVector<ProjectInfo>, AZStd::string> PythonBindings::GetProjectsForAllRepos(bool enabledOnly)
    {
        QVector<ProjectInfo> projects;
        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto pyProjects = m_repo.attr("get_project_json_data_from_all_cached_repos")(enabledOnly);
                if (pybind11::isinstance<pybind11::list>(pyProjects))
                {
                    for (auto pyProjectJsonData : pyProjects)
                    {
                        ProjectInfo projectInfo = ProjectInfoFromDict(pyProjectJsonData);
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

    IPythonBindings::DetailedOutcome PythonBindings::AddGemsToProject(const QStringList& gemPaths, const QStringList& gemNames, const QString& projectPath, bool force)
    {
        bool activateResult = false;
        bool result = ExecuteWithLock(
            [&]
            {
                using namespace pybind11::literals;
                auto pythonActivateResult = m_projectManagerInterface.attr("add_gems_to_project")(
                    "gem_paths"_a = QStringList_To_Py_List(gemPaths),
                    "gem_names"_a = QStringList_To_Py_List(gemNames),
                    "project_path"_a = QString_To_Py_Path(projectPath),
                    "force"_a = force
                );

                // Returns an exit code so boolify it then invert result
                activateResult = !pythonActivateResult.cast<bool>();
            });

        if (!result || !activateResult)
        {
            return AZ::Failure(GetErrorPair());
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::RemoveGemFromProject(const QString& gemName, const QString& projectPath)
    {
        return ExecuteWithLockErrorHandling(
            [&]
            {
                using namespace pybind11::literals;
                auto result = m_disableGemProject.attr("disable_gem_in_project")(
                    "gem_name"_a = QString_To_Py_String(gemName),
                    "project_path"_a = QString_To_Py_Path(projectPath)
                    );

                // an error code of 1 indicates an error, error code 2 means the gem was not active to begin with 
                if (result.cast<int>() == 1)
                {
                    throw std::runtime_error("Failed to remove gem");
                }
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
                    "new_engine_name"_a = QString_To_Py_String(projectInfo.m_engineName),
                    "new_version"_a = QString_To_Py_String(projectInfo.m_version)
                    );
                updateProjectSucceeded = (editResult.cast<int>() == 0);

                // use the specific path locally until we have a UX for specifying the engine version 
                auto userEditResult = m_editProjectProperties.attr("edit_project_props")(
                    "proj_path"_a = QString_To_Py_Path(projectInfo.m_path),
                    "new_engine_path"_a = QString_To_Py_Path(projectInfo.m_enginePath),
                    "user"_a = true
                    );
                updateProjectSucceeded &= (userEditResult.cast<int>() == 0);
            });

        if (!result.IsSuccess())
        {
            return result;
        }
        else if (!updateProjectSucceeded)
        {
            return AZ::Failure("Failed to update project.");
        }

        return AZ::Success();
    }

    ProjectTemplateInfo PythonBindings::ProjectTemplateInfoFromDict(pybind11::handle templateData, const QString& path) const
    {
        ProjectTemplateInfo templateInfo(TemplateInfoFromDict(templateData, path));
        if (templateInfo.IsValid())
        {
            QString templateProjectPath = QDir(templateInfo.m_path).filePath("Template");
            constexpr bool includeDependencies = false;
            auto enabledGems = GetEnabledGems(templateProjectPath, includeDependencies);
            if (enabledGems)
            {
                for (auto gemName : enabledGems.GetValue().keys())
                {
                    // Exclude the template ${Name} placeholder for the list of included gems
                    // That Gem gets created with the project
                    if (!gemName.contains("${Name}"))
                    {
                        templateInfo.m_includedGems.push_back(gemName);
                    }
                }
            }
        }

        return templateInfo;
    }

    ProjectTemplateInfo PythonBindings::ProjectTemplateInfoFromPath(pybind11::handle path) const
    {
        ProjectTemplateInfo templateInfo(TemplateInfoFromPath(path));
        if (templateInfo.IsValid())
        {
            if (QFileInfo(templateInfo.m_path).isFile())
            {
                // remote templates in the cache that have not been downloaded
                // use the cached template.json file as the path so we cannot
                // get the enabled gems until the repo.json is updated to include that data
                return templateInfo;
            }

            QString templateProjectPath = QDir(templateInfo.m_path).filePath("Template");
            constexpr bool includeDependencies = false;
            auto enabledGems = GetEnabledGems(templateProjectPath, includeDependencies);
            if (enabledGems)
            {
                for (auto gemName : enabledGems.GetValue().keys())
                {
                    // Exclude the template ${Name} placeholder for the list of included gems
                    // That Gem gets created with the project
                    if (!gemName.contains("${Name}"))
                    {
                        templateInfo.m_includedGems.push_back(gemName);
                    }
                }
            }
        }

        return templateInfo;
    }

    TemplateInfo PythonBindings::TemplateInfoFromDict(pybind11::handle data, const QString& path) const
    {
        TemplateInfo templateInfo;
        if (!path.isEmpty())
        {
            templateInfo.m_path = path;
        }
        else
        {
            templateInfo.m_path = Py_To_String_Optional(data, "path", templateInfo.m_path);
        }

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
                templateInfo = TemplateInfoFromDict(data, Py_To_String(path));
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

    AZ::Outcome<QVector<ProjectTemplateInfo>> PythonBindings::GetProjectTemplatesForRepo(const QString& repoUri, bool enabledOnly) const
    {
        QVector<ProjectTemplateInfo> templates;

        bool result = ExecuteWithLock(
            [&]
            {
                using namespace pybind11::literals;
                auto pyTemplates = m_repo.attr("get_template_json_data_from_cached_repo")(
                    "repo_uri"_a = QString_To_Py_String(repoUri), "enabled_only"_a = enabledOnly
                    );

                if (pybind11::isinstance<pybind11::list>(pyTemplates))
                {
                    for (auto pyTemplateJsonData : pyTemplates)
                    {
                        ProjectTemplateInfo remoteTemplate = TemplateInfoFromDict(pyTemplateJsonData);
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

    AZ::Outcome<QVector<ProjectTemplateInfo>> PythonBindings::GetProjectTemplatesForAllRepos(bool enabledOnly) const
    {
        QVector<ProjectTemplateInfo> templates;

        bool result = ExecuteWithLock(
            [&]
            {
                auto pyTemplates = m_repo.attr("get_template_json_data_from_all_cached_repos")(enabledOnly);
                if (pybind11::isinstance<pybind11::list>(pyTemplates))
                {
                    for (auto pyTemplateJsonData : pyTemplates)
                    {
                        ProjectTemplateInfo remoteTemplate = ProjectTemplateInfoFromDict(pyTemplateJsonData);
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

    AZ::Outcome<void, AZStd::string> PythonBindings::RefreshGemRepo(const QString& repoUri, bool downloadMissingOnly)
    {
        bool refreshResult = false;
        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto pyUri = QString_To_Py_String(repoUri);
                auto pythonRefreshResult = m_repo.attr("refresh_repo")(pyUri, downloadMissingOnly);

                // Returns an exit code so boolify it then invert result
                refreshResult = !pythonRefreshResult.cast<bool>();
            });

        if (!result.IsSuccess())
        {
            return result;
        }
        else if (!refreshResult)
        {
            return AZ::Failure("Failed to refresh repo.");
        }

        return AZ::Success();
    }

    bool PythonBindings::RefreshAllGemRepos(bool downloadMissingOnly)
    {
        bool refreshResult = false;
        bool result = ExecuteWithLock(
            [&]
            {
                auto pythonRefreshResult = m_repo.attr("refresh_repos")(downloadMissingOnly);

                // Returns an exit code so boolify it then invert result
                refreshResult = !pythonRefreshResult.cast<bool>();
            });

        return result && refreshResult;
    }

    AZ::Outcome<QStringList, IPythonBindings::ErrorPair> PythonBindings::GetProjectEngineIncompatibleObjects(const QString& projectPath, const QString& enginePath)
    {
        QStringList incompatibleObjects;
        bool executeResult = ExecuteWithLock(
            [&]
            {
                using namespace pybind11::literals;
                auto result = m_projectManagerInterface.attr("get_project_engine_incompatible_objects")(
                        "project_path"_a = QString_To_Py_Path(projectPath),
                        "engine_path"_a = enginePath.isEmpty() ? pybind11::none() : QString_To_Py_Path(enginePath)
                    );
                if (pybind11::isinstance<pybind11::set>(result))
                {
                    // We don't use a const ref here because pybind11 iterator
                    // returns a temp pybind11::handle so using a reference will cause
                    // a warning/error, and copying the handle is what we want to do
                    for (auto incompatibleObject : result)
                    {
                        incompatibleObjects.push_back(Py_To_String(incompatibleObject));
                    }
                }
                else
                {
                    throw std::runtime_error("Failed to check if this project and engine have any incompatible objects.");
                }
            });

        if (!executeResult)
        {
            return AZ::Failure(GetErrorPair());
        }

        return AZ::Success(incompatibleObjects);
    }

    AZ::Outcome<QStringList, AZStd::string> PythonBindings::GetIncompatibleProjectGems(
        const QStringList& gemPaths, const QStringList& gemNames, const QString& projectPath)
    {
        QStringList incompatibleGems;
        bool executeResult = ExecuteWithLock(
            [&]
            {
                using namespace pybind11::literals;
                auto result = m_projectManagerInterface.attr("get_incompatible_project_gems")(
                        "gem_paths"_a = QStringList_To_Py_List(gemPaths),
                        "gem_names"_a = QStringList_To_Py_List(gemNames),
                        "project_path"_a = QString_To_Py_String(projectPath)
                    );
                if (pybind11::isinstance<pybind11::set>(result))
                {
                    // We don't use a const ref here because pybind11 iterator
                    // returns a temp pybind11::handle so using a reference will cause
                    // a warning/error, and copying the handle is what we want to do
                    for (auto incompatibleGem : result)
                    {
                        incompatibleGems.push_back(Py_To_String(incompatibleGem));
                    }
                }
                else
                {
                    throw std::runtime_error("Failed to get incompatible gems for project");
                }
            });

        if (!executeResult)
        {
            return AZ::Failure("Failed to get incompatible gems for project");
        }

        return AZ::Success(incompatibleGems);
    }

    IPythonBindings::DetailedOutcome PythonBindings::AddGemRepo(const QString& repoUri)
    {
        bool registrationResult = false;
        bool result = ExecuteWithLock(
            [&]
            {
                using namespace pybind11::literals;
                auto pythonRegistrationResult = m_register.attr("register")("repo_uri"_a = QString_To_Py_String(repoUri));

                // Returns an exit code so boolify it then invert result
                registrationResult = !pythonRegistrationResult.cast<bool>();
            });

        if (!result || !registrationResult)
        {
            return AZ::Failure(GetErrorPair());
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
                    "repo_uri"_a = QString_To_Py_String(repoUri),
                    "remove"_a   = true,
                    "force"_a    = false
                    );

                // Returns an exit code so boolify it then invert result
                registrationResult = !pythonRegistrationResult.cast<bool>();
            });

        return result && registrationResult;
    }


    bool PythonBindings::SetRepoEnabled(const QString& repoUri, bool enabled)
    {
        bool enableResult = false;
        bool result = ExecuteWithLock(
            [&]
            {
                auto pyResult = m_projectManagerInterface.attr("set_repo_enabled")(
                    QString_To_Py_String(repoUri), enabled);

                // Returns an exit code so boolify it then invert result
                enableResult = !pyResult.cast<bool>();
            });

        return result && enableResult;
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

                const QString lastUpdated = Py_To_String_Optional(data, "last_updated", "");

                // first attempt to read in the ISO8601 UTC python format with milliseconds
                gemRepoInfo.m_lastUpdated = QDateTime::fromString(lastUpdated, Qt::ISODateWithMs).toLocalTime();
                if (!gemRepoInfo.m_lastUpdated.isValid())
                {
                    // try without milliseconds
                    gemRepoInfo.m_lastUpdated = QDateTime::fromString(lastUpdated, Qt::ISODate).toLocalTime();
                }

                if (!gemRepoInfo.m_lastUpdated.isValid())
                {
                    const QStringList legacyFormats{ "dd/MM/yyyy HH:mm", RepoTimeFormat };
                    for (auto format : legacyFormats)
                    {
                        gemRepoInfo.m_lastUpdated = QDateTime::fromString(lastUpdated, format);
                        if (gemRepoInfo.m_lastUpdated.isValid())
                        {
                            break;
                        }
                    }
                }

                if (data.contains("enabled"))
                {
                    gemRepoInfo.m_isEnabled = data["enabled"].cast<bool>();
                }
                else
                {
                    gemRepoInfo.m_isEnabled = true;
                }

                if (gemRepoInfo.m_repoUri.compare(CanonicalRepoUri, Qt::CaseInsensitive) == 0)
                {
                    gemRepoInfo.m_badgeType = GemRepoInfo::BadgeType::BlueBadge;
                }
                else
                {
                    gemRepoInfo.m_badgeType = GemRepoInfo::BadgeType::NoBadge;
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
            return AZ::Failure(result.GetError().c_str());
        }

        AZStd::sort(gemRepos.begin(), gemRepos.end());
        return AZ::Success(AZStd::move(gemRepos));
    }

    AZ::Outcome<QVector<GemInfo>, AZStd::string> PythonBindings::GetGemInfosForRepo(const QString& repoUri, bool enabledOnly)
    {
        QVector<GemInfo> gemInfos;
        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto pyUri = QString_To_Py_String(repoUri);
                auto pyGems = m_repo.attr("get_gem_json_data_from_cached_repo")(pyUri, enabledOnly);
                if (pybind11::isinstance<pybind11::list>(pyGems))
                {
                    for (auto pyGemJsonData : pyGems)
                    {
                        GemInfo gemInfo;
                        GetGemInfoFromPyDict(gemInfo, pyGemJsonData.cast<pybind11::dict>());
                        gemInfo.m_downloadStatus = GemInfo::DownloadStatus::NotDownloaded;
                        gemInfo.m_gemOrigin = GemInfo::Remote;
                        gemInfos.push_back(AZStd::move(gemInfo));
                    }
                }
            });

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError());
        }

        return AZ::Success(AZStd::move(gemInfos));
    }

    AZ::Outcome<QVector<GemInfo>, AZStd::string> PythonBindings::GetGemInfosForAllRepos(const QString& projectPath, bool enabledOnly)
    {
        QVector<GemInfo> gems;
        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                const auto pyProjectPath = QString_To_Py_Path(projectPath);
                const auto gemInfos = m_projectManagerInterface.attr("get_gem_infos_from_all_repos")(pyProjectPath, enabledOnly);
                for (pybind11::handle pyGemJsonData : gemInfos)
                {
                    GemInfo gemInfo;
                    GetGemInfoFromPyDict(gemInfo, pyGemJsonData.cast<pybind11::dict>());
                    gemInfo.m_downloadStatus = GemInfo::DownloadStatus::NotDownloaded;
                    gemInfo.m_gemOrigin = GemInfo::Remote;
                    gems.push_back(AZStd::move(gemInfo));
                }
            });

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError());
        }

        return AZ::Success(AZStd::move(gems));
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
            return AZ::Failure(AZStd::move(pythonRunError));
        }
        else if (!downloadSucceeded)
        {
            return AZ::Failure(GetErrorPair());
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
            return AZ::Failure(AZStd::move(pythonRunError));
        }
        else if (!downloadSucceeded)
        {
            return AZ::Failure(GetErrorPair());
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
            return AZ::Failure(AZStd::move(pythonRunError));
        }
        else if (!downloadSucceeded)
        {
            return AZ::Failure(GetErrorPair());
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

    bool PythonBindings::ExtendSysPath(const AZStd::vector<AZ::IO::Path>& extendPaths)
    {
        AZStd::unordered_set<AZ::IO::Path> oldPathSet;
        auto SplitPath = [&oldPathSet](AZStd::string_view pathPart)
        {
            oldPathSet.emplace(AZ::IO::FixedMaxPath(pathPart));
        };
        AZ::StringFunc::TokenizeVisitor(Py_EncodeLocale(Py_GetPath(), nullptr), SplitPath, DELIM);
        bool appended{ false };
        AZStd::string pathAppend{ "import sys\n" };
        for (const auto& thisStr : extendPaths)
        {
            if (!oldPathSet.contains(thisStr.c_str()))
            {
                pathAppend.append(AZStd::string::format("sys.path.append(r'%s')\n", thisStr.c_str()));
                appended = true;
            }
        }
        if (appended)
        {
            PyRun_SimpleString(pathAppend.c_str());
            return true;
        }
        return false;
    }

} // namespace O3DE::ProjectManager
