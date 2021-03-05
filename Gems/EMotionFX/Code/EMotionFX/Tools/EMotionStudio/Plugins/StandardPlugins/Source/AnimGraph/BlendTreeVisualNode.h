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
        uint32 GetType() const      { return BlendTreeVisualNode::TYPE_ID; }

        void Render(QPainter& painter, QPen* pen, bool renderShadow) override;

        int32 CalcRequiredHeight() const;

    private:
        QColor GetPortColor(const EMotionFX::AnimGraphNode::Port& port) const;
    };

}   // namespace EMStudio
