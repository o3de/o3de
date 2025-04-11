/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NodePaletteModelUpdater.h"
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

namespace EMStudio
{
    NodePaletteModelUpdater::NodePaletteModelUpdater(AnimGraphPlugin* plugin)
        : m_rootItem(aznew GraphCanvas::NodePaletteTreeItem("root", AnimGraphEditorId))
        , m_plugin(plugin)

    {
        static const EMotionFX::AnimGraphNode::ECategory categories[] = {
            EMotionFX::AnimGraphNode::CATEGORY_SOURCES,     EMotionFX::AnimGraphNode::CATEGORY_BLENDING,
            EMotionFX::AnimGraphNode::CATEGORY_CONTROLLERS, EMotionFX::AnimGraphNode::CATEGORY_PHYSICS,
            EMotionFX::AnimGraphNode::CATEGORY_LOGIC,       EMotionFX::AnimGraphNode::CATEGORY_MATH,
            EMotionFX::AnimGraphNode::CATEGORY_MISC
        };

        for (EMotionFX::AnimGraphNode::ECategory category : categories)
        {
            const char* categoryName = EMotionFX::AnimGraphObject::GetCategoryName(category);
            auto* node = m_rootItem->CreateChildNode<GraphCanvas::NodePaletteTreeItem>(categoryName, AnimGraphEditorId);
            m_categoryNodes.insert({ category, node });
        }
    }

    GraphCanvas::NodePaletteTreeItem* NodePaletteModelUpdater::GetRootItem()
    {
        return m_rootItem;
    }

    void NodePaletteModelUpdater::InitForNode(EMotionFX::AnimGraphNode* focusNode)
    {
        for (auto& [category, categoryNode] : m_categoryNodes)
        {
            categoryNode->ClearChildren();
            categoryNode->SetEnabled(false);
        }

        const AZStd::vector<EMotionFX::AnimGraphObject*>& objectPrototypes = m_plugin->GetAnimGraphObjectFactory()->GetUiObjectPrototypes();
        for (const EMotionFX::AnimGraphObject* objectPrototype : objectPrototypes)
        {
            const EMotionFX::AnimGraphObject::ECategory objectCategory = objectPrototype->GetPaletteCategory();
            const auto categoryNodeIt = m_categoryNodes.find(objectCategory);
            if (categoryNodeIt != m_categoryNodes.end())
            {
                const EMotionFX::AnimGraphNode* nodePrototype = static_cast<const EMotionFX::AnimGraphNode*>(objectPrototype);
                const auto typeString = azrtti_typeid(nodePrototype).ToFixedString();
                auto* node = categoryNodeIt->second->CreateChildNode<EMStudio::BlendGraphNodePaletteTreeItem>(
                    nodePrototype->GetPaletteName(), QString::fromUtf8(typeString.c_str(), static_cast<int>(typeString.size())),
                    AnimGraphEditorId, nodePrototype->GetVisualColor());

                const bool nodeEnabled = focusNode && m_plugin->CheckIfCanCreateObject(focusNode, objectPrototype, categoryNodeIt->first);
                node->SetEnabled(nodeEnabled);

                if (nodeEnabled)
                {
                    categoryNodeIt->second->SetEnabled(true);
                }
            }
        }
    }

} // namespace EMStudio
