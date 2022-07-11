/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/DOM/Backends/JSON/JsonBackend.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyHandlerWidget.h>

#include <QHBoxLayout>
#include <QScrollArea>

#endif // Q_MOC_RUN

class QCheckBox;
class QTimer;

namespace AzToolsFramework
{
    class DocumentPropertyEditor;

    class DPELayout : public QHBoxLayout
    {
        Q_OBJECT

    signals:
        void expanderChanged(bool expanded);

        // todo: look into caching and QLayoutItem::invalidate()
    public:
        DPELayout(int depth, QWidget* parentWidget = nullptr);
        virtual ~DPELayout();
        void SetExpanderShown(bool shouldShow);
        void SetExpanded(bool expanded);
        bool IsExpanded() const;

        // QLayout overrides
        QSize sizeHint() const override;
        QSize minimumSize() const override;
        void setGeometry(const QRect& rect) override;
        Qt::Orientations expandingDirections() const override;

    protected slots:
        void onCheckstateChanged(int expanderState);

    protected:
        DocumentPropertyEditor* GetDPE() const;
        void CreateExpanderWidget();

        int m_depth = 0; //!< number of levels deep in the tree. Used for indentation
        bool m_showExpander = false;
        bool m_expanded = true;
        QCheckBox* m_expanderWidget = nullptr;
    };
    class DPERowWidget : public QWidget
    {
        Q_OBJECT
        friend class DocumentPropertyEditor;

    public:
        explicit DPERowWidget(int depth, DPERowWidget* parentRow);
        ~DPERowWidget();

        void Clear(); //!< destroy all layout contents and clear DOM children
        void AddChildFromDomValue(const AZ::Dom::Value& childValue, int domIndex);

        //! clears and repopulates all children from a given DOM array
        void SetValueFromDom(const AZ::Dom::Value& domArray);

        //! handles a patch operation at the given path, or delegates to a child that will
        void HandleOperationAtPath(const AZ::Dom::PatchOperation& domOperation, size_t pathIndex = 0);

        //! returns the last descendent of this row in its own layout
        DPERowWidget* GetLastDescendantInLayout();

        bool IsExpanded() const;

    protected slots:
        void onExpanderChanged(int expanderState);

    protected:
        DocumentPropertyEditor* GetDPE() const;
        void AddDomChildWidget(int domIndex, QWidget* childWidget);

        DPERowWidget* m_parentRow = nullptr;
        int m_depth = 0; //!< number of levels deep in the tree. Used for indentation
        DPELayout* m_columnLayout = nullptr;

        //! widget children in DOM specified order; mix of row and column widgets
        AZStd::deque<QWidget*> m_domOrderedChildren;

        // a map from the propertyHandler widgets to the propertyHandlers that created them
        AZStd::unordered_map<QWidget*, AZStd::unique_ptr<PropertyHandlerWidgetInterface>> m_widgetToPropertyHandler;
    };

    class DocumentPropertyEditor : public QScrollArea
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(DocumentPropertyEditor, AZ::SystemAllocator, 0);

        explicit DocumentPropertyEditor(QWidget* parentWidget = nullptr);
        ~DocumentPropertyEditor();

        auto GetAdapter()
        {
            return m_adapter;
        }
        void AddAfterWidget(QWidget* precursor, QWidget* widgetToAdd);

        enum class ExpanderState : uint8_t
        {
            NotSet,
            Collapsed,
            Expanded
        };

        void SetSavedExpanderStateForRow(DPERowWidget* row, ExpanderState expanderState);
        ExpanderState GetSavedExpanderStateForRow(DPERowWidget* row) const;
        void RemoveExpanderStateForRow(DPERowWidget* row);
        AZ::Dom::Value GetDomValueForRow(DPERowWidget* row) const;

        void ReleaseHandler(AZStd::unique_ptr<PropertyHandlerWidgetInterface>&& handler);

        // sets whether this DPE should also spawn a DPEDebugWindow when its adapter
        // is set. Initially, this takes its value from the CVAR ed_showDPEDebugView,
        // but can be overridden here
        void SetSpawnDebugView(bool shouldSpawn);

    public slots:
        //! set the DOM adapter for this DPE to inspect
        void SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapterPtr theAdapter);

    protected:
        QVBoxLayout* GetVerticalLayout();
        void AddRowFromValue(const AZ::Dom::Value& domValue, int rowIndex);
        AZStd::vector<size_t> GetPathToRoot(DPERowWidget* row) const;

        void HandleReset();
        void HandleDomChange(const AZ::Dom::Patch& patch);
        void CleanupReleasedHandlers();

        AZ::DocumentPropertyEditor::DocumentAdapterPtr m_adapter;
        AZ::DocumentPropertyEditor::DocumentAdapter::ResetEvent::Handler m_resetHandler;
        AZ::DocumentPropertyEditor::DocumentAdapter::ChangedEvent::Handler m_changedHandler;
        QVBoxLayout* m_layout = nullptr;

        bool m_spawnDebugView = false;

        QTimer* m_handlerCleanupTimer;
        AZStd::vector<AZStd::unique_ptr<PropertyHandlerWidgetInterface>> m_unusedHandlers;
        AZStd::deque<DPERowWidget*> m_domOrderedRows;

        //! tree nodes to keep track of expander state explicitly changed by the user
        struct ExpanderPathNode
        {
            ExpanderState expanderState = ExpanderState::NotSet;
            AZStd::unordered_map<size_t, ExpanderPathNode> nextNode;
        };

        //! hierarchical dom index to expander state tree
        AZStd::unordered_map<size_t, ExpanderPathNode> m_expanderPaths;
    };
} // namespace AzToolsFramework
