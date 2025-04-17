/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Types/Types.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationAction that will center the specified graph onto the specified point
    */
    class CenterOnScenePointAction
        : public EditorAutomationAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CenterOnScenePointAction, AZ::SystemAllocator);
        AZ_RTTI(CenterOnScenePointAction, "{527B3EE0-258F-4D0C-8642-6923D81A9C40}", EditorAutomationAction);

        CenterOnScenePointAction(GraphCanvas::GraphId graphId, QPointF scenePoint);
        ~CenterOnScenePointAction() override = default;

        bool Tick() override;

    private:

        GraphCanvas::GraphId m_graphId;
        QPointF m_scenePoint;
    };

    /**
        EditorAutomationAction that will ensure that the given rect is visible inside of the specified graph
    */
    class EnsureSceneRectVisibleAction
        : public DelayAction
    {
    public:
        AZ_CLASS_ALLOCATOR(EnsureSceneRectVisibleAction, AZ::SystemAllocator);
        AZ_RTTI(EnsureSceneRectVisibleAction, "{BF412F21-62F1-48AA-891E-266C986820FA}", DelayAction);

        EnsureSceneRectVisibleAction(GraphCanvas::GraphId graphId, QRectF sceneRect);
        ~EnsureSceneRectVisibleAction() override = default;

        void SetupAction() override;
        bool Tick() override;

    private:

        bool m_firstTick = false;

        GraphCanvas::GraphId m_graphId;
        QRectF m_sceneRect;
    };

    /**
        EditorAutomationAction that will move the mouse to the specified point in the scene.
    */
    class SceneMouseMoveAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(SceneMouseMoveAction, AZ::SystemAllocator);
        AZ_RTTI(SceneMouseMoveAction, "{A26D0034-0682-4F61-9425-EF659D1ABAD0}", CompoundAction);

        SceneMouseMoveAction(GraphCanvas::GraphId graphId, QPointF scenePoint);
        ~SceneMouseMoveAction() override = default;

        bool IsMissingPrecondition() override;
        EditorAutomationAction* GenerateMissingPreconditionAction() override;

        void SetupAction() override;

    private:

        GraphCanvas::GraphId m_graphId;
        GraphCanvas::ViewId  m_viewId;

        QPointF m_scenePoint;
    };

    /**
        EditorAutomationAction that will drag the mouse between the two specified points in the scene
    */
    class SceneMouseDragAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(SceneMouseDragAction, AZ::SystemAllocator);
        AZ_RTTI(SceneMouseDragAction, "{5A422603-1A84-40B2-B051-94D324F94C7A}", CompoundAction);

        SceneMouseDragAction(GraphCanvas::GraphId graphId, QPointF sceneStart, QPointF sceneEnd, Qt::MouseButton mouseButton = Qt::MouseButton::LeftButton);
        ~SceneMouseDragAction() override = default;

        bool IsMissingPrecondition() override;
        EditorAutomationAction* GenerateMissingPreconditionAction() override;

        void SetupAction() override;

    private:

        GraphCanvas::GraphId m_graphId;
        GraphCanvas::ViewId  m_viewId;

        QPointF m_sceneStart;
        QPointF m_sceneEnd;

        Qt::MouseButton m_mouseButton;
    };
}
