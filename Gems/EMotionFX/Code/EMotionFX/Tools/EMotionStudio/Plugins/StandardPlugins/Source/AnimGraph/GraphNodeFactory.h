/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QModelIndex)
QT_FORWARD_DECLARE_CLASS(QWidget)


namespace EMotionFX
{
    class AnimGraphNode;
}

namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;
    class GraphNode;

    /**
     *
     *
     */
    class GraphNodeCreator
    {
    public:
        GraphNodeCreator() {}
        virtual ~GraphNodeCreator() {}

        virtual GraphNode* CreateGraphNode(const QModelIndex& modelIndex, AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node) = 0;
        virtual QWidget* CreateAttributeWidget() { return nullptr; }   // returns nullptr on default, indicating it should auto generate its interface widget
        virtual const AZ::TypeId& GetAnimGraphNodeType() = 0;          // return the TypeId for the EMotionFX::AnimGraphNode that it is linked to
    };

    /**
     *
     *
     */
    class GraphNodeFactory
    {
    public:
        GraphNodeFactory();
        ~GraphNodeFactory();

        void Register(GraphNodeCreator* creator);
        void Unregister(GraphNodeCreator* creator, bool delFromMem = true);
        void UnregisterAll(bool delFromMem = true);

        GraphNode* CreateGraphNode(const QModelIndex& modelIndex, AnimGraphPlugin* plugin, EMotionFX::AnimGraphNode* node);
        QWidget* CreateAttributeWidget(const AZ::TypeId& animGraphNodeType);

        GraphNodeCreator* FindCreator(const AZ::TypeId& animGraphNodeType) const;

    private:
        // Using a vector since we expect this to be small
        AZStd::vector<GraphNodeCreator*> m_creators;
    };

}   // namespace EMStudio
