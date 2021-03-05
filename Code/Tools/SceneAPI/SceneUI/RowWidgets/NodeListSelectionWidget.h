#pragma once

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

#if !defined(Q_MOC_RUN)
#include <QComboBox>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#endif

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IGraphObject;
        }

        namespace UI
        {
            class NodeListSelectionWidget : public QComboBox
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                explicit NodeListSelectionWidget(QWidget* parent);

                void SetCurrentSelection(const AZStd::string& selection);
                AZStd::string GetCurrentSelection() const;

                void AddDisabledOption(const AZStd::string& option);
                void AddDisabledOption(AZStd::string&& option);
                const AZStd::string& GetDisabledOption() const;
                void UseShortNames(bool use);
                // Sets the class type id to filter against.
                void SetClassTypeId(const Uuid& classTypeId);
                void ClearClassTypeId();
                // When the classTypeId is set as this is true, the nodes it the tree
                //      must have the exact same type id, if set to false all types
                //      matching the type id and derived classes will be listed.
                void UseExactClassTypeMatch(bool exactMatch);
                void ExcludeEndPoints(bool exclude);
                // If the assigned selection is missing the selection will default to
                //      the disabled value if present and true, otherwise the alphabetically
                //      first entry is used.
                void DefaultToDisabled(bool value);
            
            signals:
                void valueChanged(const AZStd::string& newValue);
            
            protected slots:
                void OnTextChange(const QString& text);

            protected:
                using EntrySet = AZStd::unordered_set<AZStd::string>;

                void showEvent(QShowEvent* event) override;

                void BuildList(const Containers::SceneGraph& graph);
                bool IsCorrectType(const DataTypes::IGraphObject& object) const;
                void AddEntry(EntrySet& entries, const Containers::SceneGraph::Name& name);
                void SetSelection();
                void AddDisabledOption();

                AZStd::string m_disabledOption;
                AZStd::string m_currentSelection;
                Uuid m_classTypeId;
                // Set to true if only a specific class type should be in the filter, otherwise
                //      all classes that derive from the given type will be listed.
                bool m_exactClassTypeMatch;
                // Attributes come in after widget has been created and this requires the
                //      list to be rebuild. This flag keeps track of any changes and
                //      whether or not the list should be repopulated.
                bool m_hasDirtyList;
                bool m_useShortNames;
                bool m_excludeEndPoints;
                bool m_defaultToDisabled;
            };
        } // UI
    } // SceneAPI
} // AZ
