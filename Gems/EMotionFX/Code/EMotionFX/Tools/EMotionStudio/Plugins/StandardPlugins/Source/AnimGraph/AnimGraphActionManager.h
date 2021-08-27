/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/MotionSet.h>
#include <QtCore/QObject>
#include <MCore/Source/StandardHeaders.h>
#endif

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QPersistentModelIndex)

namespace EMotionFX
{
    class AnimGraph;
    class AnimGraphNode;
    class AnimGraphReferenceNode;
    class MotionSet;
}

namespace EMStudio
{
    class AnimGraphPlugin;

    struct AnimGraphActionFilter
    {
        static AnimGraphActionFilter CreateDisallowAll();

        bool m_createNodes = true;
        bool m_editNodes = true;

        bool m_createConnections = true;
        bool m_editConnections = true;

        bool m_copyAndPaste = true;
        bool m_setEntryNode = true;
        bool m_activateState = true;
        bool m_delete = true;
        bool m_editNodeGroups = true;
    };

    class AnimGraphActionManager
        : public QObject
    {
        Q_OBJECT // AUTOMOC

    public:
        AnimGraphActionManager(AnimGraphPlugin* plugin);
        ~AnimGraphActionManager();

        bool GetIsReadyForPaste() const;

        void ShowNodeColorPicker(EMotionFX::AnimGraphNode* animGraphNode);

        enum AlignMode
        {
            Left,
            Right,
            Top,
            Bottom
        };

    signals:
        void PasteStateChanged();

    public slots:
        void Copy();
        void Cut();
        void Paste(const QModelIndex& parentIndex, const QPoint& pos);

        // Sets the first-selected node as an entry state
        void SetEntryState();

        void PreviewMotionSelected(const char* motionId);

        // Adds a wild card transition to the first-selected node
        void AddWildCardTransition();

        void EnableSelected();
        void DisableSelected();

        void MakeVirtualFinalNode();
        void RestoreVirtualFinalNode();

        // Deletes all selected nodes
        void DeleteSelectedNodes();

        void NavigateToNode();
        void NavigateToParent();

        void OpenReferencedAnimGraph(EMotionFX::AnimGraphReferenceNode* referenceNode);

        void ActivateAnimGraph();
        void ActivateGraphForSelectedActors(EMotionFX::AnimGraph* animGraph, EMotionFX::MotionSet* motionSet);

        // Align selected nodes.
        void AlignNodes(AlignMode alignMode);
        void AlignLeft() { AlignNodes(Left); }
        void AlignRight() { AlignNodes(Right); }
        void AlignTop() { AlignNodes(Top); }
        void AlignBottom() { AlignNodes(Bottom); }

    private:
        void SetSelectedEnabled(bool enabled);

        // Copy/Cut and Paste holds some state so the user can change selection
        // while doing the operation. We will store the list of selected items
        // and the type of operation until the users paste.
        // TODO: in a future, we should use something like QClipboard so users can
        // copy/cut/paste through the application and across instances.
        enum class PasteOperation
        {
            None,
            Copy,
            Cut
        };

        AnimGraphPlugin* m_plugin;
        AZStd::vector<QPersistentModelIndex> m_pasteItems;
        PasteOperation m_pasteOperation;

        void SetPasteOperation(PasteOperation newOperation);
    };
} // namespace EMStudio
