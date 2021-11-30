/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include "GraphNode.h"
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/AnimGraphInstance.h>

EMFX_FORWARD_DECLARE(AnimGraphNode)

namespace EMStudio
{
    class AnimGraphPlugin;


    // the blend graph node
    class AnimGraphVisualNode
        : public GraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphVisualNode, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        AnimGraphVisualNode(const QModelIndex& modelIndex, AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node);
        virtual ~AnimGraphVisualNode();

        void Sync() override;

        MCORE_INLINE void SetEMFXNode(EMotionFX::AnimGraphNode* emfxNode)  { m_emfxNode = emfxNode; }
        MCORE_INLINE EMotionFX::AnimGraphNode* GetEMFXNode()               { return m_emfxNode; }
        MCORE_INLINE AnimGraphPlugin* GetAnimGraphPlugin() const          { return m_plugin; }
        EMotionFX::AnimGraphInstance* ExtractAnimGraphInstance() const;

        void RenderTracks(QPainter& painter, const QColor bgColor, const QColor bgColor2, int32 heightOffset = 0);
        void RenderDebugInfo(QPainter& painter);

        bool GetAlwaysColor() const override;
        bool GetHasError() const override;

    protected:
        QColor AzColorToQColor(const AZ::Color& col) const;
    
        EMotionFX::AnimGraphNode*  m_emfxNode;
        EMotionFX::AnimGraphPose   m_pose;
        AnimGraphPlugin*           m_plugin;
    };
}   // namespace EMStudio
