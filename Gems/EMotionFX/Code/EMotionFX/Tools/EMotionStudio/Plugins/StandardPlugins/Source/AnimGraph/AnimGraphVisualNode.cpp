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

// include required headers
#include "AnimGraphVisualNode.h"
#include "NodeGraph.h"
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphSyncTrack.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include "../../../../EMStudioSDK/Source/MotionEventPresetManager.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "AnimGraphPlugin.h"


namespace EMStudio
{
    // constructor
    AnimGraphVisualNode::AnimGraphVisualNode(const QModelIndex& modelIndex, AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node)
        : GraphNode(modelIndex, node->GetName(), 0, 0)
    {
        mEMFXNode           = node;
        mCanHaveChildren    = node->GetCanHaveChildren();
        mHasVisualGraph     = node->GetHasVisualGraph();
        mPlugin             = plugin;
        SetSubTitle(node->GetPaletteName(), false);
    }


    // destructor
    AnimGraphVisualNode::~AnimGraphVisualNode()
    {
    }


    QColor AnimGraphVisualNode::AzColorToQColor(const AZ::Color& col) const
    {
        return QColor::fromRgbF(col.GetR(), col.GetG(), col.GetB(), col.GetA());
    }


    void AnimGraphVisualNode::Sync()
    {
        SetName(mEMFXNode->GetName());
        SetNodeInfo(mEMFXNode->GetNodeInfo());
        
        SetDeletable(mEMFXNode->GetIsDeletable());
        SetBaseColor(AzColorToQColor(mEMFXNode->GetVisualColor()));
        SetHasChildIndicatorColor(AzColorToQColor(mEMFXNode->GetHasChildIndicatorColor()));
        SetIsCollapsed(mEMFXNode->GetIsCollapsed());

        // Update position
        UpdateRects();
        MoveAbsolute(QPoint(mEMFXNode->GetVisualPosX(), mEMFXNode->GetVisualPosY()));

        SetIsVisualized(mEMFXNode->GetIsVisualizationEnabled());
        SetCanVisualize(mEMFXNode->GetSupportsVisualization());
        SetIsEnabled(mEMFXNode->GetIsEnabled());
        SetVisualizeColor(AzColorToQColor(mEMFXNode->GetVisualizeColor()));
        SetHasVisualOutputPorts(mEMFXNode->GetHasVisualOutputPorts());
        mHasVisualGraph = mEMFXNode->GetHasVisualGraph();

        UpdateTextPixmap();
    }


    // render some debug infos
    void AnimGraphVisualNode::RenderDebugInfo(QPainter& painter)
    {
        MCORE_UNUSED(painter);
        /*  // get the selected actor instance
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance == nullptr)
                return;

            // get the anim graph instance
            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (animGraphInstance == nullptr)
                return;

            QRect rect( mRect.left(), mRect.bottom(), mRect.width(), 10 );

            // draw header text
            QTextOption textOptions;
            textOptions.setAlignment( Qt::AlignCenter|Qt::AlignTop );
            painter.setPen( QColor(0,255,0) );

            QString s;
            //s.sprintf("%.3f  %.3f", mEMFXNode->FindUniqueData(animGraphInstance)->GetInternalPlaySpeed(), mEMFXNode->FindUniqueData(animGraphInstance)->GetPlaySpeed());
            //painter.drawText( rect, s, textOptions );

            //rect.translate(0, 12);
            s.sprintf("%.3f  %.3f", mEMFXNode->FindUniqueData(animGraphInstance)->GetGlobalWeight(), mEMFXNode->FindUniqueData(animGraphInstance)->GetLocalWeight());
            painter.drawText( rect, s, textOptions );*/
    }


    // render the tracks
    void AnimGraphVisualNode::RenderTracks(QPainter& painter, const QColor bgColor, const QColor bgColor2, int32 heightOffset)
    {
        // get the sync track
        QRect rect(mRect.left() + 5, mRect.bottom() - 13 + heightOffset, mRect.width() - 10, 8);

        painter.setPen(bgColor.darker(185));
        painter.setBrush(bgColor2);
        painter.drawRect(rect);

        // get the anim graph instance
        EMotionFX::AnimGraphInstance* animGraphInstance = ExtractAnimGraphInstance();
        if (!animGraphInstance)
        {
            return;
        }

        const float duration = mEMFXNode->GetDuration(animGraphInstance);
        if (duration < MCore::Math::epsilon)
        {
            return;
        }

        // draw the background rect
        QRect playRect = rect;
        int32 x = aznumeric_cast<int32>(rect.left() + 1 + (rect.width() - 2) * (mEMFXNode->GetCurrentPlayTime(animGraphInstance) / duration));
        playRect.setRight(x);
        playRect.setLeft(rect.left() + 1);
        playRect.setTop(rect.top() + 1);
        playRect.setBottom(rect.bottom() - 1);
        painter.setBrush(QColor(255, 255, 255, 32));
        painter.setPen(Qt::NoPen);
        painter.drawRect(playRect);

        // draw the sync keys
        const EMotionFX::AnimGraphNodeData* uniqueData = mEMFXNode->FindOrCreateUniqueNodeData(animGraphInstance);
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
                syncCol = presetManager->GetEventColor(syncTrack->GetEvent(i).GetEventDatas());

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
        x = aznumeric_cast<int32>(rect.left() + 1 + (rect.width() - 2) * (mEMFXNode->GetCurrentPlayTime(animGraphInstance) / duration));
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

        return (animGraphInstance == nullptr) || (animGraphInstance->GetIsOutputReady(mEMFXNode->GetParentNode()->GetObjectIndex()) == false);
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
        EMotionFX::AnimGraphObjectData* uniqueData = mEMFXNode->FindOrCreateUniqueNodeData(animGraphInstance);
        return mEMFXNode->HierarchicalHasError(uniqueData);
    }
}   // namespace EMStudio
