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

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>

namespace ScriptCanvas::Developer
{

    //! This action waits for the signal from AssetEditorNotificationBus that signals the active graph
    //! has changed, meaning that in the context of these tests, the newly created graph is in focus.
    class WaitForNewGraphAction
        : public EditorAutomationAction
        , public GraphCanvas::AssetEditorNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(WaitForNewGraphAction, AZ::SystemAllocator);
        AZ_RTTI(WaitForNewGraphAction, "{0632736B-ACC3-4051-9772-3A7169B375E9}", EditorAutomationAction);

        WaitForNewGraphAction();
        ~WaitForNewGraphAction() override;

        bool Tick() override;

        const GraphCanvas::GraphId& GetGraphId() const { return m_graphId; }

    private:

        // AssetEditorNotificationBus...
        void OnActiveGraphChanged(const AZ::EntityId& graphId) override;


        bool m_newGraphCreated = false;
        AZ::EntityId m_graphId;
    };


    //! Action that will create a new runtime graph using the toolbar button
    class CreateNewGraphAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateNewGraphAction, AZ::SystemAllocator);
        AZ_RTTI(CreateNewGraphAction, "{26A316D4-ADCC-4796-A3C5-E0545EB96C0E}", CompoundAction);

        CreateNewGraphAction();
        ~CreateNewGraphAction() override = default;

        void SetupAction() override;

        GraphCanvas::GraphId GetGraphId() const;

        ActionReport GenerateReport() const override;

    protected:

        void OnActionsComplete() override;

        GraphCanvas::GraphId m_graphId;
        WaitForNewGraphAction* m_newGraphAction;
    };

    //! Action that will create a new function graph using the toolbar button
    class CreateNewFunctionAction
        : public CreateNewGraphAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateNewFunctionAction, AZ::SystemAllocator);
        AZ_RTTI(CreateNewFunctionAction, "{EF730E0B-AA26-4A81-BC48-22FBEB06CBF9}", CreateNewGraphAction);

        CreateNewFunctionAction();
        ~CreateNewFunctionAction() override = default;

        void SetupAction() override;

        ActionReport GenerateReport() const override;

    protected:

        void OnActionsComplete() override;

    private:

        GraphCanvas::GraphId m_graphId;
    };

    //! Action that will forcibly close the active graph, bypassing any prompts for saving
    class ForceCloseActiveGraphAction
        : public ProcessUserEventsAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceCloseActiveGraphAction, AZ::SystemAllocator);
        AZ_RTTI(ForceCloseActiveGraphAction, "{39BE4561-49A3-4087-BC70-15773EC2117F}", ProcessUserEventsAction);

        ForceCloseActiveGraphAction();
        ~ForceCloseActiveGraphAction() override = default;

        void SetupAction() override;
        bool Tick() override;

        ActionReport GenerateReport() const override;

    private:

        GraphCanvas::GraphId m_activeGraphId;

        bool m_firstTick = false;
    };
}
