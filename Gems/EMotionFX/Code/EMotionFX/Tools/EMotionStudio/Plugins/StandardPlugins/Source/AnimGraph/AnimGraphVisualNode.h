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

        MCORE_INLINE void SetEMFXNode(EMotionFX::AnimGraphNode* emfxNode)  { mEMFXNode = emfxNode; }
        MCORE_INLINE EMotionFX::AnimGraphNode* GetEMFXNode()               { return mEMFXNode; }
        MCORE_INLINE AnimGraphPlugin* GetAnimGraphPlugin() const          { return mPlugin; }
        EMotionFX::AnimGraphInstance* ExtractAnimGraphInstance() const;

        void RenderTracks(QPainter& painter, const QColor bgColor, const QColor bgColor2, int32 heightOffset = 0);
        void RenderDebugInfo(QPainter& painter);

        bool GetAlwaysColor() const override;
        bool GetHasError() const override;

    protected:
        QColor AzColorToQColor(const AZ::Color& col) const;
    
        EMotionFX::AnimGraphNode*  mEMFXNode;
        EMotionFX::AnimGraphPose   mPose;
        AnimGraphPlugin*           mPlugin;
    };
}   // namespace EMStudio
