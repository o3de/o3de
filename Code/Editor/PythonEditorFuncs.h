/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Include/SandboxAPI.h>
#include <AzCore/Component/Component.h>
#include "PythonEditorEventsBus.h"

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for the Editor
    class PythonEditorFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(PythonEditorFuncsHandler, "{0F470E7E-9741-4608-84B1-7E4735FDA526}")

        SANDBOX_API static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

    //! Component to access the PythonEditorFuncs
    class PythonEditorComponent final
        : public AZ::Component
        , public EditorLayerPythonRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PythonEditorComponent, "{B06810A1-E3C0-4A63-8DDD-3A01C5299DD3}")

        PythonEditorComponent() = default;
        ~PythonEditorComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        // Component...
        void Activate() override;
        void Deactivate() override;

        const char* GetCVar(const char* pName) override;

        void SetCVar(const char* pName, const AZStd::any& value) override;

        void SetCVarFromString(const char* pName, const char* pValue) override;

        void SetCVarFromInteger(const char* pName, int pValue) override;

        void SetCVarFromFloat(const char* pName, float pValue) override;

        void PyRunConsole(const char* text) override;

        void EnterGameMode() override;

        bool IsInGameMode() override;

        void ExitGameMode() override;

        void EnterSimulationMode() override;

        bool IsInSimulationMode() override;

        void ExitSimulationMode() override;

        void RunFile(const char* pFile) override;

        void RunFileParameters(const char* pFile, const char* pArguments) override;

        void ExecuteCommand(const char* cmdline) override;

        bool MessageBoxOkCancel(const char* pMessage) override;

        bool MessageBoxYesNo(const char* pMessage) override;

        bool MessageBoxOk(const char* pMessage) override;

        AZStd::string EditBox(AZStd::string_view pTitle) override;

        AZStd::any EditBoxCheckDataType(const char* pTitle) override;

        AZStd::string OpenFileBox() override;

        const char* GetAxisConstraint() override;

        void SetAxisConstraint(AZStd::string_view pConstrain) override;

        AZ::IO::Path GetPakFromFile(const char* filename) override;

        void Log(const char* pMessage) override;

        void Undo() override;

        void Redo() override;

        void DrawLabel(int x, int y, float size, float r, float g, float b, float a, const char* pLabel) override;

        AZStd::string ComboBox(AZStd::string title, AZStd::vector<AZStd::string> values, int selectedIdx = 0) override;
    };

} // namespace AzToolsFramework
