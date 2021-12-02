/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributesWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/StateGraphNode.h>
#include <MCore/Source/Compare.h>


namespace EMStudio
{
    const QColor StateMachineColors::s_transitionColor = QColor(125, 125, 125);
    const QColor StateMachineColors::s_activeColor = QColor(4, 255, 0);
    const QColor StateMachineColors::s_interruptedColor = QColor(255, 0, 255);
    const QColor StateMachineColors::s_interruptionCandidateColor = QColor(63, 140, 62);
    const QColor StateMachineColors::s_selectedColor = QColor(255, 128, 0);

    StateConnection::StateConnection(NodeGraph* parentGraph, const QModelIndex& modelIndex, GraphNode* sourceNode, GraphNode* targetNode, bool isWildcardConnection)
        : NodeConnection(parentGraph, modelIndex, targetNode, 0, sourceNode, 0)
    {
        m_color                  = StateMachineColors::s_transitionColor;
        m_isWildcardConnection   = isWildcardConnection;
    }


    StateConnection::~StateConnection()
    {
    }


    void StateConnection::Render([[maybe_unused]] const QItemSelectionModel& selectionModel, QPainter& painter, QPen* pen, QBrush* brush, int32 stepSize, const QRect& visibleRect, float opacity, bool alwaysColor)
    {
        MCORE_UNUSED(stepSize);
        MCORE_UNUSED(visibleRect);
        MCORE_UNUSED(opacity);
        MCORE_UNUSED(alwaysColor);

        QPoint start, end;
        CalcStartAndEndPoints(start, end);

        // Adjust the start and end points in case this is a wildcard transition.
        if (m_isWildcardConnection)
        {
            start = end - QPoint(WILDCARDTRANSITION_SIZE, WILDCARDTRANSITION_SIZE);
            end += QPoint(3, 3);
        }

        const EMotionFX::AnimGraphStateTransition* transition = m_modelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        if (!transition)
        {
            AZ_Error("EMotionFX", false, "Cannot render transition, model index is invalid.");
            return;
        }

        const EMotionFX::AnimGraphNode* targetState = transition->GetTargetNode();
        if (!targetState)
        {
            AZ_Error("EMotionFX", false, "The target state always is expected to be valid. Cannot render transition.");
            return;
        }

        EMotionFX::AnimGraphStateMachine* stateMachine = azdynamic_cast<EMotionFX::AnimGraphStateMachine*>(targetState->GetParentNode());
        if (!stateMachine)
        {
            AZ_Error("EMotionFX", false, "Cannot render transition. State machine for transition not valid.");
            return;
        }

        EMotionFX::AnimGraphInstance* animGraphInstance = m_modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();

        bool isActive = false;
        bool gotInterrupted = false;
        bool isLatestTransition = false;
        bool isLastInterruptedTransition = false;
        bool isInterruptionCandidate = false;
        float blendWeight = 0.0f;
        size_t numActiveTransitions = 0;
        if (animGraphInstance && animGraphInstance->GetAnimGraph() == transition->GetAnimGraph())
        {
            const AZStd::vector<EMotionFX::AnimGraphStateTransition*>& activeTransitions = stateMachine->GetActiveTransitions(animGraphInstance);
            isActive = stateMachine->IsTransitionActive(transition, animGraphInstance);
            blendWeight = transition->GetBlendWeight(animGraphInstance);

            const EMotionFX::AnimGraphStateTransition* latestActiveTransition = stateMachine->GetLatestActiveTransition(animGraphInstance);
            isLatestTransition = (transition == latestActiveTransition);
            numActiveTransitions = activeTransitions.size();
            gotInterrupted = transition->GotInterrupted(animGraphInstance);

            if (numActiveTransitions > 1)
            {
                isLastInterruptedTransition = (transition == activeTransitions.back());
            }

            if (latestActiveTransition && latestActiveTransition->CanBeInterruptedBy(transition, animGraphInstance) && !isActive)
            {
                isInterruptionCandidate = true;
            }
        }

        bool interruptionSelectionMode = false;
        QModelIndex attributeWindowModelIndex;
        AttributesWindowRequestBus::BroadcastResult(attributeWindowModelIndex, &AttributesWindowRequests::GetModelIndex);
        if (attributeWindowModelIndex.isValid() && attributeWindowModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::TRANSITION)
        {
            const EMotionFX::AnimGraphStateTransition* attributeWindowTransition = attributeWindowModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
            if (attributeWindowTransition)
            {
                bool isLocked = false;
                AttributesWindowRequestBus::BroadcastResult(isLocked, &AttributesWindowRequests::IsLocked);

                if (isLocked && attributeWindowTransition == transition)
                {
                    interruptionSelectionMode = true;
                }

                if (attributeWindowTransition->CanBeInterruptedBy(transition))
                {
                    isInterruptionCandidate = true;
                }
            }
        }

        QColor color = m_color;

        if (GetIsSelected())
        {
            color = StateMachineColors::s_selectedColor;
        }
        else if (isInterruptionCandidate)
        {
            color = StateMachineColors::s_interruptionCandidateColor;
        }
        else if (m_isSynced)
        {
            color.setRgb(115, 125, 200);
        }

        // darken the color in case the transition is disabled
        if (m_isDisabled)
        {
            color = color.darker(165);
        }

        // lighten the color in case the transition is highlighted
        if (m_isHighlighted)
        {
            color = color.lighter(150);
            painter.setOpacity(1.0);
        }

        // lighten the color in case the transition is connected to the currently selected node
        if (m_isConnectedHighlighted)
        {
            pen->setWidth(2);
            color = color.lighter(150);
            painter.setOpacity(1.0);
        }

        bool isSelected = GetIsSelected();
        if (interruptionSelectionMode)
        {
            isSelected = true;
            color = StateMachineColors::s_selectedColor;
            color.setAlphaF(0.5f);
        }

        QColor activeColor = StateMachineColors::s_activeColor;
        if (gotInterrupted)
        {
            activeColor = StateMachineColors::s_interruptedColor;
        }

        const bool showBlendState = isActive &&
            ((!gotInterrupted && isLatestTransition && numActiveTransitions == 1) || isLastInterruptedTransition);

        RenderTransition(painter, *brush, *pen,
            start, end,
            color, activeColor,
            isSelected, /*isDashed=*/m_isDisabled,
            showBlendState, blendWeight,
            /*highlightHead=*/m_isHeadHighlighted && m_isWildcardConnection == false,
            /*gradientActiveIndicator=*/!gotInterrupted);

        if (m_isHeadHighlighted)
        {
            brush->setColor(color);
            painter.setBrush(*brush);
            pen->setColor(color);
            painter.setPen(*pen);
        }

        if (!isActive)
        {
            RenderConditionsAndActions(animGraphInstance, &painter, pen, brush, start, end);
        }
    }

    void StateConnection::RenderConditionsAndActions(EMotionFX::AnimGraphInstance* animGraphInstance, QPainter* painter, QPen* pen, QBrush* brush, QPoint& start, QPoint& end)
    {
        // disable the dash pattern in case the transition is disabled
        pen->setStyle(Qt::SolidLine);
        painter->setPen(*pen);

        const AZ::Vector2   transitionStart(aznumeric_cast<float>(start.rx()), aznumeric_cast<float>(start.ry()));
        const AZ::Vector2   transitionEnd(aznumeric_cast<float>(end.rx()), aznumeric_cast<float>(end.ry()));

        EMotionFX::AnimGraphStateTransition* transition = m_modelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        AZ_Assert(transition, "Expected non-null transition");

        const size_t numConditions = transition->GetNumConditions();
        const size_t numActions = transition->GetTriggerActionSetup().GetNumActions();
        const size_t sumSize = numConditions + numActions;

        // precalculate some values we need for the condition rendering
        const float             shapeDiameter = 3.0f;
        const float             shapeStride = 4.0f;
        const float             elementSize = shapeDiameter + shapeStride;
        const AZ::Vector2       localEnd = transitionEnd - transitionStart;

        // only draw the transition conditions in case the arrow has enough space for it, avoid zero rect sized crashes as well
        if (localEnd.GetLength() > sumSize * elementSize)
        {
            const AZ::Vector2   transitionMid = transitionStart + localEnd * 0.5;
            const AZ::Vector2   transitionDir = localEnd.GetNormalized();
            const AZ::Vector2   conditionStart = transitionMid - transitionDir * (elementSize * 0.5f * (float)(sumSize));
            const AZ::Vector2   actionStart = transitionMid - transitionDir * (elementSize * 0.5f * (float)sumSize) + transitionDir * aznumeric_cast<float>(elementSize) * aznumeric_cast<float>(numConditions);

            for (size_t i = 0; i < numConditions; ++i)
            {
                EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);

                // set the condition color either green if the test went okay or red if the test returned false
                QColor conditionColor;
                if (animGraphInstance)
                {
                    conditionColor = condition->TestCondition(animGraphInstance) ? Qt::green : Qt::red;
                }
                else
                {
                    conditionColor = Qt::gray;
                }

                // darken the color in case the transition is disabled
                if (m_isDisabled)
                {
                    conditionColor = conditionColor.darker(185);
                }

                brush->setColor(conditionColor);

                // calculate the circle middle point
                const AZ::Vector2 circleMid = conditionStart + transitionDir * (elementSize * (float)i);

                // render the circle per condition
                painter->setBrush(*brush);
                painter->drawEllipse(QPointF(circleMid.GetX(), circleMid.GetY()), shapeDiameter, shapeDiameter);
            }

            QColor actionColor = Qt::yellow;
            // darken the color in case the transition is disabled
            if (m_isDisabled)
            {
                actionColor = actionColor.darker(185);
            }

            for (size_t i = 0; i < numActions; ++i)
            {
                brush->setColor(actionColor);

                // calculate the rect left top
                const AZ::Vector2 recLeft = actionStart + transitionDir * (elementSize * (float)i) - AZ::Vector2(shapeDiameter, shapeDiameter);

                // render the rect per action
                painter->setBrush(*brush);
                painter->drawRect(aznumeric_cast<int>(recLeft.GetX()), aznumeric_cast<int>(recLeft.GetY()), aznumeric_cast<int>(shapeDiameter * 2), aznumeric_cast<int>(shapeDiameter * 2));
            }
        }
    }

    EMotionFX::AnimGraphTransitionCondition* StateConnection::FindCondition(const QPoint& mousePos)
    {
        // if the transition is invalid, return directly
        if (!m_modelIndex.isValid())
        {
            return nullptr;
        }

        QPoint start, end;
        CalcStartAndEndPoints(start, end);

        // check if we are dealing with a wildcard transition
        if (m_isWildcardConnection)
        {
            start = end - QPoint(WILDCARDTRANSITION_SIZE, WILDCARDTRANSITION_SIZE);
            end += QPoint(3, 3);
        }

        const AZ::Vector2   transitionStart(aznumeric_cast<float>(start.rx()), aznumeric_cast<float>(start.ry()));
        const AZ::Vector2   transitionEnd(aznumeric_cast<float>(end.rx()), aznumeric_cast<float>(end.ry()));

        EMotionFX::AnimGraphStateTransition* transition = m_modelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        AZ_Assert(transition, "Expected non-null transition");

        const size_t numConditions = transition->GetNumConditions();

        // precalculate some values we need for the condition rendering
        const float             circleDiameter  = 3.0f;
        const float             circleStride    = 4.0f;
        const float             elementSize     = circleDiameter + circleStride;
        const AZ::Vector2   localEnd        = transitionEnd - transitionStart;

        // only draw the transition conditions in case the arrow has enough space for it, avoid zero rect sized crashes as well
        if (localEnd.GetLength() > numConditions * elementSize)
        {
            const AZ::Vector2   transitionMid   = transitionStart + localEnd * 0.5f;
            const AZ::Vector2   transitionDir   = localEnd.GetNormalized();
            const AZ::Vector2   conditionStart  = transitionMid - transitionDir * (elementSize * 0.5f * (float)numConditions);

            // iterate through the conditions and render them
            for (size_t i = 0; i < numConditions; ++i)
            {
                // get access to the condition
                EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);

                // calculate the circle middle point
                const AZ::Vector2 circleMid = conditionStart  + transitionDir * (elementSize * (float)i);

                const float distance = AZ::Vector2(AZ::Vector2(aznumeric_cast<float>(mousePos.x()), aznumeric_cast<float>(mousePos.y())) - circleMid).GetLength();
                if (distance <= circleDiameter)
                {
                    return condition;
                }
            }
        }

        return nullptr;
    }

    bool StateConnection::Intersects(const QRect& rect)
    {
        QPoint start, end;
        CalcStartAndEndPoints(start, end);
        return NodeGraph::LineIntersectsRect(rect, aznumeric_cast<float>(start.x()), aznumeric_cast<float>(start.y()), aznumeric_cast<float>(end.x()), aznumeric_cast<float>(end.y()));
    }

    bool StateConnection::CheckIfIsCloseTo(const QPoint& point)
    {
        QPoint start, end;
        CalcStartAndEndPoints(start, end);
        return (NodeGraph::DistanceToLine(aznumeric_cast<float>(start.x()), aznumeric_cast<float>(start.y()), aznumeric_cast<float>(end.x()), aznumeric_cast<float>(end.y()), aznumeric_cast<float>(point.x()), aznumeric_cast<float>(point.y())) <= 5.0f);

        //QRect testRect(point.x() - 1, point.y() - 1, 2, 2);
        //return Intersects(testRect);
    }

    bool StateConnection::CheckIfIsCloseToHead(const QPoint& point) const
    {
        QPoint start, end;
        CalcStartAndEndPoints(start, end);

        AZ::Vector2 dir = AZ::Vector2(aznumeric_cast<float>(end.x() - start.x()), aznumeric_cast<float>(end.y() - start.y()));
        dir.Normalize();
        AZ::Vector2 newStart = AZ::Vector2(aznumeric_cast<float>(end.x()), aznumeric_cast<float>(end.y())) - dir * 5.0f;

        return (NodeGraph::DistanceToLine(newStart.GetX(), newStart.GetY(), aznumeric_cast<float>(end.x()), aznumeric_cast<float>(end.y()), aznumeric_cast<float>(point.x()), aznumeric_cast<float>(point.y())) <= 7.0f);
    }

    bool StateConnection::CheckIfIsCloseToTail(const QPoint& point) const
    {
        QPoint start, end;
        CalcStartAndEndPoints(start, end);

        AZ::Vector2 dir = AZ::Vector2(aznumeric_cast<float>(end.x() - start.x()), aznumeric_cast<float>(end.y() - start.y()));
        dir.Normalize();
        AZ::Vector2 newStart = AZ::Vector2(aznumeric_cast<float>(start.x()), aznumeric_cast<float>(start.y())) + dir * 6.0f;

        return (AZ::Vector2(newStart - AZ::Vector2(aznumeric_cast<float>(point.x()), aznumeric_cast<float>(point.y()))).GetLength() <= 6.0f);
    }

    void StateConnection::CalcStartAndEndPoints(QPoint& outStart, QPoint& outEnd) const
    {
        EMotionFX::AnimGraphStateTransition* transition = m_modelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        const QPoint startOffset = QPoint(transition->GetVisualStartOffsetX(), transition->GetVisualStartOffsetY());
        const QPoint endOffset = QPoint(transition->GetVisualEndOffsetX(), transition->GetVisualEndOffsetY());

        QPoint start = startOffset;
        QPoint end = m_targetNode->GetRect().topLeft() + endOffset;
        if (m_sourceNode)
        {
            start += m_sourceNode->GetRect().topLeft();
        }
        else
        {
            start = end - QPoint(WILDCARDTRANSITION_SIZE, WILDCARDTRANSITION_SIZE);
        }

        QRect sourceRect;
        if (m_sourceNode)
        {
            sourceRect = m_sourceNode->GetRect();
        }

        QRect targetRect = m_targetNode->GetRect();
        targetRect.adjust(-2, -2, 2, 2);

        // calc the real start point
        double realX, realY;
        if (NodeGraph::LineIntersectsRect(sourceRect, aznumeric_cast<float>(start.x()), aznumeric_cast<float>(start.y()), aznumeric_cast<float>(end.x()), aznumeric_cast<float>(end.y()), &realX, &realY))
        {
            start.setX(aznumeric_cast<int>(realX));
            start.setY(aznumeric_cast<int>(realY));
        }

        // calc the real end point
        if (NodeGraph::LineIntersectsRect(targetRect, aznumeric_cast<float>(start.x()), aznumeric_cast<float>(start.y()), aznumeric_cast<float>(end.x()), aznumeric_cast<float>(end.y()), &realX, &realY))
        {
            end.setX(aznumeric_cast<int>(realX));
            end.setY(aznumeric_cast<int>(realY));
        }

        outStart    = start;
        outEnd      = end;
    }

    void StateConnection::RenderTransition(QPainter& painter, QBrush& brush, QPen& pen,
        QPoint start, QPoint end,
        const QColor& color, const QColor& activeColor,
        bool isSelected, bool isDashed, bool isActive, float weight, bool highlightHead, bool gradientActiveIndicator)
    {
        const AZ::Vector2 azStart = AZ::Vector2(aznumeric_cast<float>(start.x()), aznumeric_cast<float>(start.y()));
        const AZ::Vector2 azEnd = AZ::Vector2(aznumeric_cast<float>(end.x()), aznumeric_cast<float>(end.y()));
        AZ::Vector2 azStartEnd = azEnd - azStart;

        // Skip degenerated transitions (in case nodes are moved close or over each other).
        if (MCore::Compare<float>::CheckIfIsClose(azStartEnd.GetX(), 0.0f, MCore::Math::epsilon) &&
            MCore::Compare<float>::CheckIfIsClose(azStartEnd.GetY(), 0.0f, MCore::Math::epsilon))
        {
            return;
        }

        const float length = azStartEnd.GetLength();
        AZ::Vector2 lineDir = azStartEnd;
        lineDir.Normalize();

        QPointF direction;
        direction.setX(lineDir.GetX() * 8.0f);
        direction.setY(lineDir.GetY() * 8.0f);

        QPointF normalOffset((end.y() - start.y()) / length, (start.x() - end.x()) / length);

        QPointF headPoints[3];
        headPoints[0] = end;
        headPoints[1] = end - direction + (normalOffset * 5.0f);
        headPoints[2] = end - direction - (normalOffset * 5.0f);

        pen.setStyle(Qt::SolidLine);
        brush.setStyle(Qt::SolidPattern);

        if (isDashed)
        {
            pen.setStyle(Qt::DashLine);
            painter.setPen(pen);
        }
        else
        {
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);
        }

        if (isActive)
        {
            pen.setWidthF(1.0f);
        }
        else if (isSelected)
        {
            pen.setWidthF(2.0f);
        }
        else
        {
            pen.setWidthF(1.5f);
        }

        painter.setBrush(color);
        pen.setColor(color);
        pen.setBrush(color);
        painter.setPen(pen);
        painter.drawLine(start, end);

        if (highlightHead)
        {
            QColor headTailColor(0, 255, 0);
            brush.setColor(headTailColor);
            painter.setBrush(brush);
            pen.setColor(headTailColor);
            painter.setPen(pen);
        }
        else
        {
            painter.setBrush(color);
            pen.setBrush(color);
            painter.setPen(pen);
        }

        brush.setStyle(Qt::SolidPattern);
        painter.drawPolygon(headPoints, 3);

        if (isActive)
        {
            pen.setWidthF(3.0f);

            QLinearGradient gradient(start, end);

            if (MCore::Compare<float>::CheckIfIsClose(weight, 1.0f, MCore::Math::epsilon))
            {
                painter.setBrush(activeColor);
                pen.setBrush(activeColor);
                painter.setPen(pen);
            }
            else
            {
                float gradientLength = 0.001f;
                if (gradientActiveIndicator)
                {
                    gradientLength = 0.1f;
                }

                const QColor transparent(0, 0, 0, 0);
                gradient.setColorAt(0.0, activeColor);
                gradient.setColorAt(MCore::Clamp(weight, 0.0f, 1.0f), activeColor);
                gradient.setColorAt(MCore::Clamp(weight + gradientLength, 0.0f, 1.0f), transparent);
                gradient.setColorAt(1.0, transparent);

                painter.setBrush(gradient);
                pen.setBrush(gradient);
                painter.setPen(pen);
            }

            painter.drawLine(start, end);

            pen.setWidthF(1.0f);
            painter.setPen(pen);
            painter.drawPolygon(headPoints, 3);
        }

        pen.setWidthF(1.0f);
    }

    void StateConnection::RenderInterruptedTransitions(QPainter& painter, EMStudio::AnimGraphModel& animGraphModel, EMStudio::NodeGraph& nodeGraph)
    {
        const QModelIndex& nodeGraphModelIndex = nodeGraph.GetModelIndex();
        if (!nodeGraphModelIndex.isValid() ||
            nodeGraphModelIndex.data(EMStudio::AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<EMStudio::AnimGraphModel::ModelItemType>() != EMStudio::AnimGraphModel::ModelItemType::NODE)
        {
            return;
        }

        const EMotionFX::AnimGraphNode* node = nodeGraphModelIndex.data(EMStudio::AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        const EMotionFX::AnimGraphStateMachine* stateMachine = azdynamic_cast<const EMotionFX::AnimGraphStateMachine*>(node);
        if (!stateMachine)
        {
            // We might be viewing a blend tree, nothing to do here.
            return;
        }

        EMotionFX::AnimGraphInstance* animGraphInstance = nodeGraphModelIndex.data(EMStudio::AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
        if (!animGraphInstance || animGraphInstance->GetAnimGraph() != stateMachine->GetAnimGraph())
        {
            return;
        }

        const AZStd::vector<EMotionFX::AnimGraphStateTransition*>& activeTransitions = stateMachine->GetActiveTransitions(animGraphInstance);
        const size_t numActiveTransitions = activeTransitions.size();
        if (numActiveTransitions < 2)
        {
            // No transition interrupted, nothing to do here.
            return;
        }

        QBrush brush;
        QPen pen;

        EMotionFX::AnimGraphStateTransition* latestActiveTransition = activeTransitions[numActiveTransitions - 1];
        QModelIndex latestActiveTransitionModelIndex = animGraphModel.FindModelIndex(latestActiveTransition, animGraphInstance);
        EMStudio::StateConnection* latestActiveVisualTransition = nodeGraph.FindStateConnection(latestActiveTransitionModelIndex);

        float previousTransitionBlendWeight = latestActiveTransition->GetBlendWeight(animGraphInstance);
        QPoint previousTransitionStart;
        QPoint previousTransitionEnd;
        latestActiveVisualTransition->CalcStartAndEndPoints(previousTransitionStart, previousTransitionEnd);

        // Iterate the transition stack back to front, starting at the first started and interrupted transition and going towards the latest one.
        for (size_t i = 1; i < numActiveTransitions; ++i)
        {
            const size_t activeTransitionIndex = numActiveTransitions - 1 - i;
            EMotionFX::AnimGraphStateTransition* currentTransition = activeTransitions[activeTransitionIndex];
            const QModelIndex& currentTransitionModelIndex = animGraphModel.FindModelIndex(currentTransition, animGraphInstance);
            EMStudio::StateConnection* currentVisualTransition = nodeGraph.FindStateConnection(currentTransitionModelIndex);
            if (currentVisualTransition)
            {
                const float blendWeight = currentTransition->GetBlendWeight(animGraphInstance);
                QPoint transitionStart;
                QPoint transitionEnd;
                currentVisualTransition->CalcStartAndEndPoints(transitionStart, transitionEnd);

                QColor activeColor = EMStudio::StateMachineColors::s_activeColor;
                if (activeTransitionIndex != 0)
                {
                    activeColor = EMStudio::StateMachineColors::s_interruptedColor;
                }

                const QPoint renderedStart = previousTransitionStart + (previousTransitionEnd - previousTransitionStart) * previousTransitionBlendWeight;

                RenderTransition(painter, brush, pen,
                    renderedStart, transitionEnd,
                    EMStudio::StateMachineColors::s_transitionColor, activeColor,
                    /*isSelected=*/ false, /*isDashed=*/ false, /*isActive=*/ true, blendWeight, /*highlightHead=*/ false,
                    /*gradientActiveIndicator=*/ activeTransitionIndex == 0);

                previousTransitionBlendWeight = blendWeight;
                previousTransitionStart = renderedStart;
                previousTransitionEnd = transitionEnd;
            }
            else
            {
                previousTransitionBlendWeight = 0.0f;
                previousTransitionEnd = QPoint(0, 0);
            }
        }
    }

    //--------------------------------------------------------------------------------
    // class StateGraphNode
    //--------------------------------------------------------------------------------
    StateGraphNode::StateGraphNode(const QModelIndex& modelIndex, AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node)
        : AnimGraphVisualNode(modelIndex, plugin, node)
    {
        ResetBorderColor();
        SetCreateConFromOutputOnly(true);

        m_inputPorts.resize(1);
        m_outputPorts.resize(4);
    }

    StateGraphNode::~StateGraphNode()
    {
    }

    void StateGraphNode::Sync()
    {
        AnimGraphVisualNode::Sync();

        EMotionFX::AnimGraphStateMachine* parentStateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(m_emfxNode->GetParentNode());
        if (parentStateMachine->GetEntryState() == m_emfxNode)
        {
            m_parentGraph->SetEntryNode(this);
        }
    }

    void StateGraphNode::Render(QPainter& painter, QPen* pen, bool renderShadow)
    {
        if (!m_isVisible)
        {
            return;
        }

        if (renderShadow)
        {
            RenderShadow(painter);
        }

        EMotionFX::AnimGraphInstance* animGraphInstance = m_modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();

        bool isActive = false;
        bool gotInterrupted = false;
        if (animGraphInstance && m_emfxNode && animGraphInstance->GetAnimGraph() == m_emfxNode->GetAnimGraph())
        {
            AZ_Assert(azrtti_typeid(m_emfxNode->GetParentNode()) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>(), "Expected a valid state machine.");
            const EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(m_emfxNode->GetParentNode());

            const AZStd::vector<EMotionFX::AnimGraphNode*>& activeStates = stateMachine->GetActiveStates(animGraphInstance);
            if (AZStd::find(activeStates.begin(), activeStates.end(), m_emfxNode) != activeStates.end())
            {
                isActive = true;

                const AZStd::vector<EMotionFX::AnimGraphStateTransition*>& activeTransitions = stateMachine->GetActiveTransitions(animGraphInstance);
                const EMotionFX::AnimGraphStateTransition* latestActiveTransition = stateMachine->GetLatestActiveTransition(animGraphInstance);
                for (const EMotionFX::AnimGraphStateTransition* activeTransition : activeTransitions)
                {
                    if (activeTransition != latestActiveTransition && activeTransition->GetTargetNode() == m_emfxNode)
                    {
                        gotInterrupted = true;
                        break;
                    }
                }
            }
        }

        m_borderColor.setRgb(0, 0, 0);
        if (isActive)
        {
            m_borderColor = StateMachineColors::s_activeColor;
        }
        if (gotInterrupted)
        {
            m_borderColor = StateMachineColors::s_interruptedColor;
        }

        QColor borderColor;
        pen->setWidth(2);
        const bool isSelected = GetIsSelected();

        if (isSelected)
        {
            borderColor = StateMachineColors::s_selectedColor;
        }
        else
        {
            borderColor = m_borderColor;
        }

        // background color
        QColor bgColor;
        if (isSelected)
        {
            bgColor.setRgbF(0.93f, 0.547f, 0.0f, 1.0f); //  rgb(72, 63, 238)
        }
        else
        {
            bgColor = m_baseColor;
        }

        // blinking red error color
        const bool hasError = GetHasError();
        if (hasError && !isSelected)
        {
            if (m_parentGraph->GetUseAnimation())
            {
                borderColor = m_parentGraph->GetErrorBlinkColor();
            }
            else
            {
                borderColor = Qt::red;
            }
        }

        QColor bgColor2;
        bgColor2 = bgColor.lighter(30); // make darker actually, 30% of the old color, same as bgColor * 0.3f;

        QColor textColor = isSelected ? Qt::black : Qt::white;

        // is highlighted/hovered (on-mouse-over effect)
        if (m_isHighlighted)
        {
            bgColor = bgColor.lighter(120);
            bgColor2 = bgColor2.lighter(120);
        }

        // draw the main rect
        {
            QLinearGradient bgGradient(0, m_rect.top(), 0, m_rect.bottom());
            bgGradient.setColorAt(0.0f, bgColor);
            bgGradient.setColorAt(1.0f, bgColor2);
            painter.setBrush(bgGradient);
            painter.setPen(borderColor);
        }

        // add 4px to have empty space for the visualize button
        painter.drawRoundedRect(m_rect, BORDER_RADIUS, BORDER_RADIUS);

        // if the scale is so small that we can still see the small things
        if (m_parentGraph->GetScale() > 0.3f)
        {
            // draw the visualize area
            if (m_canVisualize)
            {
                RenderVisualizeRect(painter, bgColor, bgColor2);
            }

            // render the tracks etc
            if (m_emfxNode->GetHasOutputPose() && m_isProcessed)
            {
                RenderTracks(painter, bgColor, bgColor2, 3);
            }

            // render the marker which indicates that you can go inside this node
            RenderHasChildsIndicator(painter, pen, borderColor, bgColor2);
        }

        painter.setClipping(false);

        // render the text overlay with the pre-baked node name and port names etc.
        const float textOpacity = MCore::Clamp<float>(m_parentGraph->GetScale() * m_parentGraph->GetScale() * 1.5f, 0.0f, 1.0f);
        painter.setOpacity(textOpacity);
        painter.setFont(m_headerFont);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(textColor);
        painter.drawStaticText(m_rect.left(), aznumeric_cast<int>(m_rect.center().y() - m_titleText.size().height() / 2), m_titleText);
        painter.setOpacity(1.0f);

        RenderDebugInfo(painter);
    }

    int32 StateGraphNode::CalcRequiredHeight() const
    {
        return 40;
    }

    int32 StateGraphNode::CalcRequiredWidth()
    {
        const uint32 headerWidth = m_headerFontMetrics->horizontalAdvance(m_elidedName) + 40;

        // make sure the node is at least 100 units in width
        return MCore::Max<uint32>(headerWidth, 100);
    }

    QRect StateGraphNode::CalcInputPortRect(AZ::u16 portNr)
    {
        MCORE_UNUSED(portNr);
        return m_rect.adjusted(10, 10, -10, -10);
    }

    QRect StateGraphNode::CalcOutputPortRect(AZ::u16 portNr)
    {
        switch (portNr)
        {
        case 0:
            return QRect(m_rect.left(),     m_rect.top(),            m_rect.width(),  8);
            break;                                                                                                  // top
        case 1:
            return QRect(m_rect.left(),     m_rect.bottom() - 8,       m_rect.width(),  9);
            break;                                                                                                  // bottom
        case 2:
            return QRect(m_rect.left(),     m_rect.top(),            8,              m_rect.height());
            break;                                                                                                  // left
        case 3:
            return QRect(m_rect.right() - 8,  m_rect.top(),            9,              m_rect.height());
            break;                                                                                                  // right
        default:
            MCORE_ASSERT(false);
            return QRect();
        }
        //MCore::LOG("CalcOutputPortRect: (%i, %i, %i, %i)", rect.top(), rect.left(), rect.bottom(), rect.right());
    }

    void StateGraphNode::RenderVisualizeRect(QPainter& painter, const QColor& bgColor, const QColor& bgColor2)
    {
        MCORE_UNUSED(bgColor2);

        QColor vizBorder;
        if (m_visualize)
        {
            vizBorder = Qt::black;
        }
        else
        {
            vizBorder = bgColor.darker(225);
        }

        painter.setPen(m_visualizeHighlighted ? StateMachineColors::s_selectedColor : vizBorder);
        if (!GetIsSelected())
        {
            painter.setBrush(m_visualize ? m_visualizeColor : bgColor);
        }
        else
        {
            painter.setBrush(m_visualize ? StateMachineColors::s_selectedColor : bgColor);
        }

        painter.drawRect(m_visualizeRect);
    }

    void StateGraphNode::UpdateTextPixmap()
    {
        m_titleText.setTextOption(m_textOptionsCenter);
        m_titleText.setTextFormat(Qt::PlainText);
        m_titleText.setPerformanceHint(QStaticText::AggressiveCaching);
        m_titleText.setTextWidth(m_rect.width());
        m_titleText.setText(m_elidedName);
        m_titleText.prepare(QTransform(), m_headerFont);
    }
} // namespace EMStudio
