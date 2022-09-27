/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/IPropertyEditor.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyHandlerWidget.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditorSettings.h>

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
        void SharePriorColumn(QWidget* previousWidget);
        void SetSharePrior(bool sharePrior);
        bool ShouldSharePrior();
        int SharedWidgetCount();
        void WidgetAlignment(QWidget* alignedWidget, Qt::Alignment widgetAlignment);
        void AddMinimumWidthWidget(QWidget* widget);


        // QLayout overrides
        void invalidate() override;
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

        //! boolean to keep track of whether we should add to an existing shared column or create a new shared column.
        bool m_shouldSharePrior = false;

        //! Vector containing pairs of widgets and integers, where each pair in the vector represents a unique shared column layout.
        //! Each widget in a pair will be the first widget in the shared column,
        //! while the integer in a pair represents the number of widgets in the shared column. 
        AZStd::vector<AZStd::pair<QWidget*, int>> m_sharePriorColumn;

        //! Map containing all widgets that have special alignment.
        //! Each widget will be aligned according to its Qt::Alignment value.
        AZStd::unordered_map<QWidget*, Qt::Alignment> m_widgetAlignment;

        //! Vector containing all widgets that have the UseMinimumWidth attribute.
        //! Dependent attribute that forces widgets to use their minimum width within a shared column.
        AZStd::unordered_set<QWidget*> m_minimumWidthWidgets;

    private:
        // These cached sizes must be mutable since they are set inside of an overidden const function
        mutable QSize m_cachedLayoutSize;
        mutable QSize m_cachedMinLayoutSize;
    };

    class DPERowWidget : public QFrame
    {
        Q_OBJECT
        Q_PROPERTY(bool hasChildRows READ HasChildRows);
        Q_PROPERTY(int getLevel READ GetLevel);

        friend class DocumentPropertyEditor;

    public:
        explicit DPERowWidget(int depth, DPERowWidget* parentRow);
        ~DPERowWidget();

        void Clear(); //!< destroy all layout contents and clear DOM children
        void AddChildFromDomValue(const AZ::Dom::Value& childValue, size_t domIndex);

        //! clears and repopulates all children from a given DOM array
        void SetValueFromDom(const AZ::Dom::Value& domArray);
        void SetAttributesFromDom(const AZ::Dom::Value& domArray);

        //! handles a patch operation at the given path, or delegates to a child that will
        void HandleOperationAtPath(const AZ::Dom::PatchOperation& domOperation, size_t pathIndex = 0);

        //! returns the last descendent of this row in its own layout
        DPERowWidget* GetLastDescendantInLayout();

        void SetExpanded(bool expanded, bool recurseToChildRows = false);
        bool IsExpanded() const;

        const AZ::Dom::Path GetPath() const;

        bool HasChildRows() const;
        int GetLevel() const;

    protected slots:
        void onExpanderChanged(int expanderState);

    protected:
        DocumentPropertyEditor* GetDPE() const;
        void AddDomChildWidget(size_t domIndex, QWidget* childWidget);
        void AddColumnWidget(QWidget* columnWidget, size_t domIndex, const AZ::Dom::Value& domValue);

        AZ::Dom::Path BuildDomPath();
        void SaveExpanderStatesForChildRows(bool isExpanded);

        QWidget* CreateWidgetForHandler(PropertyEditorToolsSystemInterface::PropertyHandlerId handlerId, const AZ::Dom::Value& domValue);

        DPERowWidget* m_parentRow = nullptr;
        int m_depth = 0; //!< number of levels deep in the tree. Used for indentation
        DPELayout* m_columnLayout = nullptr;

        // This widget's indexed path from the root
        AZ::Dom::Path m_domPath;

        //! widget children in DOM specified order; mix of row and column widgets
        AZStd::deque<QWidget*> m_domOrderedChildren;

        // row attributes extracted from the DOM
        AZStd::optional<bool> m_forceAutoExpand;
        AZStd::optional<bool> m_expandByDefault;

        // a map from the propertyHandler widgets to the propertyHandlers that created them
        struct HandlerInfo
        {
            PropertyEditorToolsSystemInterface::PropertyHandlerId handlerId = nullptr;
            AZStd::unique_ptr<PropertyHandlerWidgetInterface> hanlderInterface;
        };
        AZStd::unordered_map<QWidget*, HandlerInfo> m_widgetToPropertyHandlerInfo;
    };

    class DocumentPropertyEditor
        : public QScrollArea
        , public IPropertyEditor
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

        void SetSavedExpanderStateForRow(const AZ::Dom::Path& rowPath, bool isExpanded);
        bool GetSavedExpanderStateForRow(const AZ::Dom::Path& rowPath) const;
        bool HasSavedExpanderStateForRow(const AZ::Dom::Path& rowPath) const;
        void RemoveExpanderStateForRow(const AZ::Dom::Path& rowPath);
        void ExpandAll();
        void CollapseAll();

        // IPropertyEditor overrides
        void SetSavedStateKey(AZ::u32 key, AZStd::string propertyEditorName = {}) override;

        AZ::Dom::Value GetDomValueForRow(DPERowWidget* row) const;

        void ReleaseHandler(AZStd::unique_ptr<PropertyHandlerWidgetInterface>&& handler);

        // sets whether this DPE should also spawn a DPEDebugWindow when its adapter
        // is set. Initially, this takes its value from the CVAR ed_showDPEDebugView,
        // but can be overridden here
        void SetSpawnDebugView(bool shouldSpawn);

        static bool ShouldReplaceRPE();

        AZStd::vector<size_t> GetPathToRoot(DPERowWidget* row) const;
        bool IsRecursiveExpansionOngoing() const;
        void SetRecursiveExpansionOngoing(bool isExpanding);

    public slots:
        //! set the DOM adapter for this DPE to inspect
        void SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapterPtr theAdapter);
        void Clear();

    protected:
        QVBoxLayout* GetVerticalLayout();

        void HandleReset();
        void HandleDomChange(const AZ::Dom::Patch& patch);
        void HandleDomMessage(const AZ::DocumentPropertyEditor::AdapterMessage& message, AZ::Dom::Value& value);

        void CleanupReleasedHandlers();

        AZ::DocumentPropertyEditor::DocumentAdapterPtr m_adapter;
        AZ::DocumentPropertyEditor::DocumentAdapter::ResetEvent::Handler m_resetHandler;
        AZ::DocumentPropertyEditor::DocumentAdapter::ChangedEvent::Handler m_changedHandler;
        AZ::DocumentPropertyEditor::DocumentAdapter::MessageEvent::Handler m_domMessageHandler;

        QVBoxLayout* m_layout = nullptr;

        AZStd::unique_ptr<DocumentPropertyEditorSettings> m_dpeSettings;
        bool m_isRecursiveExpansionOngoing = false;

        bool m_spawnDebugView = false;

        QTimer* m_handlerCleanupTimer;
        AZStd::vector<AZStd::unique_ptr<PropertyHandlerWidgetInterface>> m_unusedHandlers;
        DPERowWidget* m_rootNode = nullptr;
    };
} // namespace AzToolsFramework
