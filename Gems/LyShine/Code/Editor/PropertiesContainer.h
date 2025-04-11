/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QScrollArea>

#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>

#include "EditorCommon.h"
#endif

class PropertiesWidget;
class EditorWindow;

class PropertiesContainer
    : public QScrollArea
{
    Q_OBJECT

public:

    PropertiesContainer(PropertiesWidget* propertiesWidget,
        EditorWindow* editorWindow);

    void Refresh(AzToolsFramework::PropertyModificationRefreshLevel refreshLevel = AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree, const AZ::Uuid* componentType = nullptr);
    void SelectionChanged(HierarchyItemRawPtrList* items);
    void SelectedEntityPointersChanged();
    bool IsCanvasSelected() { return m_isCanvasSelected; }
    AZ::Entity::ComponentArrayType GetSelectedComponents();

    void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* node, const QPoint& globalPos);

    void SetSelectedEntityDisplayNameWidget(QLineEdit* selectedEntityDisplayNameWidget);
    void SetEditorOnlyCheckbox(QCheckBox* editorOnlyCheckbox);

private:

    // A SharedComponentInfo represents one component
    // which all selected entities have in common.
    // If entities have multiple of the same component-type
    // then there will be a SharedComponentInfo for each.
    // Example: Say 3 entities are selected and each entity has 2 MeshComponents.
    // There will be 2 SharedComponentInfo, one for each MeshComponent.
    // Each SharedComponentInfo::m_instances has 3 entries,
    // one for the Nth MeshComponent in each entity.
    struct SharedComponentInfo
    {
        SharedComponentInfo()
            : m_classData(nullptr)
            , m_compareInstance(nullptr)
        {}
        const AZ::SerializeContext::ClassData* m_classData;

        /// Components instanced (one from each entity).
        AZ::Entity::ComponentArrayType m_instances;

        /// Canonical instance to compare others against
        AZ::Component* m_compareInstance;
    };

    // Widget overrides
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool eventFilter(QObject* object, QEvent* event) override;

    bool HandleSelectionEvents(QObject* object, QEvent* event);
    bool m_selectionEventAccepted; // ensure selection logic executes only once per click since eventFilter may execute multiple times for a single click

    // A collection of SharedComponentInfo.
    // The map is keyed on the component-type.
    // In the case of /ref GenericComponentWrapper,
    // the type corresponds to the component-type being wrapped,
    // though SharedComponentInfo::m_instances still point to the
    // GenericComponentWrapper*.
    using ComponentTypeMap = AZStd::unordered_map<AZ::Uuid, AZStd::vector<SharedComponentInfo>>;

    void BuildSharedComponentList(ComponentTypeMap&, const AzToolsFramework::EntityIdList& entitiesShown);
    void BuildSharedComponentUI(ComponentTypeMap&, const AzToolsFramework::EntityIdList& entitiesShown);
    AzToolsFramework::ComponentEditor* CreateComponentEditor(const AZ::Component& componentInstance);

    // Helper functions for selecting components
    bool DoesOwnFocus() const;
    QRect GetWidgetGlobalRect(const QWidget* widget) const;
    bool DoesIntersectWidget(const QRect& globalRect, const QWidget* widget) const;
    bool DoesIntersectSelectedComponentEditor(const QRect& globalRect) const;
    bool DoesIntersectNonSelectedComponentEditor(const QRect& globalRect) const;

    void ClearComponentEditorSelection();
    void SelectRangeOfComponentEditors(const AZ::s32 index1, const AZ::s32 index2, bool selected = true);
    void SelectIntersectingComponentEditors(const QRect& globalRect, bool selected = true);
    void ToggleIntersectingComponentEditors(const QRect& globalRect);
    AZ::s32 GetComponentEditorIndex(const AzToolsFramework::ComponentEditor* componentEditor) const;
    AZStd::vector<AzToolsFramework::ComponentEditor*> GetIntersectingComponentEditors(const QRect& globalRect) const;

    // Create actions to add/remove/cut/copy/paste components
    void CreateActions();

    // Update states
    void UpdateActions();
    void UpdateOverlay();
    void UpdateInternalState();

    // Called when an action to add a component was triggered
    void OnAddComponent();

    // Context menu helpers
    void OnDisplayUiComponentEditorMenu(const QPoint& position);
    void ShowContextMenu(const QPoint& position);

    void Update();
    void UpdateEditorOnlyCheckbox();

    PropertiesWidget* m_propertiesWidget;
    EditorWindow* m_editorWindow;

    QWidget* m_componentListContents;
    QVBoxLayout* m_rowLayout;
    QLineEdit* m_selectedEntityDisplayNameWidget;
    QCheckBox* m_editorOnlyCheckbox; //!< Checkbox associated with the value of the selected entities' "editor only component" value

    QAction* m_actionToAddComponents;
    QAction* m_actionToDeleteComponents;
    QAction* m_actionToCutComponents;
    QAction* m_actionToCopyComponents;
    QAction* m_actionToPasteComponents;

    // We require an overlay widget to act as a canvas to draw on top of everything in the properties pane
    // so that we can draw outside of the component editors' bounds
    friend class PropertyContainerOverlay;
    class PropertyContainerOverlay* m_overlay = nullptr;

    using ComponentPropertyEditorMap = AZStd::unordered_map<AZ::Uuid, AZStd::vector<AzToolsFramework::ComponentEditor*>>;
    ComponentPropertyEditorMap m_componentEditorsByType;

    using ComponentEditorVector = AZStd::vector<AzToolsFramework::ComponentEditor*>;
    ComponentEditorVector m_componentEditors; // list of component editors in order shown
    AZ::s32 m_componentEditorLastSelectedIndex;

    bool m_selectionHasChanged;
    AZStd::vector<AZ::EntityId> m_selectedEntities;

    bool m_isCanvasSelected;

    // Pointer to entity that first entity is compared against for the purpose of rendering deltas vs. slice in the property grid.
    AZStd::unique_ptr<AZ::Entity> m_compareToEntity;

    // Global app serialization context, cached for internal usage during the life of the control.
    AZ::SerializeContext* m_serializeContext;
};
