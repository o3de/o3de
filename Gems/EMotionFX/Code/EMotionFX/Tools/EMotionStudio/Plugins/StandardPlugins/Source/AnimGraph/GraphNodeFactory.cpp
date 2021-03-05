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

#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendTreeVisualNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNodeFactory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/StateGraphNode.h>


namespace EMStudio
{
    GraphNodeFactory::GraphNodeFactory()
    {
        m_creators.reserve(20);
    }

    GraphNodeFactory::~GraphNodeFactory()
    {
        UnregisterAll(true); // true = delete objects from memory as well
    }

    void GraphNodeFactory::Register(GraphNodeCreator* creator)
    {
        AZ_Assert(creator, "Expected non null GraphNodeCreator");

        // check if we already registered this one before
        AZ_Assert(!FindCreator(creator->GetAnimGraphNodeType()), "GraphNodeFactory::Register() - There has already been a creator registered for the given node type %d.", creator->GetAnimGraphNodeType());

        // register it
        m_creators.push_back(creator);
    }

    void GraphNodeFactory::Unregister(GraphNodeCreator* creator, bool delFromMem)
    {
        m_creators.erase(AZStd::remove(m_creators.begin(), m_creators.end(), creator), m_creators.end());
        if (delFromMem)
        {
            delete creator;
        }
    }

    void GraphNodeFactory::UnregisterAll(bool delFromMem)
    {
        // if we want to delete the creators from memory as well
        if (delFromMem)
        {
            for (GraphNodeCreator* creator : m_creators)
            {
                delete creator;
            }
        }

        // clear the array
        m_creators.clear();
    }

    GraphNode* GraphNodeFactory::CreateGraphNode(const QModelIndex& modelIndex, AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node)
    {
        EMotionFX::AnimGraphNode* parent = node->GetParentNode();
        if (parent)
        {
            if (azrtti_typeid(parent) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                return new StateGraphNode(modelIndex, plugin, node);
            }
            else
            {
                GraphNodeCreator* creator = FindCreator(azrtti_typeid(node));
                if (!creator)
                {
                    return new BlendTreeVisualNode(modelIndex, plugin, node);
                }
                return creator->CreateGraphNode(modelIndex, plugin, node);
            }
        }

        // try to locate the creator
        GraphNodeCreator* creator = FindCreator(azrtti_typeid(node));
        if (creator)
        {
            return creator->CreateGraphNode(modelIndex, plugin, node);
        }

        return new GraphNode(modelIndex, node->GetName());
    }

    QWidget* GraphNodeFactory::CreateAttributeWidget(const AZ::TypeId& animGraphNodeType)
    {
        // try to locate the creator
        GraphNodeCreator* creator = FindCreator(animGraphNodeType);
        if (creator)
        {
            return creator->CreateAttributeWidget();
        }

        return nullptr;
    }

    // search for the right creator
    GraphNodeCreator* GraphNodeFactory::FindCreator(const AZ::TypeId& animGraphNodeType) const
    {
        for (GraphNodeCreator* creator : m_creators)
        {
            if (creator->GetAnimGraphNodeType() == animGraphNodeType)
            {
                return creator;
            }
        }
        return nullptr;
    }

} // namespace EMStudio
