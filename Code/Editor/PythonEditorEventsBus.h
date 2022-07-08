#pragma once
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/any.h>

namespace AzToolsFramework
{
    /**
    * This bus can be used to send commands to the editor.
    */
    class EditorLayerPythonRequests
        : public AZ::ComponentBus
    {
    public:

        /*
        * Gets a CVar value as a string.
        */
        virtual const char* GetCVar(const char* pName) = 0;

        /*
        * Sets a CVar value from any simple value.
        */
        virtual void SetCVar(const char* pName, const AZStd::any& value) = 0;

        /*
        * Sets a CVar value from a string.
        */
        virtual void SetCVarFromString(const char* pName, const char* pValue) = 0;

        /*
        * Sets a CVar value from an integer.
        */
        virtual void SetCVarFromInteger(const char* pName, int pValue) = 0;

        /*
        * Sets a CVar value from a float.
        */
        virtual void SetCVarFromFloat(const char* pName, float pValue) = 0;

        /*
        * Runs a console command.
        */
        virtual void PyRunConsole(const char* text) = 0;

        /*
        * Enters the editor game mode.
        */
        virtual void EnterGameMode() = 0;

        /*
        * Queries if it's in the game mode or not.
        */
        virtual bool IsInGameMode() = 0;

        /*
        * Exits the editor game mode.
        */
        virtual void ExitGameMode() = 0;

        /*
        * Enters the editor AI/Physics simulation mode.
        */
        virtual void EnterSimulationMode() = 0;

        /*
        * Queries if the editor is currently in the AI/Physics simulation mode or not.
        */
        virtual bool IsInSimulationMode() = 0;

        /*
        * Exits the editor AI/Physics simulation mode.
        */
        virtual void ExitSimulationMode() = 0;

        /*
        * Runs a script file. A relative path from the editor user folder or an absolute path should be given as an argument.
        */
        virtual void RunFile(const char* pFile) = 0;

        /*
        * Runs a script file with parameters. A relative path from the editor user folder or an absolute path should be given as an argument. The arguments should be separated by whitespace.
        */
        virtual void RunFileParameters(const char* pFile, const char* pArguments) = 0;

        /*
        * Executes a given string as an editor command.
        */
        virtual void ExecuteCommand(const char* cmdline) = 0;

        /*
        * Shows a confirmation message box with ok|cancel and shows a custom message.
        */
        virtual bool MessageBoxOkCancel(const char* pMessage) = 0;

        /*
        * Shows a confirmation message box with yes|no and shows a custom message.
        */
        virtual bool MessageBoxYesNo(const char* pMessage) = 0;

        /*
        * Shows a confirmation message box with only ok and shows a custom message.
        */
        virtual bool MessageBoxOk(const char* pMessage) = 0;

        /*
        * Shows an edit box and returns the value as string.
        */
        virtual AZStd::string EditBox(AZStd::string_view pTitle) = 0;

        /*
        * Shows an edit box and checks the custom value to use the return value with other functions correctly.
        */
        virtual AZStd::any EditBoxCheckDataType(const char* pTitle) = 0;

        /*
        * Shows an open file box and returns the selected file path and name.
        */
        virtual AZStd::string OpenFileBox() = 0;

        /*
        * Gets axis.
        */
        virtual const char* GetAxisConstraint() = 0;

        /*
        * Sets axis.
        */
        virtual void SetAxisConstraint(AZStd::string_view pConstrain) = 0;

        /*
        * Finds a pak file name for a given file.
        */
        virtual AZ::IO::Path GetPakFromFile(const char* filename) = 0;

        /*
        * Prints the message to the editor console window.
        */
        virtual void Log(const char* pMessage) = 0;

        /*
        * Undoes the last operation.
        */
        virtual void Undo() = 0;

        /*
        * Redoes the last undone operation.
        */
        virtual void Redo() = 0;

        /*
        * Shows a 2d label on the screen at the given position and given color.
        */
        virtual void DrawLabel(int x, int y, float size, float r, float g, float b, float a, const char* pLabel) = 0;

        /*
        * Shows a combo box listing each value passed in, returns string value selected by the user.
        */
        virtual AZStd::string ComboBox(AZStd::string title, AZStd::vector<AZStd::string> values, int selectedIdx = 0) = 0;
    };
    using EditorLayerPythonRequestBus = AZ::EBus<EditorLayerPythonRequests>;
}
