/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Instance/InstancePool.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzFramework/DocumentPropertyEditor/ExpanderSettings.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/IPropertyEditor.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyEditorToolsSystemInterface.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyHandlerWidget.h>

#include <QHBoxLayout>
#include <QScrollArea>

#endif // Q_MOC_RUN

class QCheckBox;

namespace AzQtComponents
{
    class ElidingLabel;
};

namespace AzToolsFramework
{
    class DocumentPropertyEditor;
    class DPERowWidget;

    class DPELayout : public QHBoxLayout
    {
        Q_OBJECT

        friend class DPERowWidget;
    signals:
        void expanderChanged(bool expanded);

        // todo: look into caching and QLayoutItem::invalidate()
    public:
        DPELayout(QWidget* parent);
        void Init(int depth, bool enforceMinWidth, QWidget* parentWidget = nullptr);
        void Clear();
        virtual ~DPELayout();

        void SetExpanderShown(bool shouldShow);
        void SetExpanded(bool expanded);
        bool IsExpanded() const;

        void SetAsStartOfNewColumn(size_t widgetIndex);

        // QLayout overrides
        void invalidate() override;
        QSize sizeHint() const override;
        QSize minimumSize() const override;
        void setGeometry(const QRect& rect) override;
        Qt::Orientations expandingDirections() const override;

    protected slots:
        void onCheckstateChanged(int expanderState);

    protected:
        DPERowWidget* GetRow() const;
        void CreateExpanderWidget();

        int m_depth = 0; //!< number of levels deep in the tree. Used for indentation
        bool m_showExpander = false;
        bool m_enforceMinWidth = true;
        bool m_expanded = true;
        QCheckBox* m_expanderWidget = nullptr;

        // Contains the indices of all widgets that start a new column.
        AZStd::unordered_set<size_t> m_columnStarts;

    private:
        void CloseColumn(
            QHBoxLayout* currentColumnLayout,
            QRect& itemGeometry,
            int& currentColumnCount,
            const int columnWidth,
            bool allWidgetsUnstretched,
            bool startSpacer,
            bool endSpacer
        );

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
        friend class DPELayout;

    public:
        explicit DPERowWidget();
        void Clear(); //!< destroy all layout contents and clear DOM children
        ~DPERowWidget();

        DPERowWidget* GetPriorRowInLayout(size_t domIndex);
        int GetDomIndexOfChild(const QWidget* childWidget) const; // returns domIndex of the given widget, -1 for not found
        QWidget* GetChild(size_t domIndex);
        void AddChildFromDomValue(const AZ::Dom::Value& childValue, size_t domIndex);
        void RemoveChildAt(size_t childIndex, QWidget** newOwner = nullptr);

        //! clears and repopulates all children from a given DOM array
        void SetValueFromDom(const AZ::Dom::Value& domArray);
        void SetAttributesFromDom(const AZ::Dom::Value& domArray);

        void SetPropertyEditorAttributes(size_t domIndex, const AZ::Dom::Value& domArray, QWidget* childWidget);
        void RemoveCachedAttributes(size_t domIndex);
        void ClearCachedAttributes();

        //! handles a patch operation at the given path, or delegates to a child that will
        void HandleOperationAtPath(const AZ::Dom::PatchOperation& domOperation, size_t pathIndex = 0);

        //! returns the last descendant of this row in its own layout
        DPERowWidget* GetLastDescendantInLayout();

        void SetExpanded(bool expanded, bool recurseToChildRows = false);
        bool IsExpanded() const;
        void ApplyExpansionState(const AZ::Dom::Path& rowPath, DocumentPropertyEditor* rowDPE);

        bool HasChildRows() const;
        int GetLevel() const;

    protected slots:
        void onExpanderChanged(int expanderState);

    protected:
        DocumentPropertyEditor* GetDPE() const;
        void AddDomChildWidget(size_t domIndex, QWidget* childWidget);
        void AddColumnWidget(QWidget* columnWidget, size_t domIndex, const AZ::Dom::Value& domValue);
        void AddRowChild(DPERowWidget* rowWidget, size_t domIndex);
        void PlaceRowChild(DPERowWidget* rowWidget, size_t domIndex);

        AZ::Dom::Path BuildDomPath() const;
        void SaveExpanderStatesForChildRows(bool isExpanded);

        static bool ValueHasChildRows(const AZ::Dom::Value& rowValue);

        DPERowWidget* m_parentRow = nullptr;
        int m_depth = -1; //!< number of levels deep in the tree. Used for indentation
        DPELayout* m_columnLayout = nullptr;
        bool m_enforceMinWidth = true;

        //! widget children in DOM specified order; mix of row and column widgets
        AZStd::deque<QWidget*> m_domOrderedChildren;

        struct AttributeInfo
        {
            AZ::Dpe::Nodes::PropertyEditor::Align m_alignment = AZ::Dpe::Nodes::PropertyEditor::Align::UseDefaultAlignment;
            bool m_sharePriorColumn = false;
            bool m_minimumWidth = false;

            bool IsDefault() const
            {
                return m_alignment == AZ::Dpe::Nodes::PropertyEditor::Align::UseDefaultAlignment && !m_sharePriorColumn && !m_minimumWidth;
            }
        };
        AZStd::unordered_map<size_t, AttributeInfo> m_childIndexToCachedAttributeInfo;
        AttributeInfo* GetCachedAttributes(size_t domIndex);

        bool m_expandingProgrammatically = false; //!< indicates whether an expansion is in progress

        // row attributes extracted from the DOM
        AZStd::optional<bool> m_forceAutoExpand;
        AZStd::optional<bool> m_expandByDefault;
    };

    class DocumentPropertyEditor
        : public QScrollArea
        , public IPropertyEditor
    {
        Q_OBJECT
        friend class DPERowWidget;

    public:
        AZ_CLASS_ALLOCATOR(DocumentPropertyEditor, AZ::SystemAllocator);

        explicit DocumentPropertyEditor(QWidget* parentWidget = nullptr);
        ~DocumentPropertyEditor();

        /*! Sets whether this DPE should allow vertical scrolling and show a scrollbar, or just take up
            the full space that its contents requests.
            This is typically used when a DPE is going into another scroll area and it is undesirable
            for the DPE to provide its own vertical scrollbar */
        void SetAllowVerticalScroll(bool allowVerticalScroll);

        /*! Sets whether this DPE should enforce the minimum width of its sub-widgets, or allow the user
         *  to make the DPE arbitrarily narrow. */
        void SetEnforceMinWidth(bool enforceMinWidth);

        virtual QSize sizeHint() const override;

        auto GetAdapter()
        {
            return m_adapter;
        }

        const auto GetAdapter() const
        {
            return m_adapter;
        }
        void AddAfterWidget(QWidget* precursor, QWidget* widgetToAdd);

        void SetSavedExpanderStateForRow(const AZ::Dom::Path& rowPath, bool isExpanded);
        bool GetSavedExpanderStateForRow(const AZ::Dom::Path& rowPath) const;
        bool HasSavedExpanderStateForRow(const AZ::Dom::Path& rowPath) const;
        bool ShouldEraseExpanderStateWhenRowRemoved() const;
        void RemoveExpanderStateForRow(const AZ::Dom::Path& rowPath);
        void ApplyExpansionStates();
        void ExpandAll();
        void CollapseAll();

        // IPropertyEditor overrides
        void SetSavedStateKey(AZ::u32 key, AZStd::string propertyEditorName = {}) override;
        void ClearInstances() override;

        AZ::Dom::Value GetDomValueForRow(DPERowWidget* row) const;

        // sets whether this DPE should also spawn a DPEDebugWindow when its adapter
        // is set. Initially, this takes its value from the CVAR ed_showDPEDebugView,
        // but can be overridden here
        void SetSpawnDebugView(bool shouldSpawn);

        static bool ShouldReplaceCVarEditor();

        static constexpr const char* GetEnableCVarEditorName()
        {
            return "ed_enableCVarDPE";
        }

        AZStd::vector<size_t> GetPathToRoot(const DPERowWidget* row) const;
        bool IsRecursiveExpansionOngoing() const;
        void SetRecursiveExpansionOngoing(bool isExpanding);

        // shared pools of recycled widgets
        static auto GetRowPool()
        {
            return static_cast<AZ::InstancePoolManager*>(AZ::Interface<AZ::InstancePoolManagerInterface>::Get())->GetPool<DPERowWidget>();
        }

        static auto GetLabelPool()
        {
            return static_cast<AZ::InstancePoolManager*>(AZ::Interface<AZ::InstancePoolManagerInterface>::Get())
                ->GetPool<AzQtComponents::ElidingLabel>();
        }

        void RegisterHandlerPool(AZ::Name handlerName, AZStd::shared_ptr<AZ::InstancePoolBase> handlerPool);

        struct HandlerInfo
        {
            PropertyEditorToolsSystemInterface::PropertyHandlerId handlerId = nullptr;
            PropertyHandlerWidgetInterface* handlerInterface = nullptr;

            bool IsNull()
            {
                return !handlerId && !handlerInterface;
            }
        };
        static HandlerInfo GetInfoFromWidget(const QWidget* widget);

    signals:
        void ExpanderChangedByUser();
        void RequestSizeUpdate(); //!< needed to inform the ComponentEditor Card that the DPE's sizehint needs to be re-evaluated

    public slots:
        //! set the DOM adapter for this DPE to inspect
        void SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapterPtr theAdapter);
        void Clear();

    protected:
        QVBoxLayout* GetVerticalLayout();

        QWidget* GetWidgetAtPath(const AZ::Dom::Path& path);

        void HandleReset();
        void HandleDomChange(const AZ::Dom::Patch& patch);
        void HandleDomMessage(const AZ::DocumentPropertyEditor::AdapterMessage& message, AZ::Dom::Value& value);

        AZ::DocumentPropertyEditor::DocumentAdapterPtr m_adapter;
        AZ::DocumentPropertyEditor::DocumentAdapter::ResetEvent::Handler m_resetHandler;
        AZ::DocumentPropertyEditor::DocumentAdapter::ChangedEvent::Handler m_changedHandler;
        AZ::DocumentPropertyEditor::DocumentAdapter::MessageEvent::Handler m_domMessageHandler;

        QVBoxLayout* m_layout = nullptr;
        bool m_allowVerticalScroll = true;
        bool m_enforceMinWidth = true;

        AZStd::unique_ptr<AZ::DocumentPropertyEditor::ExpanderSettings> m_dpeSettings;
        bool m_isRecursiveExpansionOngoing = false;
        bool m_spawnDebugView = false;

        DPERowWidget* m_rootNode = nullptr;

        bool m_isBeingCleared = false;

        // keep pools of frequently used widgets that can be recycled for efficiency without
        // incurring the cost of creating and destroying them
        AZStd::shared_ptr<AZ::InstancePool<DPERowWidget>> m_rowPool;
        AZStd::shared_ptr<AZ::InstancePool<AzQtComponents::ElidingLabel>> m_labelPool;

        QWidget* CreateWidgetForHandler(PropertyEditorToolsSystemInterface::PropertyHandlerId handlerId, const AZ::Dom::Value& domValue);
        static AZ::Name GetNameForHandlerId(PropertyEditorToolsSystemInterface::PropertyHandlerId handlerId);
        static void ReleaseHandler(HandlerInfo& handler);

        // Co-owns the handler pool that is needed in DPE and the ownership would be released when the DPE is deleted
        AZStd::unordered_map<AZ::Name, AZStd::shared_ptr<AZ::InstancePoolBase>> m_handlerPools;
    };
} // namespace AzToolsFramework

// expose type info for external classes so that they can be used with the InstancePool system
namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzToolsFramework::DPERowWidget, "{C457A594-6E19-4674-A617-3CC09CF7E532}");
    AZ_TYPE_INFO_SPECIALIZE(AzQtComponents::ElidingLabel, "{02674C46-1401-4237-97F1-2774A067BF80}");
} // namespace AZ

Q_DECLARE_METATYPE(AzToolsFramework::DocumentPropertyEditor::HandlerInfo);
