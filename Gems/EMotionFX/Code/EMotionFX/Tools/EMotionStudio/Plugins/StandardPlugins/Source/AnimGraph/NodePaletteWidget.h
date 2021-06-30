/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QTreeView)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)


namespace EMStudio
{
    class AnimGraphPlugin;
    class NodePaletteModel;


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
            NodePaletteWidget*  mWidget;
        };

        NodePaletteWidget(AnimGraphPlugin* plugin);
        ~NodePaletteWidget();

        void Init(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node);

        static QIcon GetNodeIcon(const EMotionFX::AnimGraphNode* node);

    private slots:
        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);

    private:
        void SaveExpandStates();
        void RestoreExpandStates();

        AnimGraphPlugin*            mPlugin;
        NodePaletteModel*           mModel;
        QTreeView*                  mTreeView;
        EMotionFX::AnimGraphNode*   mNode;
        EventHandler*               mEventHandler;
        QVBoxLayout*                mLayout;
        QLabel*                     mInitialText;

        // Cache the expanded states.
        AZStd::unordered_set<EMotionFX::AnimGraphObject::ECategory> m_expandedCatagory;
    };
}   // namespace EMStudio
