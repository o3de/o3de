/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphVisualNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeConnection.h>


namespace EMotionFX
{
    class AnimGraphStateMachine;
    class AnimGraphStateTransition;
}

namespace EMStudio
{
    class AnimGraphPlugin;

    class StateMachineColors
    {
    public:
        static const QColor s_transitionColor;
        static const QColor s_interruptedColor;
        static const QColor s_activeColor;
        static const QColor s_interruptionCandidateColor;
        static const QColor s_selectedColor;
    };

    class StateConnection
        : public NodeConnection
    {
        MCORE_MEMORYOBJECTCATEGORY(StateConnection, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
    public:
        enum
        {
            TYPE_ID = 0x00000002
        };
        StateConnection(NodeGraph* parentGraph, const QModelIndex& modelIndex, GraphNode* sourceNode, GraphNode* targetNode, bool isWildcardConnection);
        ~StateConnection();

        void Render(const QItemSelectionModel& selectionModel, QPainter& painter, QPen* pen, QBrush* brush, int32 stepSize, const QRect& visibleRect, float opacity, bool alwaysColor) override;
        bool Intersects(const QRect& rect) override;
        bool CheckIfIsCloseTo(const QPoint& point) override;
        void CalcStartAndEndPoints(QPoint& start, QPoint& end) const override;

        bool CheckIfIsCloseToHead(const QPoint& point) const override;
        bool CheckIfIsCloseToTail(const QPoint& point) const override;

        uint32 GetType() const override                     { return TYPE_ID; }

        EMotionFX::AnimGraphTransitionCondition* FindCondition(const QPoint& mousePos);

        bool GetIsWildcardTransition() const override       { return m_isWildcardConnection; }

        static void RenderTransition(QPainter& painter, QBrush& brush, QPen& pen,
            QPoint start, QPoint end,
            const QColor& color, const QColor& activeColor,
            bool isSelected, bool isDashed, bool isActive, float weight, bool highlightHead, bool gradientActiveIndicator);

        static void RenderInterruptedTransitions(QPainter& painter, EMStudio::AnimGraphModel& animGraphModel, EMStudio::NodeGraph& nodeGraph);

    private:
        void RenderConditionsAndActions(EMotionFX::AnimGraphInstance* animGraphInstance, QPainter* painter, QPen* pen, QBrush* brush, QPoint& start, QPoint& end);

        bool m_isWildcardConnection;
    };


    // the blend graph node
    class StateGraphNode
        : public AnimGraphVisualNode
    {
        MCORE_MEMORYOBJECTCATEGORY(StateGraphNode, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        enum
        {
            TYPE_ID = 0x00000003
        };

        StateGraphNode(const QModelIndex& modelIndex, AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node);
        ~StateGraphNode();

        void Sync() override;

        void Render(QPainter& painter, QPen* pen, bool renderShadow) override;
        void RenderVisualizeRect(QPainter& painter, const QColor& bgColor, const QColor& bgColor2) override;
        //void Update(const QRect& visibleRect, const QPoint& mousePos) override;

        int32 CalcRequiredHeight() const override;
        int32 CalcRequiredWidth() override;

        QRect CalcInputPortRect(AZ::u16 portNr) override;
        QRect CalcOutputPortRect(AZ::u16 portNr) override;

        void UpdateTextPixmap() override;

        uint32 GetType() const override                     { return StateGraphNode::TYPE_ID; }
    };
}   // namespace EMStudio
