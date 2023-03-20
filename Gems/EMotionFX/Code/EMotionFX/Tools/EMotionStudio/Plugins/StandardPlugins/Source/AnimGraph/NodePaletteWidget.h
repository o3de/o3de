/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/EventHandler.h>
#include "../StandardPluginsConfig.h"
#include "NodePaletteModelUpdater.h"
#include <QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QTreeView)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)


namespace GraphCanvas
{
    class NodePaletteWidget;
}

namespace EMStudio
{
    class AnimGraphPlugin;

    class NodePaletteWidget
        : public QWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(NodePaletteWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT // AUTOMOC

    public:
        class EventHandler
            : public EMotionFX::EventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            EventHandler(NodePaletteWidget* widget);
            ~EventHandler() override = default;

            const AZStd::vector<EMotionFX::EventTypes> GetHandledEventTypes() const override { return { EMotionFX::EVENT_TYPE_ON_CREATED_NODE, EMotionFX::EVENT_TYPE_ON_REMOVED_CHILD_NODE }; }
            void OnCreatedNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node) override;
            void OnRemovedChildNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* parentNode) override;

        private:
            NodePaletteWidget*  m_widget;
        };

        NodePaletteWidget(AnimGraphPlugin* plugin);
        ~NodePaletteWidget();

        void Init(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node);

    private slots:
        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);

    private:

        AnimGraphPlugin*            m_plugin;
        NodePaletteModelUpdater      m_modelUpdater;
        EMotionFX::AnimGraphNode*   m_node;
        EventHandler*               m_eventHandler;
        QVBoxLayout*                m_layout;
        QLabel*                     m_initialText;
        GraphCanvas::NodePaletteWidget* m_palette;

        // Cache the expanded states.
        AZStd::unordered_set<EMotionFX::AnimGraphObject::ECategory> m_expandedCatagory;
    };
}   // namespace EMStudio
