/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        if (commandLine->empty())
        {
            ShowHelp();
            return false;
        }

        for (auto&& switchItem : *commandLine)
        {
            if (switchItem.m_option == "help")
            {
                ShowHelp();
                return false;
            }
            else if (switchItem.m_option.starts_with('v'))
            {
                m_verbose = true;
            }
            else if (switchItem.m_option.starts_with('f'))
            {
                m_pythonFilename = switchItem.m_value;
            }
            else if (switchItem.m_option.starts_with('a'))
            {
                m_pythonArgs.push_back(switchItem.m_value);
            }
            else if (switchItem.m_option.starts_with('s'))
            {
                m_pythonStatement = switchItem.m_value;
            }
            else if (switchItem.m_option.starts_with('i'))
            {
                m_interactiveMode = true;
            }
            else if (switchItem.m_option.starts_with("regset"))
            {
                // skip
            }
            else
            {
                AZ_Warning("python_app", false, "Unknown switch %s \n", switchItem.m_option.c_str());
            }
        }
        return true;
    }
}
