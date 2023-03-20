/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Types/Types.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/GraphActions.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationState that will create a runtime graph
    */
    DefineStateId(CreateRuntimeGraphState);

    class CreateRuntimeGraphState 
    : public StaticIdAutomationState<CreateRuntimeGraphStateId>
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateRuntimeGraphState, AZ::SystemAllocator)

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
        AZ_CLASS_ALLOCATOR(CreateFunctionGraphState, AZ::SystemAllocator)

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
        AZ_CLASS_ALLOCATOR(ForceCloseActiveGraphState, AZ::SystemAllocator)

        ForceCloseActiveGraphState();
        ~ForceCloseActiveGraphState() override = default;

    protected:

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;

    private:

        ForceCloseActiveGraphAction m_forceCloseActiveGraph;
    };
}
