/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AnimGraphNodeWidget.h"
#include <EMotionFX/Source/ActorInstanceBus.h>
#include <EMotionFX/Source/BlendSpaceNode.h>
#include <EMotionFX/Source/EventHandler.h>
#include <AzCore/Math/Vector2.h>
#include <QMetaObject>
#include <QPainter>

QT_FORWARD_DECLARE_CLASS(QToolButton)

namespace EMotionFX
{
    class AnimGraphObjectData;
    class MotionSet;
}

namespace EMStudio
{
    class BlendSpaceNodeWidget
        : private EMotionFX::EventHandler
        , private EMotionFX::ActorInstanceNotificationBus::Handler
    {
    public:
        BlendSpaceNodeWidget();
        ~BlendSpaceNodeWidget();

        void RenderCurrentSamplePoint(QPainter& painter, const QPointF& samplePoint);
        void RenderSampledMotionPoint(QPainter& painter, const QPointF& point, float weight);
        void RenderTextBox(QPainter& painter, const QPointF& point, const QString& text);

    protected:
        void InitializePreviewControls(QWidget* widget);
        void ResetPreview();
        void StopPreviewPlayback();
        void SetPreviewPosition(const AZ::Vector2& position);
        EMotionFX::AnimGraphObjectData* GetBlendSpaceData(
            EMotionFX::BlendSpaceNode* node, const QPersistentModelIndex& modelIndex);
        //! Why GetBlendSpaceData() last returned nullptr, for display in the view.
        const char* GetPreviewStatus() const { return m_previewStatus; }

        // Grid editing. Point indices are indices into the unique data, which skips motions missing from the motion set.
        AZ::u32 GetMotionIndexForPoint(AZ::u32 pointIndex) const;
        bool BeginMotionDrag(EMotionFX::BlendSpaceNode* node, AZ::u32 pointIndex);
        void UpdateMotionDrag(const AZ::Vector2& coordinates);
        void EndMotionDrag();
        void CancelMotionDrag();
        bool IsDraggingMotion() const { return m_dragNode != nullptr; }
        void ShowGridContextMenu(EMotionFX::BlendSpaceNode* node, const QPoint& globalPos, AZ::u32 pointIndex, const AZ::Vector2& coordinates);
        void RemoveMotionAtPoint(EMotionFX::BlendSpaceNode* node, AZ::u32 pointIndex);

        AZ::u32 m_selectedPointIndex = MCORE_INVALIDINDEX32;

        // Current sample point
        static const float s_currentSamplePointWidth;
        static const QColor s_currentSamplePointColor;

        // Sampled motion points
        static const float s_motionPointWidthMin;
        static const float s_motionPointWidthMax;
        static const float s_motionPointAlphaMin;
        static const float s_motionPointAlphaMax;
        static const QColor s_motionPointColor;

        static const char* s_warningOffsetForIcon;

    private:
        void RenderCircle(QPainter& painter, const QPointF& point, const QColor& color, float size);
        void SetPreviewPlayback(bool enabled);
        const AZStd::vector<EMotionFX::EventTypes> GetHandledEventTypes() const override;
        void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;
        void OnDeleteAnimGraphInstance(EMotionFX::AnimGraphInstance* instance) override;
        void OnDeleteAnimGraph(EMotionFX::AnimGraph* graph) override;
        void OnDeleteMotionSet(EMotionFX::MotionSet* motionSet) override;
        void OnDeleteMotion(EMotionFX::Motion* motion) override;
        void OnRemoveNode(EMotionFX::AnimGraph* graph, EMotionFX::AnimGraphNode* node) override;

        using BlendSpaceMotions = AZStd::vector<EMotionFX::BlendSpaceNode::BlendSpaceMotion>;
        void AddMotionAt(EMotionFX::BlendSpaceNode* node, const AZ::Vector2& coordinates);
        //! Show the motions on the grid and in the preview without recording an undo step.
        void ApplyMotionsLive(EMotionFX::BlendSpaceNode* node, const BlendSpaceMotions& motions);
        //! Replace the node's motions through an undoable AnimGraphAdjustNode command.
        void CommitMotions(EMotionFX::BlendSpaceNode* node, const BlendSpaceMotions& before, const BlendSpaceMotions& after);

        class StopPreviewCommandCallback;
        StopPreviewCommandCallback* m_stopPreviewCallback = nullptr;

        QWidget* m_previewWidget = nullptr;
        QToolButton* m_previewButton = nullptr;
        QMetaObject::Connection m_modelConnection;
        const QAbstractItemModel* m_previewModel = nullptr;
        QPersistentModelIndex m_previewModelIndex;
        EMotionFX::BlendSpaceNode* m_previewSource = nullptr;
        EMotionFX::BlendSpaceNode* m_previewNode = nullptr;
        EMotionFX::AnimGraph* m_previewGraph = nullptr;
        EMotionFX::AnimGraphInstance* m_previewInstance = nullptr;
        EMotionFX::AnimGraphInstance* m_previousInstance = nullptr;
        const char* m_previewStatus = "Load an actor in the Animation Editor to preview this blend space.";
        bool m_previewDirty = false;
        bool m_previewPlaying = false;
        AZ::Vector2 m_previewPosition = AZ::Vector2::CreateZero();

        EMotionFX::BlendSpaceNode* m_dataNode = nullptr; // node whose unique data GetBlendSpaceData() returned
        EMotionFX::MotionSet* m_dataMotionSet = nullptr;
        EMotionFX::BlendSpaceNode* m_dragNode = nullptr;
        AZ::u32 m_dragMotionIndex = 0;
        BlendSpaceMotions m_dragStartMotions;
        bool m_dragMoved = false;
    };
} // namespace EMStudio
