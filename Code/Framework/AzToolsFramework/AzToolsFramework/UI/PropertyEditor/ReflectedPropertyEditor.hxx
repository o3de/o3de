/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef REFLECTEDPROPERTYEDITOR_H
#define REFLECTEDPROPERTYEDITOR_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/IPropertyEditor.h>
#include "PropertyEditorAPI.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QFrame>
#endif

class QScrollArea;
class QLayout;
class QSpacerItem;
class QVBoxLayout;

namespace AZ
{
    struct ClassDataReflection;
    class SerializeContext;
}

namespace AzToolsFramework
{
    class ReflectedPropertyEditorState;
    class PropertyRowWidget;
    class ComponentEditor;

    class ReflectedPropertyEditor;
    AZ_TYPE_INFO_SPECIALIZE(ReflectedPropertyEditor, "{F5B220A4-16DF-4E03-B0B2-CF776D59B9B7}");

    /**
     * the Reflected Property Editor is a Qt Control which you can place inside a GUI, which you then feed
     * a series of object(s) and instances.  Any object or instance with Editor reflection will then be editable
     * in the Reflected Property editor control, with the GUI arrangement specified in the edit reflection for
     * those objects.
     */
    class ReflectedPropertyEditor
        : public QFrame
        , public IPropertyEditor
    {
        Q_OBJECT
    public:
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR(ReflectedPropertyEditor, AZ::SystemAllocator);

        typedef AZStd::unordered_map<InstanceDataNode*, PropertyRowWidget*> WidgetList;

        ReflectedPropertyEditor::WidgetList m_specialGroupWidgets;

        ReflectedPropertyEditor(QWidget* pParent);
        virtual ~ReflectedPropertyEditor();

        void Setup(AZ::SerializeContext* context, IPropertyEditorNotify* ptrNotify, bool enableScrollbars, int propertyLabelWidth = 200, ComponentEditor *editorParent = nullptr);

        // allows disabling display of root container property widgets
        void SetHideRootProperties(bool hideRootProperties) override;

        bool AddInstance(void* instance, const AZ::Uuid& classId, void* aggregateInstance = nullptr, void* compareInstance = nullptr) override;
        void SetCompareInstance(void* instance, const AZ::Uuid& classId);
        void ClearInstances() override;
        void ReadValuesIntoGui(QWidget* widget, InstanceDataNode* node);
        template<class T>
        bool AddInstance(T* instance, void* aggregateInstance = nullptr, void* compareInstance = nullptr)
        {
            return AddInstance(instance, azrtti_typeid(instance), aggregateInstance, compareInstance);
        }

        void InvalidateAll(const char* filter = nullptr) override; // recreates the entire tree of properties.
        void InvalidateAttributesAndValues(); // re-reads all attributes, and all values.
        void InvalidateValues(); // just updates the values inside properties.

        void SetFilterString(AZStd::string str) override;
        AZStd::string GetFilterString();
        // a settings key which is used to store and load the set of things that are expanded or not and other settings
        void SetSavedStateKey([[maybe_unused]] AZ::u32 key, [[maybe_unused]] AZStd::string propertyEditorName = "") override;

        void QueueInvalidation(PropertyModificationRefreshLevel level) override;
        //will force any queued invalidations to happen immediately
        void ForceQueuedInvalidation();

        void CancelQueuedRefresh() override; // Cancels any pending property refreshes

        //! When set, disables data access for this property editor.
        //! This prevents any value refreshes from the inspected values from occurring as well as disabling user input.
        void PreventDataAccess(bool shouldPrevent) override;
        // O3DE_DEPRECATED(LY-120821)
        void PreventRefresh(bool shouldPrevent){PreventDataAccess(shouldPrevent);}

        void SetAutoResizeLabels(bool autoResizeLabels) override;

        InstanceDataNode* GetNodeFromWidget(QWidget* pTarget) const;
        PropertyRowWidget* GetWidgetFromNode(InstanceDataNode* node) const override;

        void ExpandAll(bool saveExpansionState = true);
        void CollapseAll(bool saveExpansionState = true);

        const WidgetList& GetWidgets() const;

        int GetContentHeight() const;
        int GetMaxLabelWidth() const;

        void SetLabelAutoResizeMinimumWidth(int labelMinimumWidth);
        void SetLabelWidth(int labelWidth);

        void SetSelectionEnabled(bool selectionEnabled);
        void SelectInstance(InstanceDataNode* node);
        InstanceDataNode* GetSelectedInstance() const;

        QSize sizeHint() const override;

        using InstanceDataHierarchyCallBack = AZStd::function<void(AzToolsFramework::InstanceDataHierarchy& /*hierarchy*/)>;
        void EnumerateInstances(InstanceDataHierarchyCallBack enumerationCallback) override;

        void SetValueComparisonFunction(const InstanceDataHierarchy::ValueComparisonFunction& valueComparisonFunction) override;

        // Set custom function for evaluating whether a node is read-only
        void SetReadOnlyQueryFunction(const ReadOnlyQueryFunction& readOnlyQueryFunction) override;

        // Set custom function for evaluating whether a node is hidden
        void SetHiddenQueryFunction(const HiddenQueryFunction& hiddenQueryFunction) override;

        bool HasFilteredOutNodes() const override;
        bool HasVisibleNodes() const override;

        // Set custom function for evaluating if we a given node should show an indicator or not
        void SetIndicatorQueryFunction(const IndicatorQueryFunction& indicatorQueryFunction) override;

        // if you want it to save its state, you need to give it a user settings label:
        //void SetSavedStateLabel(AZ::u32 label);
        //static void Reflect(const AZ::ClassDataReflection& reflection);

        void SetDynamicEditDataProvider(DynamicEditDataProvider provider) override;


        QWidget* GetContainerWidget();
        
        void SetSizeHintOffset(const QSize& offset);
        QSize GetSizeHintOffset() const;

        // Controls the indentation of the different levels in the tree. 
        // Note: do not call this method after adding instances, this indentation value is passed to PropertyRowWidgets 
        // during construction, therefore doesn't support updating dynamically after showing the widget.
        void SetTreeIndentation(int indentation);
        
        // Controls the indentation of the leafs in the tree. 
        // Note: do not call this method after adding instances, this indentation value is passed to PropertyRowWidgets 
        // during construction, therefore doesn't support updating dynamically after showing the widget.
        void SetLeafIndentation(int indentation);

        using VisibilityCallback = AZStd::function<void(InstanceDataNode* node, NodeDisplayVisibility& visibility, bool& checkChildVisibility)>;
        void SetVisibilityCallback(VisibilityCallback callback);

        void MoveNodeToIndex(InstanceDataNode* node, int index) override;
        void MoveNodeBefore(InstanceDataNode* nodeToMove, InstanceDataNode* nodeToMoveBefore) override;
        void MoveNodeAfter(InstanceDataNode* nodeToMove, InstanceDataNode* nodeToMoveBefore) override;

        int GetNodeIndexInContainer(InstanceDataNode* node);
        InstanceDataNode* GetNodeAtIndex(int index);
        QSet<PropertyRowWidget*> GetTopLevelWidgets() override;
    signals:
        void OnExpansionContractionDone();
    private:
        InstanceDataNode* FindContainerNodeForNode(InstanceDataNode* node) const;
        void ChangeNodeIndex(InstanceDataNode* containerNode, InstanceDataNode* node, int oldIndex, int newIndex);

        class Impl;
        std::unique_ptr<Impl> m_impl;
        
        AZStd::string m_currentFilterString;

        virtual void paintEvent(QPaintEvent* event) override;
        int m_updateDepth = 0;
    signals:
        void releasePrompt();
    private slots:
        void OnPropertyRowExpandedOrContracted(PropertyRowWidget* widget, InstanceDataNode* node, bool expanded, bool fromUserInteraction);
        void DoRefresh();
        void RecreateTabOrder();
        void OnPropertyRowRequestClear(PropertyRowWidget* widget, InstanceDataNode* node);
        void OnPropertyRowRequestContainerRemoveItem(PropertyRowWidget* widget, InstanceDataNode* node);
        void OnPropertyRowRequestContainerAddItem(PropertyRowWidget* widget, InstanceDataNode* node);
    };
}

#endif
