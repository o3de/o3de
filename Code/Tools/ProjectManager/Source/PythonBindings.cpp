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
#include <pybind11/functional.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#pragma pop_macro("slots")

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>

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

    // Implemented in each different platform's PAL implentation files, as it differs per platform.
    AZStd::string GetPythonHomePath(const char* pythonPackage, const char* engineRoot);

} // namespace Platform

#define Py_To_String(obj) obj.cast<std::string>().c_str()
#define Py_To_String_Optional(dict, key, default_string) dict.contains(key) ? Py_To_String(dict[key]) : default_string

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
            AZ_Warning("python", false, "Python is already active");
            return false;
        }

        // set PYTHON_HOME
        AZStd::string pyBasePath = Platform::GetPythonHomePath(PY_PACKAGE, m_enginePath.c_str());
        if (!AZ::IO::SystemFile::Exists(pyBasePath.c_str()))
        {
            AZ_Warning("python", false, "Python home path must exist. path:%s", pyBasePath.c_str());
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

            // Setup sys.path
            int result = PyRun_SimpleString("import sys");
            AZ_Warning("ProjectManagerWindow", result != -1, "Import sys failed");
            result = PyRun_SimpleString(AZStd::string::format("sys.path.append('%s')", m_enginePath.c_str()).c_str());
            AZ_Warning("ProjectManagerWindow", result != -1, "Append to sys path failed");

            // import required modules
            m_registration = pybind11::module::import("cmake.Tools.registration");

            return result == 0 && !PyErr_Occurred();
        } catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("ProjectManagerWindow", false, "Py_Initialize() failed with %s", e.what());
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
            AZ_Warning("ProjectManagerWindow", false, "Did not finalize since Py_IsInitialized() was false");
        }
        return !PyErr_Occurred();
    }

    bool PythonBindings::ExecuteWithLock(AZStd::function<void()> executionCallback)
    {
        AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
        pybind11::gil_scoped_release release;
        pybind11::gil_scoped_acquire acquire;

        try
        {
            executionCallback();
            return true;
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("PythonBindings", false, "Python exception %s", e.what());
            return false;
        }
    }

    AZ::Outcome<EngineInfo> PythonBindings::GetEngineInfo()  
    {
        return AZ::Failure();
    }

    bool PythonBindings::SetEngineInfo([[maybe_unused]] const EngineInfo& engineInfo)  
    {
        return false;
    }

    AZ::Outcome<GemInfo> PythonBindings::GetGem(const QString& path)  
    {
        GemInfo gemInfo = GemInfoFromPath(pybind11::str(path.toStdString()));
        if (gemInfo.IsValid())
        {
            return AZ::Success(AZStd::move(gemInfo)); 
        }
        else
        {
            return AZ::Failure();
        }
    }

    AZ::Outcome<QVector<GemInfo>> PythonBindings::GetGems()  
    {
        QVector<GemInfo> gems;

        bool result = ExecuteWithLock([&] {
            // external gems 
            for (auto path : m_registration.attr("get_gems")())
            {
                gems.push_back(GemInfoFromPath(path));
            }

            // gems from the engine 
            for (auto path : m_registration.attr("get_engine_gems")())
            {
                gems.push_back(GemInfoFromPath(path));
            }
        });

        if (!result)
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(gems)); 
        }
    }

    AZ::Outcome<ProjectInfo> PythonBindings::CreateProject([[maybe_unused]] const ProjectTemplateInfo& projectTemplate,[[maybe_unused]]  const ProjectInfo& projectInfo)  
    {
        return AZ::Failure();
    }

    AZ::Outcome<ProjectInfo> PythonBindings::GetProject(const QString& path)  
    {
        ProjectInfo projectInfo = ProjectInfoFromPath(pybind11::str(path.toStdString()));
        if (projectInfo.IsValid())
        {
            return AZ::Success(AZStd::move(projectInfo)); 
        }
        else
        {
            return AZ::Failure();
        }
    }

    GemInfo PythonBindings::GemInfoFromPath(pybind11::handle path)
    {
        GemInfo gemInfo;
        gemInfo.m_path = Py_To_String(path); 

        auto data = m_registration.attr("get_gem_data")(pybind11::none(), path);
        if (pybind11::isinstance<pybind11::dict>(data))
        {
            try
            {
                // required
                gemInfo.m_name        = Py_To_String(data["Name"]); 
                gemInfo.m_uuid        = AZ::Uuid(Py_To_String(data["Uuid"])); 

                // optional
                gemInfo.m_displayName = Py_To_String_Optional(data, "DisplayName", gemInfo.m_name); 
                gemInfo.m_summary     = Py_To_String_Optional(data, "Summary", ""); 
                gemInfo.m_version     = Py_To_String_Optional(data, "Version", ""); 

                if (data.contains("Dependencies"))
                {
                    for (auto dependency : data["Dependencies"])
                    {
                        gemInfo.m_dependingGemUuids.push_back(AZ::Uuid(Py_To_String(dependency["Uuid"])));
                    }
                }
                if (data.contains("Tags"))
                {
                    for (auto tag : data["Tags"])
                    {
                        gemInfo.m_features.push_back(Py_To_String(tag));
                    }
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

        auto projectData = m_registration.attr("get_project_data")(pybind11::none(), path);
        if (pybind11::isinstance<pybind11::dict>(projectData))
        {
            try
            {
                // required fields
                projectInfo.m_productName = Py_To_String(projectData["product_name"]); 
                projectInfo.m_projectName = Py_To_String(projectData["project_name"]); 
                projectInfo.m_projectId   = AZ::Uuid(Py_To_String(projectData["project_id"])); 
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
            for (auto path : m_registration.attr("get_projects")())
            {
                projects.push_back(ProjectInfoFromPath(path));
            }

            // projects from the engine 
            for (auto path : m_registration.attr("get_engine_projects")())
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

    bool PythonBindings::UpdateProject([[maybe_unused]] const ProjectInfo& projectInfo)  
    {
        return false;
    }

    ProjectTemplateInfo PythonBindings::ProjectTemplateInfoFromPath(pybind11::handle path)
    {
        ProjectTemplateInfo templateInfo;
        templateInfo.m_path = Py_To_String(path); 

        auto data = m_registration.attr("get_template_data")(pybind11::none(), path);
        if (pybind11::isinstance<pybind11::dict>(data))
        {
            try
            {
                // required
                templateInfo.m_displayName = Py_To_String(data["display_name"]); 
                templateInfo.m_name        = Py_To_String(data["template_name"]); 
                templateInfo.m_summary     = Py_To_String(data["summary"]); 
                
                // optional
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
                        templateInfo.m_canonicalTags.push_back(Py_To_String(tag));
                    }
                }
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                AZ_Warning("PythonBindings", false, "Failed to get ProjectTemplateInfo for %s", Py_To_String(path));
            }
        }

        return templateInfo;
    }

    AZ::Outcome<QVector<ProjectTemplateInfo>> PythonBindings::GetProjectTemplates()  
    {
        QVector<ProjectTemplateInfo> templates;

        bool result = ExecuteWithLock([&] {
            for (auto path : m_registration.attr("get_project_templates")())
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
}
