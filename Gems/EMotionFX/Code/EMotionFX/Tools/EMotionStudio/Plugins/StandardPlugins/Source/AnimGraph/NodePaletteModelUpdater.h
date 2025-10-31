/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/AnimGraphNode.h>

namespace GraphCanvas
{
    class NodePaletteWidget;
    class NodePaletteTreeItem;
} // namespace GraphCanvas

namespace EMStudio
{
    class AnimGraphPlugin;

    //! NodePaletteModelUpdater
    //!
    //! This class builds a hierarchy of GraphCanvas::NodePaletteTreeItem nodes
    //! that represent nodes that can be added to the anim graph when a given
    //! node is focused. They will be grouped into EMotionFX node categories.
    //!
    //! On initialization and on every change of focused anim graph node you
    //! need to call InitForNode. Without it, you'll get only empty category
    //! nodes.
    //!
    //! Root node returned by GetRootItem() is typically passed in a config
    //! for GraphCanvas::NodePaletteWidget setup. Object of this class should live
    //! no longer than the widget because the widget takes the ownership of the
    //! root node and the nodes in this class will become dangling pointers if the
    //! wigdet is destroyed.
    class NodePaletteModelUpdater
    {
    public:
        explicit NodePaletteModelUpdater(AnimGraphPlugin* plugin);
        GraphCanvas::NodePaletteTreeItem* GetRootItem();

        //! Rebuild the list of available/enabled nodes when a given node is focused
        void InitForNode(EMotionFX::AnimGraphNode* focusNode);

    private:
        AnimGraphPlugin* m_plugin;
        GraphCanvas::NodePaletteTreeItem* m_rootItem;
        AZStd::map<EMotionFX::AnimGraphNode::ECategory, GraphCanvas::NodePaletteTreeItem*> m_categoryNodes;
    };

} // namespace EMStudio
