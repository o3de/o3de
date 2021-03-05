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

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/NodeHierarchyWidget.h>

namespace EMotionFX
{
    class ActorInstance;
}

namespace EMStudio
{
    class NodeSelectionWindow;

    class ActorJointBrowseEdit
        : public AzQtComponents::BrowseEdit
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        explicit ActorJointBrowseEdit(QWidget* parent = nullptr);

        void SetSingleJointSelection(bool singleSelectionEnabled);
        void SetSelectedJoints(const AZStd::vector<SelectionItem>& selectedJoints);

        EMotionFX::ActorInstance* GetActorInstance() const;
        const AZStd::vector<SelectionItem>& GetPreviouslySelectedJoints() const { return m_previouslySelectedJoints; }

    signals:
        // Emitted when the new joint selection got accepted from the joint selection window.
        void SelectionDone(const AZStd::vector<SelectionItem>& selectedJoints);

        // Selection got rejected. In case reaction happened on the SelectionChanged() signal, handle going back to the original state.
        void SelectionRejected(const AZStd::vector<SelectionItem>& previouslySelectedJoints);

        // Emitted while the selection window is open and selection changes (final selection via SelectionDone() signal).
        void SelectionChanged(const AZStd::vector<SelectionItem>& selectedJoints);

    private slots:
        void OnBrowseButtonClicked();
        void OnSelectionDone(const AZStd::vector<SelectionItem>& selectedJoints);
        void OnSelectionDoneMCoreArray(const MCore::Array<SelectionItem>& selectedJoints);
        void OnSelectionChanged();
        void OnSelectionRejected();
        void OnTextEdited(const QString& text);

    private:
        void UpdatePlaceholderText();

        AZStd::vector<SelectionItem> FromMCoreArray(const MCore::Array<SelectionItem>& in) const;

        AZStd::vector<SelectionItem> m_previouslySelectedJoints; /// Selected joints before selection window opened.
        AZStd::vector<SelectionItem> m_selectedJoints;
        NodeSelectionWindow* m_jointSelectionWindow = nullptr;
        bool m_singleJointSelection = true;
    };
} // namespace EMStudio
