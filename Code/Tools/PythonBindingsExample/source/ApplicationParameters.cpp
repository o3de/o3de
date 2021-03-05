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
#include <source/ApplicationParameters.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

namespace PythonBindingsExample
{
    void ApplicationParameters::ShowHelp()
    {
        constexpr const char* helpText =
R"(PythonBindingsExample - An example of how to bind the Behavior Context in a simple Tools Application

PythonBindingsExample.exe --file path/to/file.py --arg one --arg two
--help Prints the help text
--verbose (v) Uses verbose output
--file (f) Execute this Python script file
--arg (a) Any number of args sent to the Python script
--interactive (i) Run in interactive mode after file and/or statement (note: enable --verbose to get Python output)
--statement (s) Run Python string statement)";
        puts(helpText);
    }

    bool ApplicationParameters::Parse(const AzFramework::CommandLine* commandLine)
    {
        if (commandLine->GetSwitchList().empty())
        {
            ShowHelp();
            return false;
        }

        auto&& switchList = commandLine->GetSwitchList();
        for (auto&& switchItem : switchList)
        {
            if (switchItem.first == "help")
            {
                ShowHelp();
                return false;
            }
            else if (switchItem.first.starts_with('v'))
            {
                m_verbose = true;
            }
            else if (switchItem.first.starts_with('f'))
            {
                m_pythonFilename = switchItem.second[0];
            }
            else if (switchItem.first.starts_with('a'))
            {
                m_pythonArgs.push_back(switchItem.second[0]);
            }
            else if (switchItem.first.starts_with('s'))
            {
                m_pythonStatement = switchItem.second[0];
            }
            else if (switchItem.first.starts_with('i'))
            {
                m_interactiveMode = true;
            }
            else
            {
                AZ_Warning("python_app", false, "Unknown switch %s \n", switchItem.first.c_str());
            }
        }
        return true;
    }
}
