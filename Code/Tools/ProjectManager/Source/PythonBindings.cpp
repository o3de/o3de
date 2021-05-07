/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <PythonBindings.h>

// Qt defines slots, which interferes with the use here.
#pragma push_macro("slots")
#undef slots
#include <Python.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#pragma pop_macro("slots")

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace Platform
{
    // Implemented in each different platform's implentation files, as it differs per platform.
    bool InsertPythonBinaryLibraryPaths(AZStd::unordered_set<AZStd::string>& paths, const char* pythonPackage, const char* engineRoot);
    AZStd::string GetPythonHomePath(const char* pythonPackage, const char* engineRoot);
} // namespace Platform

namespace O3DE::ProjectManager 
{
    PythonBindings::PythonBindings(const AZ::IO::PathView& enginePath)
        : m_enginePath(enginePath)
    {
        StartPython();
    }

    PythonBindings::~PythonBindings()
    {
        StopPython();
    }

    bool PythonBindings::StartPython()
    {
        if (Py_IsInitialized())
        {
            AZ_Warning("python", false, "Python is already active!");
            return false;
        }

        // set PYTHON_HOME
        AZStd::string pyBasePath = Platform::GetPythonHomePath(PY_PACKAGE, m_enginePath.c_str());
        if (!AZ::IO::SystemFile::Exists(pyBasePath.c_str()))
        {
            AZ_Warning("python", false, "Python home path must exist! path:%s", pyBasePath.c_str());
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

        try
        {
            // ignore system location for sites site-packages
            Py_IsolatedFlag = 1; // -I - Also sets Py_NoUserSiteDirectory.  If removed PyNoUserSiteDirectory should be set.
            Py_IgnoreEnvironmentFlag = 1; // -E

            const bool initializeSignalHandlers = true;
            pybind11::initialize_interpreter(initializeSignalHandlers);

            // Acquire GIL before calling Python code
            AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
            pybind11::gil_scoped_acquire acquire;

            // print Python version using AZ logging
            const int verRet = PyRun_SimpleStringFlags("import sys \nprint (sys.version) \n", nullptr);
            AZ_Error("python", verRet == 0, "Error trying to fetch the version number in Python!");

            // Setup sys.path
            int result = PyRun_SimpleString("import sys");
            AZ_Warning("ProjectManagerWindow", result != -1, "import sys failed");
            result = PyRun_SimpleString(AZStd::string::format("sys.path.append('%s')", m_enginePath.c_str()).c_str());
            AZ_Warning("ProjectManagerWindow", result != -1, "append to sys path failed");

            return verRet == 0 && !PyErr_Occurred();
        } catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("python", false, "Py_Initialize() failed with %s!", e.what());
            return false;
        }
    }

    bool PythonBindings::StopPython()
    {
        if (Py_IsInitialized())
        {
            pybind11::finalize_interpreter();
        }
        else
        {
            AZ_Warning("python", false, "Did not finalize since Py_IsInitialized() was false.");
        }
        return !PyErr_Occurred();
    }

    void PythonBindings::ExecuteWithLock(AZStd::function<void()> executionCallback)
    {
        AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
        pybind11::gil_scoped_release release;
        pybind11::gil_scoped_acquire acquire;
        executionCallback();
    }

    Project PythonBindings::GetCurrentProject()
    {
        Project project;

        ExecuteWithLock([&] {
            auto currentProjectTool = pybind11::module::import("cmake.Tools.current_project");
            auto getCurrentProject = currentProjectTool.attr("get_current_project");
            auto currentProject = getCurrentProject(m_enginePath.c_str());

            project.m_path = currentProject.cast<std::string>().c_str();
        });

        return project; 
    }
}
