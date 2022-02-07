/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Cli/PythonCliBindings.h>

#include <Cli/PythonCliMethodNames.h>
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

    void Intialize(PyObject* module)
    {
        s_RedirectModule = module;

        SetRedirection("stdout", g_redirect_stdout_saved, g_redirect_stdout, []([[maybe_unused]] const char* msg) {
            AZ_TracePrintf("Python", msg);
        });

        SetRedirection("stderr", g_redirect_stderr_saved, g_redirect_stderr, []([[maybe_unused]] const char* msg) {
            AZStd::string lastPythonError = msg;
            constexpr const char* pythonErrorPrefix = "ERROR:root:";
            constexpr size_t lengthOfErrorPrefix = AZStd::char_traits<char>::length(pythonErrorPrefix);
            auto errorPrefix = lastPythonError.find(pythonErrorPrefix);
            if (errorPrefix != AZStd::string::npos)
            {
                lastPythonError.erase(errorPrefix, lengthOfErrorPrefix);
            }
            O3DE::ProjectManager::PythonBindingsInterface::Get()->AddErrorString(lastPythonError);

            AZ_TracePrintf("Python", msg);
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


namespace O3DE::ProjectManager
{
    PythonCliBindings::PythonCliBindings(const AZ::IO::PathView& enginePath)
        : m_enginePath(enginePath)
    {
        m_pythonStarted = StartPython();
    }

    PythonCliBindings::~PythonCliBindings()
    {
        StopPython();
    }

    bool PythonCliBindings::PythonStarted()
    {
        return m_pythonStarted && Py_IsInitialized();
    }

    bool PythonCliBindings::StartPython()
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

            RedirectOutput::Intialize(PyImport_ImportModule("azlmbr_redirect"));

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

    bool PythonCliBindings::StopPython()
    {
        if (Py_IsInitialized())
        {
            RedirectOutput::Shutdown();

            // Release all modules before shutting down interpreter
            m_engineTemplate.release();
            m_engineProperties.release();
            m_cmake.release();
            m_register.release();
            m_manifest.release();
            m_enableGemProject.release();
            m_disableGemProject.release();
            m_editProjectProperties.release();
            m_download.release();
            m_repo.release();
            m_pathlib.release();

            pybind11::finalize_interpreter();
        }
        else
        {
            AZ_Warning("ProjectManagerWindow", false, "Did not finalize since Py_IsInitialized() was false");
        }
        return !PyErr_Occurred();
    }


    pybind11::handle PythonCliBindings::PathLib()
    {
        return m_pathlib;
    }

    pybind11::object PythonCliBindings::GetEngineJson(pybind11::handle enginePath)
    {
        return m_manifest.attr(PyCli::GetEngineJsonMethod)(pybind11::none(), enginePath);
    }

    pybind11::object PythonCliBindings::LoadO3deManifest()
    {
        return m_manifest.attr(PyCli::LoadManifestMethod)();
    }

    pybind11::object PythonCliBindings::GetGemsFolder()
    {
        return m_manifest.attr(PyCli::GetGemsFolderMethod)();
    }
    pybind11::object PythonCliBindings::GetProjectsFolder()
    {
        return m_manifest.attr(PyCli::GetProjectsFolderMethod)();
    }
    pybind11::object PythonCliBindings::GetRestrictedFolder()
    {
        return m_manifest.attr(PyCli::GetRestrictedFolderMethod)();
    }
    pybind11::object PythonCliBindings::GetTemplatesFolder()
    {
        return m_manifest.attr(PyCli::GetTemplatesFolderMethod)();
    }
    pybind11::object PythonCliBindings::GetThirdPartyFolder()
    {
        return m_manifest.attr(PyCli::GetThirdPartyFolderMethod)();
    }

    pybind11::object PythonCliBindings::GetManifestEngines()
    {
        return m_manifest.attr(PyCli::GetManifestEnginesMethod)();
    }

    pybind11::object PythonCliBindings::GetThisEnginePath()
    {
        return m_manifest.attr(PyCli::GetThisEnginePathMethod)();
    }

    pybind11::object PythonCliBindings::GetRegisterEnginePath(pybind11::str engineName)
    {
        return m_manifest.attr(PyCli::GetRegisterEnginePathMethod)(engineName);
    }

    int PythonCliBindings::EditEngine(pybind11::handle enginePath, pybind11::str engineName, pybind11::str engineVersion)
    {
        return m_engineProperties.attr(PyCli::EditEngineMethod)
            (enginePath, pybind11::none(), engineName, engineVersion).cast<int>();
    }

    int PythonCliBindings::RegisterEngine(
        pybind11::handle enginePath,
        pybind11::handle projectsFolderPath,
        pybind11::handle gemsFolderPath,
        pybind11::handle templatesFolderPath,
        pybind11::handle thirdPartyPath,
        bool force)
    {
        return m_register
            .attr(PyCli::RegisterMethod)(
                enginePath,
                pybind11::none(), // project_path
                pybind11::none(), // gem_path
                pybind11::none(), // external_subdir_path
                pybind11::none(), // template_path
                pybind11::none(), // restricted_path
                pybind11::none(), // repo_uri
                pybind11::none(), // default_engines_folder
                projectsFolderPath,
                gemsFolderPath,
                templatesFolderPath,
                pybind11::none(), // default_restricted_folder
                thirdPartyPath,
                pybind11::none(), // external_subdir_engine_path
                pybind11::none(), // external_subdir_project_path
                false,            // remove
                force)
            .cast<int>();
    }

    pybind11::object PythonCliBindings::GetEngineGems()
    {
        return m_manifest.attr(PyCli::GetEngineGemsMethod)();
    }

    pybind11::object PythonCliBindings::GetAllGems(pybind11::handle projectPath)
    {
        return m_manifest.attr(PyCli::GetAllGemsMethod)(projectPath);
    }

    pybind11::object PythonCliBindings::GetGemsCmakeFilePath(pybind11::handle projectPath)
    {
        return m_cmake.attr(PyCli::GetGemsCmakeFilePathMethod)(pybind11::none(), projectPath);
    }

    pybind11::object PythonCliBindings::GetEnabledGemNames(pybind11::handle cmakeFilePath)
    {
        return m_cmake.attr(PyCli::GetEnabledGemNamesMethod)(cmakeFilePath);
    }

    int PythonCliBindings::RegisterGem(pybind11::handle gemPath, pybind11::handle externalProjectPath, bool remove)
    {
        return m_register
            .attr(PyCli::RegisterMethod)(
                pybind11::none(), // engine_path
                pybind11::none(), // project_path
                gemPath,          // gem folder
                pybind11::none(), // external subdirectory
                pybind11::none(), // template_path
                pybind11::none(), // restricted folder
                pybind11::none(), // repo uri
                pybind11::none(), // default_engines_folder
                pybind11::none(), // default_projects_folder
                pybind11::none(), // default_gems_folder
                pybind11::none(), // default_templates_folder
                pybind11::none(), // default_restricted_folder
                pybind11::none(), // default_third_party_folder
                pybind11::none(), // external_subdir_engine_path
                externalProjectPath, // external_subdir_project_path
                remove)           // remove
            .cast<int>();
    }

    int PythonCliBindings::RegisterProject(pybind11::handle projectPath, bool remove)
    {
        return m_register
            .attr(PyCli::RegisterMethod)(
                pybind11::none(), // engine_path
                projectPath,      // project_path
                pybind11::none(), // gem_path
                pybind11::none(), // external_subdir_path
                pybind11::none(), // template_path
                pybind11::none(), // restricted_path
                pybind11::none(), // repo_uri
                pybind11::none(), // default_engines_folder
                pybind11::none(), // default_projects_folder
                pybind11::none(), // default_gems_folder
                pybind11::none(), // default_templates_folder
                pybind11::none(), // default_restricted_folder
                pybind11::none(), // default_third_party_folder
                pybind11::none(), // external_subdir_engine_path
                pybind11::none(), // external_subdir_project_path
                remove,           // remove
                false)            // force
            .cast<int>();
    }

    int PythonCliBindings::CreateProject(pybind11::handle projectPath, pybind11::str projectName, pybind11::handle templatePath)
    {
        return m_engineTemplate
            .attr(PyCli::CreateProjectMethod)(
                projectPath,
                projectName,
                templatePath)
            .cast<int>();
    }

    pybind11::object PythonCliBindings::GetGemJson(pybind11::handle gemPath, pybind11::handle projectPath)
    {
        return m_manifest.attr(PyCli::GetGemJsonMethod)(pybind11::none(), gemPath, projectPath);
    }

    pybind11::object PythonCliBindings::GetProjectJson(pybind11::handle projectPath)
    {
        return m_manifest.attr(PyCli::GetProjectJsonMethod)(pybind11::none(), projectPath);
    }

    pybind11::object PythonCliBindings::GetManifestProjects()
    {
        return m_manifest.attr(PyCli::GetManifestProjectsMethod)();
    }

    pybind11::object PythonCliBindings::GetEngineProjects()
    {
        return m_manifest.attr(PyCli::GetEngineProjectsMethod)();
    }

    int PythonCliBindings::EnableProjectGem(pybind11::handle gemPath, pybind11::handle projectPath)
    {
        return m_enableGemProject
            .attr(PyCli::EnableProjectGemMethod)(
                pybind11::none(), // gem name not needed as path is provided
                gemPath,
                pybind11::none(), // project name not needed as path is provided
                projectPath)
            .cast<int>();
    }

    int PythonCliBindings::DisableProjectGem(pybind11::handle gemPath, pybind11::handle projectPath)
    {
        return m_enableGemProject
            .attr(PyCli::DisableProjectGemMethod)(
                pybind11::none(), // gem name not needed as path is provided
                gemPath,
                pybind11::none(), // project name not needed as path is provided
                projectPath)
            .cast<int>();
    }

    int PythonCliBindings::RemoveInvalidProjects()
    {
        return m_register.attr(PyCli::RemoveInvalidProjectsMethod)().cast<int>();
    }

    int PythonCliBindings::EditProject(
        pybind11::handle projectPath,
        pybind11::str projectName,
        pybind11::str id,
        pybind11::str origin,
        pybind11::str displayName,
        pybind11::str summary,
        pybind11::str iconPath,
        pybind11::list tags)
    {
        return m_editProjectProperties
            .attr(PyCli::EditProjectMethod)(
                projectPath,
                pybind11::none(), // proj_name not used
                projectName,
                id,
                origin,
                displayName,
                summary,
                iconPath,         // new_icon
                pybind11::none(), // add_tags not used
                pybind11::none(), // remove_tags not used
                tags)
            .cast<int>();
    }

    pybind11::object PythonCliBindings::GetTemplateJson(pybind11::handle templatePath, pybind11::handle projectPath)
    {
        return m_manifest.attr(PyCli::GetTemplateJsonMethod)(pybind11::none(), templatePath, projectPath);
    }

    pybind11::object PythonCliBindings::GetTemplates()
    {
        return m_manifest.attr(PyCli::GetTemplatesMethod)();
    }

    int PythonCliBindings::RefreshRepo(pybind11::str repoUri)
    {
        return m_repo.attr(PyCli::RefreshRepoMethod)(repoUri).cast<int>();
    }

    int PythonCliBindings::RefreshRepos()  
    {
        return m_repo.attr(PyCli::RefreshReposMethod)().cast<int>();
    }

    int PythonCliBindings::RegisterRepo(pybind11::str repoUri, bool remove)
    {
        return m_register
            .attr(PyCli::RegisterMethod)(
                pybind11::none(), // engine_path
                pybind11::none(), // project_path
                pybind11::none(), // gem_path
                pybind11::none(), // external_subdir_path
                pybind11::none(), // template_path
                pybind11::none(), // restricted_path
                repoUri,          // repo_uri
                pybind11::none(), // default_engines_folder
                pybind11::none(), // default_projects_folder
                pybind11::none(), // default_gems_folder
                pybind11::none(), // default_templates_folder
                pybind11::none(), // default_restricted_folder
                pybind11::none(), // default_third_party_folder
                pybind11::none(), // external_subdir_engine_path
                pybind11::none(), // external_subdir_project_path
                remove,           // remove
                false)            // force
            .cast<int>();
    }

    pybind11::object PythonCliBindings::GetRepoJson(pybind11::handle repoUri)
    {
        return m_manifest.attr(PyCli::GetRepoJsonMethod)(repoUri);
    }

    pybind11::object PythonCliBindings::GetRepoPath(pybind11::handle repoUri)
    {
        return m_manifest.attr(PyCli::GetRepoPathMethod)(repoUri);
    }

    pybind11::object PythonCliBindings::GetReposUris()
    {
        return m_manifest.attr(PyCli::GetReposUrisMethod)();
    }

    pybind11::object PythonCliBindings::GetCachedGemJsonPaths(pybind11::str repoUri)
    {
        return m_repo.attr(PyCli::GetCachedGemJsonPathsMethod)(repoUri);
    }

    pybind11::object PythonCliBindings::GetAllCachedGemJsonPaths()
    {
        return m_repo.attr(PyCli::GetAllCachedGemJsonPathsMethod)();
    }

    int PythonCliBindings::DownloadGem(pybind11::str gemName, pybind11::cpp_function callback, bool force)
    {
        return m_download
            .attr(PyCli::DownloadGemMethod)(
                gemName,          // gem name
                pybind11::none(), // destination path
                false,            // skip auto register
                force,            // force overwrite
                callback)         // Callback for download progress and cancelling
            .cast<int>();
    }

    bool PythonCliBindings::IsGemUpdateAvaliable(pybind11::str gemName, pybind11::str lastUpdated)
    {
        return m_download.attr(PyCli::IsGemUpdateAvaliableMethod)(gemName, lastUpdated).cast<bool>();
    }

}
