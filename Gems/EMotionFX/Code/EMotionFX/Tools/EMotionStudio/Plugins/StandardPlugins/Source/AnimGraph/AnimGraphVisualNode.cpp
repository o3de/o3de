/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphVisualNode.h"

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MotionEventPresetManager.h>
#include "AnimGraphPlugin.h"
#include "NodeGraph.h"

#include <AzQtComponents/Utilities/Conversions.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphSyncTrack.h>

namespace EMStudio
{
    // constructor
    AnimGraphVisualNode::AnimGraphVisualNode(const QModelIndex& modelIndex, AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node)
        : GraphNode(modelIndex, node->GetName(), 0, 0)
    {
        m_emfxNode           = node;
        m_canHaveChildren    = node->GetCanHaveChildren();
        m_hasVisualGraph     = node->GetHasVisualGraph();
        m_plugin             = plugin;
        SetSubTitle(node->GetPaletteName(), false);
    }


    // destructor
    AnimGraphVisualNode::~AnimGraphVisualNode()
    {
    }

    void AnimGraphVisualNode::Sync()
    {
        SetName(m_emfxNode->GetName());
        SetNodeInfo(m_emfxNode->GetNodeInfo());
        
        SetDeletable(m_emfxNode->GetIsDeletable());
        SetBaseColor(AzQtComponents::toQColor(m_emfxNode->GetVisualColor()));
        SetHasChildIndicatorColor(AzQtComponents::toQColor(m_emfxNode->GetHasChildIndicatorColor()));
        SetIsCollapsed(m_emfxNode->GetIsCollapsed());

        // Update position
        UpdateRects();
        MoveAbsolute(QPoint(m_emfxNode->GetVisualPosX(), m_emfxNode->GetVisualPosY()));

        SetIsVisualized(m_emfxNode->GetIsVisualizationEnabled());
        SetCanVisualize(m_emfxNode->GetSupportsVisualization());
        SetIsEnabled(m_emfxNode->GetIsEnabled());
        SetVisualizeColor(AzQtComponents::toQColor(m_emfxNode->GetVisualizeColor()));
        SetHasVisualOutputPorts(m_emfxNode->GetHasVisualOutputPorts());
        m_hasVisualGraph = m_emfxNode->GetHasVisualGraph();

        UpdateTextPixmap();
    }


    // render some debug infos
    void AnimGraphVisualNode::RenderDebugInfo(QPainter& painter)
    {
        MCORE_UNUSED(painter);
    }


    // render the tracks
    void AnimGraphVisualNode::RenderTracks(QPainter& painter, const QColor bgColor, const QColor bgColor2, int32 heightOffset)
    {
        // get the sync track
        QRect rect(m_rect.left() + 5, m_rect.bottom() - 13 + heightOffset, m_rect.width() - 10, 8);

        painter.setPen(bgColor.darker(185));
        painter.setBrush(bgColor2);
        painter.drawRect(rect);

        // get the anim graph instance
        EMotionFX::AnimGraphInstance* animGraphInstance = ExtractAnimGraphInstance();
        if (!animGraphInstance)
        {
            return;
        }

        const float duration = m_emfxNode->GetDuration(animGraphInstance);
        if (duration < MCore::Math::epsilon)
        {
            return;
        }

        // draw the background rect
        QRect playRect = rect;
        int32 x = aznumeric_cast<int32>(rect.left() + 1 + (rect.width() - 2) * (m_emfxNode->GetCurrentPlayTime(animGraphInstance) / duration));
        playRect.setRight(x);
        playRect.setLeft(rect.left() + 1);
        playRect.setTop(rect.top() + 1);
        playRect.setBottom(rect.bottom() - 1);
        painter.setBrush(QColor(255, 255, 255, 32));
        painter.setPen(Qt::NoPen);
        painter.drawRect(playRect);

        // draw the sync keys
        const EMotionFX::AnimGraphNodeData* uniqueData = m_emfxNode->FindOrCreateUniqueNodeData(animGraphInstance);
        const EMotionFX::AnimGraphSyncTrack* syncTrack = uniqueData->GetSyncTrack();

        const size_t numSyncPoints = syncTrack ? syncTrack->GetNumEvents() : 0;
        if (numSyncPoints > 0)
        {
            //painter.setPen( syncPointColor );
            //painter.setBrush( syncPointColor );
            painter.setClipRect(rect);
            painter.setClipping(true);

            // get the preset manager
            const EMStudio::MotionEventPresetManager* presetManager = EMStudio::GetManager()->GetEventPresetManger();

            QPoint points[3];
            QColor syncCol;
            for (size_t i = 0; i < numSyncPoints; ++i)
            {
                // get the event color from the preset manager
                syncCol = AzQtComponents::toQColor(presetManager->GetEventColor(syncTrack->GetEvent(i).GetEventDatas()));

                painter.setPen(syncCol);
                painter.setBrush(syncCol);

                x = aznumeric_cast<int32>(rect.left() + 1 + (rect.width() - 2) * (syncTrack->GetEvent(i).GetStartTime() / duration));
                points[0] = QPoint(x, rect.top() + 1);
                points[1] = QPoint(x + 2, rect.bottom() - 1);
                points[2] = QPoint(x - 2, rect.bottom() - 1);
                painter.drawPolygon(points, 3, Qt::WindingFill);
            }
            painter.setClipping(false);
        }

        // draw the current play time
        painter.setPen(Qt::yellow);
        x = aznumeric_cast<int32>(rect.left() + 1 + (rect.width() - 2) * (m_emfxNode->GetCurrentPlayTime(animGraphInstance) / duration));
        painter.drawLine(x, rect.top() + 1, x, rect.bottom());
    }


    // extract the single selected anim graph instance
    EMotionFX::AnimGraphInstance* AnimGraphVisualNode::ExtractAnimGraphInstance() const
    {
        return m_modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
    }


    // check if we always want to render this node colored
    bool AnimGraphVisualNode::GetAlwaysColor() const
    {
        // extract anim graph instance
        EMotionFX::AnimGraphInstance* animGraphInstance = ExtractAnimGraphInstance();

        return (animGraphInstance == nullptr) || (animGraphInstance->GetIsOutputReady(m_emfxNode->GetParentNode()->GetObjectIndex()) == false);
    }


    // check if the emfx node for the given visual node has an error
    bool AnimGraphVisualNode::GetHasError() const
    {
        // extract anim graph instance
        EMotionFX::AnimGraphInstance* animGraphInstance = ExtractAnimGraphInstance();
        if (animGraphInstance == nullptr)
        {
            return false;
        }

        // return the error state of the emfx node
        EMotionFX::AnimGraphObjectData* uniqueData = m_emfxNode->FindOrCreateUniqueNodeData(animGraphInstance);
        return m_emfxNode->HierarchicalHasError(uniqueData);
    }
} // namespace EMStudio
