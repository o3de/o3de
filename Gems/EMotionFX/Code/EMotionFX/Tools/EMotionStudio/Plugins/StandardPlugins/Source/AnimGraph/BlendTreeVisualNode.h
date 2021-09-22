/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphVisualNode.h>


namespace EMStudio
{
    class AnimGraphPlugin;


    // the blend graph node
    class BlendTreeVisualNode
        : public AnimGraphVisualNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeVisualNode, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        enum
        {
            TYPE_ID = 0x00000002
        };

        BlendTreeVisualNode(const QModelIndex& modelIndex, AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node);
        ~BlendTreeVisualNode();

        void Sync() override;
        uint32 GetType() const override { return BlendTreeVisualNode::TYPE_ID; }

        void Render(QPainter& painter, QPen* pen, bool renderShadow) override;

        int32 CalcRequiredHeight() const override;

    private:
        QColor GetPortColor(const EMotionFX::AnimGraphNode::Port& port) const;
    };

}   // namespace EMStudio
