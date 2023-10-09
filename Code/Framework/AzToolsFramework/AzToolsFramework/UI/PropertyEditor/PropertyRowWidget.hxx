/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // class '...' needs to have dll-interface to be used by clients of class '...'
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFrame>
#include <QtCore/QPointer>
#include <QtCore/QElapsedTimer>
AZ_POP_DISABLE_WARNING

#include "PropertyEditorAPI.h"
#endif

namespace AzToolsFramework
{

    class PropertyHandlerBase;
    // the purpose of a Property Row Widget is to house the user's property GUI
    // and serve as a conduit to talk to the system.  You should never need to do anything
    // with this guy, except tell him to expand, collapse, etc.
    class PropertyRowWidget : public QFrame
    {
        Q_OBJECT;
        Q_PROPERTY(bool hasChildRows READ HasChildRows);
        Q_PROPERTY(bool isTopLevel READ IsTopLevel);
        Q_PROPERTY(int getLevel READ GetLevel);
        Q_PROPERTY(bool canBeReordered READ CanBeReordered);
        Q_PROPERTY(bool appendDefaultLabelToName READ GetAppendDefaultLabelToName WRITE AppendDefaultLabelToName)
    public:
        AZ_CLASS_ALLOCATOR(PropertyRowWidget, AZ::SystemAllocator)

        enum class DragImageType
        {
            SingleRow,
            IncludeVisibleChildren
        };

        PropertyRowWidget(QWidget* pParent);
        virtual ~PropertyRowWidget();

        virtual void Initialize(PropertyRowWidget* pParent, InstanceDataNode* dataNode, int depth, int labelWidth = 200);
        virtual void Initialize(const char* groupName, PropertyRowWidget* pParent, int depth, int labelWidth = 200);
        virtual void InitializeToggleGroup(const char* groupName, PropertyRowWidget* pParent, int depth, InstanceDataNode* node, int labelWidth = 200);
        virtual void Clear(); // for pooling

        // --- NOT A UNIQUE IDENTIFIER ---
        virtual AZ::u32 GetIdentifier() const; // retrieve a stable identifier that identifies this node (note: Does not include heirarchy).  Use only for attempts to restore state.

        bool IsForbidExpansion() const; 
        bool ForceAutoExpand() const;
        bool AutoExpand() const { return m_autoExpand; }
        bool IsContainer() const { return m_isContainer; }
        bool IsContainerEditable() const { return m_isContainer && m_containerEditable; }
        
        int GetDepth() const { return m_treeDepth; }

        virtual void AddedChild(PropertyRowWidget* child);

        virtual void SetExpanded(bool expanded);
        virtual bool IsExpanded() const { return m_expanded; }

        void DoExpandOrContract(bool expand, bool includeDescendents = false);

        PropertyHandlerBase* GetHandler() const;
        InstanceDataNode* GetNode() const { return m_sourceNode; }
        bool HasChildWidgetAlready() const; 
        void ConsumeChildWidget(QWidget* pChild);
        QWidget* GetChildWidget() const { return m_childWidget; }

        void SetParentRow(PropertyRowWidget* pParentRowWidget) { m_parentRow = pParentRowWidget; }
        PropertyRowWidget* GetParentRow() const { return m_parentRow; }
        int GetLevel() const;
        bool IsTopLevel() const;

        // Remove the default label and append the text to the name label.
        bool GetAppendDefaultLabelToName();
        void AppendDefaultLabelToName(bool doAppend);

        AZ::u32 GetChildRowCount() const;
        PropertyRowWidget* GetChildRowByIndex(AZ::u32 index) const;

        AZStd::vector<PropertyRowWidget*>& GetChildrenRows() { return m_childrenRows; }
        bool HasChildRows() const;

        // check if theres a notification function.
        PropertyModificationRefreshLevel DoPropertyNotify(size_t optionalIndex = 0);
        void DoEditingCompleteNotify();

        // validate a change if there are validation functions specified
        bool ShouldPreValidatePropertyChange() const;
        bool ValidatePropertyChange(void* valueToValidate, const AZ::Uuid& valueType) const;

        void RefreshAttributesFromNode(bool initial);
        void ConsumeAttribute(AZ::u32 attributeName, PropertyAttributeReader& reader, bool initial, QString* descriptionOut=nullptr, bool* foundDescriptionOut=nullptr);

        void SetReadOnlyQueryFunction(const ReadOnlyQueryFunction& readOnlyQueryFunction);

        /// Repaint the control style, which is required any time object properties used
        /// by .qss are modified.
        void RefreshStyle();

        /// Post-process based on source node data.
        void OnValuesUpdated();

        QString label() const;

        QWidget* GetFirstTabWidget();
        void UpdateWidgetInternalTabbing();
        QWidget* GetLastTabWidget();

        //return size hint for left-hand side layout including the name label and any indentation
        QSize LabelSizeHint() const;
        void SetLabelWidth(int width);

        void SetTreeIndentation(int indentation);
        void SetLeafIndentation(int indentation);

        void SetSelectionEnabled(bool selectionEnabled);
        void SetSelected(bool selected);
        bool GetSelected();
        bool eventFilter(QObject *watched, QEvent *event) override;
        void paintEvent(QPaintEvent*) override;

        /// Apply tooltip to widget and some of its children.
        void SetDescription(const QString& text);

        void HideContent();

        void SetNameLabel(const char* text);

        void UpdateIndicator(const char* imagePath);

        void SetFilterString(const AZStd::string& str);

        // Get notifications when widget refreshes should be prevented / restored
        void PreventRefresh(bool shouldPrevent);

        QVBoxLayout* GetLeftHandSideLayoutParent() { return m_leftHandSideLayoutParent; }
        QToolButton* GetIndicatorButton() { return m_indicatorButton; }
        QLabel* GetNameLabel() { return m_nameLabel; }
        QWidget* GetToggle() { return m_toggleSwitch; }
        const QWidget* GetToggle() const { return m_toggleSwitch; }
        void SetIndentSize(int w);
        void SetAsCustom(bool custom) { m_custom = custom; }

        bool CanChildrenBeReordered() const;
        bool CanBeReordered() const;

        int GetIndexInParent() const;
        bool CanMoveUp() const;
        bool CanMoveDown() const;

        int GetContainingEditorFrameWidth();
        QPixmap createDragImage(const QColor backgroundColor, const QColor borderColor, const float alpha, DragImageType imageType);
    protected:
        int CalculateLabelWidth() const;

        int GetHeightOfRowAndVisibleChildren();
        int DrawDragImageAndVisibleChildrenInto(QPainter& painter, int xpos, int ypos);

        bool IsHidden(InstanceDataNode* node) const;

        struct ChangeNotification;
        void HandleChangeNotifyAttribute(PropertyAttributeReader& reader, InstanceDataNode* node, AZStd::vector<ChangeNotification>& notifiers);
        void HandleChangeValidateAttribute(PropertyAttributeReader& reader, InstanceDataNode* node);
        InstanceDataNode* ResolveToNodeByType(InstanceDataNode* startNode, const AZ::Uuid& typeId) const;

        QHBoxLayout* m_mainLayout;
        QVBoxLayout* m_leftHandSideLayoutParent;
        QHBoxLayout* m_leftHandSideLayout;
        QHBoxLayout* m_middleLayout;
        QHBoxLayout* m_rightHandSideLayout;
        
        QPointer <QCheckBox> m_dropDownArrow;
        QPointer <QToolButton> m_containerClearButton;
        QPointer <QToolButton> m_containerAddButton;
        QPointer <QToolButton> m_elementRemoveButton;

        QWidget* m_leftAreaContainer;
        QWidget* m_middleAreaContainer;

        QToolButton* m_indicatorButton;
        AzQtComponents::ElidingLabel* m_nameLabel;
        QLabel* m_defaultLabel; // if there is no handler, we use a m_defaultLabel label
        InstanceDataNode* m_sourceNode;

        QWidget* m_toggleSwitch = nullptr;

        QString m_currentFilterString;

        struct ChangeNotification
        {
            ChangeNotification(InstanceDataNode* node, AZ::Edit::Attribute* attribute)
                : m_attribute(attribute)
                , m_node(node)
            { }

            InstanceDataNode* m_node = nullptr;
            AZ::Edit::Attribute* m_attribute = nullptr;
        };

        AZStd::vector<ChangeNotification> m_changeNotifiers; 
        AZStd::vector<ChangeNotification> m_changeValidators;
        AZStd::vector<ChangeNotification> m_editingCompleteNotifiers;
        QSpacerItem* m_indent;
        PropertyHandlerBase* m_handler = nullptr; // the CURRENT handler.
        PropertyRowWidget* m_parentRow = nullptr; // not the parent widget.
        AZStd::vector<PropertyRowWidget*> m_childrenRows; // children rows of this row.

        QWidget* m_childWidget = nullptr;

        bool m_forbidExpansion = false;
        bool m_forceAutoExpand = false;
        bool m_autoExpand = false;
        bool m_expanded = false;
        bool m_containerEditable = false;
        bool m_isContainer = false;
        bool m_initialized = false;
        bool m_isMultiSizeContainer = false;
        bool m_isFixedSizeOrSmartPtrContainer = false;
        bool m_custom = false;
        bool m_isSceneSetting = false;

        bool m_isSelected = false;
        bool m_selectionEnabled = false;
        bool m_readOnly = false; // whether widget is currently read-only
        bool m_readOnlyDueToAttribute = false; // holds whether the ReadOnly attribute was set
        bool m_readOnlyDueToFunction = false; // holds whether m_readOnlyQueryFunction evaluates to true
        ReadOnlyQueryFunction m_readOnlyQueryFunction; // customizable function that can say a property is read-only

        QElapsedTimer m_clickStartTimer;
        QPoint m_clickPos;

        int m_containerSize = 0;
        int m_requestedLabelWidth = 0;
        AZ::u32 m_identifier = 0;
        AZ::u32 m_handlerName = 0;
        AZStd::string m_defaultValueString;
        bool m_hadChildren = false;
        int m_treeDepth = 0;
        int m_treeIndentation = 14;
        int m_leafIndentation = 16;

        QIcon m_iconOpen;
        QIcon m_iconClosed;

        bool m_reorderAllow = false;

        /// Marks the field to be visualized as "overridden".
        void SetOverridden(bool overridden);

        void contextMenuEvent(QContextMenuEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

        void UpdateDropDownArrow();
        void CreateGroupToggleSwitch();
        void ChangeSourceNode(InstanceDataNode* node);
        void UpdateDefaultLabel(InstanceDataNode* node);

        void createContainerButtons();

        void UpdateReadOnlyState();
        void UpdateEnabledState();

    signals:
        void onUserExpandedOrContracted(InstanceDataNode* node, bool expanded);
        void onRequestedContainerClear(InstanceDataNode* node);
        void onRequestedContainerElementRemove(InstanceDataNode* node);
        void onRequestedContainerAdd(InstanceDataNode* node);
        void onRequestedContextMenu(InstanceDataNode* node, const QPoint& point);
        void onRequestedSelection(InstanceDataNode* node);

    private slots:
        void OnClickedExpansionButton();

        void OnClickedToggleButton(bool checked);
        void OnClickedAddElementButton();
        void OnClickedRemoveElementButton();
        void OnClickedClearContainerButton();
        void OnContextMenuRequested(const QPoint&);

    };
}

