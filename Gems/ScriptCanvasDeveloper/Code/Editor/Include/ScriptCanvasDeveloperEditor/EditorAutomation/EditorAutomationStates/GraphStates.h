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
#pragma once

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Types/Types.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/GraphActions.h>

namespace ScriptCanvasDeveloper
{
    /**
        EditorAutomationState that will create a runtime graph
    */
    DefineStateId(CreateRuntimeGraphState);

    class CreateRuntimeGraphState 
    : public StaticIdAutomationState<CreateRuntimeGraphStateId>
    {
    public:

        CreateRuntimeGraphState();
        ~CreateRuntimeGraphState() override = default;

    protected:

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        CreateNewGraphAction m_createNewGraphAction;
    };

    /**
        EditorAutomationState that will create a function graph
    */
    DefineStateId(CreateFunctionGraphState);

    class CreateFunctionGraphState 
        : public StaticIdAutomationState<CreateFunctionGraphStateId>
    {
    public:

        CreateFunctionGraphState();
        ~CreateFunctionGraphState() override = default;

    protected:

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;

        void OnStateActionsComplete() override;

    private:

        CreateNewFunctionAction m_createNewFunctionAction;
    };

    /**
        EditorAutomationState that will force close the currently opened graph, bypassing any save prompts.
    */
    DefineStateId(ForceCloseActiveGraphState);

    class ForceCloseActiveGraphState 
        : public StaticIdAutomationState<ForceCloseActiveGraphStateId>
    {
    public:

        ForceCloseActiveGraphState();
        ~ForceCloseActiveGraphState() override = default;

    protected:

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;

    private:

        ForceCloseActiveGraphAction m_forceCloseActiveGraph;
    };
}
