/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef ENTITY_PROPERTY_EDITOR_H
#define ENTITY_PROPERTY_EDITOR_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsoleTypes.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/API/EntityPropertyEditorNotificationBus.h>
#include <AzToolsFramework/API/EntityPropertyEditorRequestsBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityBus.h>
#include <AzToolsFramework/FocusMode/FocusModeNotificationBus.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponentBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

#include <AzQtComponents/Components/O3DEStylesheet.h>

#include <QComboBox>
#include <QtGui/QIcon>
#include <QtWidgets/QWidget>
#endif

class QLabel;
class QMenu;
class QMimeData;
class QSpacerItem;
class QStandardItem;

namespace Ui
{
    class EntityPropertyEditorUI;
}

namespace AZ
{
    struct ClassDataReflection;
    class Component;
    class Entity;
}

namespace AzToolsFramework
{
    class ComponentEditor;
    class ComponentPaletteWidget;
    class ComponentModeCollectionInterface;
    class ContainerEntityInterface;
    class FocusModeInterface;
    class MenuManagerInterface;
    class ReadOnlyEntityPublicInterface;
    struct SourceControlFileInfo;
 
    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
    }

    namespace Prefab
    {
        class PrefabPublicInterface;
        class InstanceUpdateExecutorInterface;
    };

    namespace UndoSystem
    {
        class URSequencePoint;
    }

    using ProductCallback = AZStd::function<void(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*)>;

    using ComponentEditorVector = AZStd::vector<ComponentEditor*>;

    /**
     * the entity property editor shows all components for a given entity or set of entities.
     * it displays their values and lets you edit them.  The editing actually happens through the sub editor parts, though.
     * only components which the selected entities have in common are displayed (if theres more than one)
     * if there are components that are not in common, there is a message indicating that this is the case.
     * each component is shown as a heading which can be expanded into an actual component specific property editor.
     * so this widget is actually only interested in specifically what entities are selected, what their components are,
     * and what is in common.
     */
    class EntityPropertyEditor
        : public QWidget
        , private ToolsApplicationEvents::Bus::Handler
        , public IPropertyEditorNotify
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , public AzToolsFramework::EntityPropertyEditorNotificationBus::Handler
        , public AzToolsFramework::EntityPropertyEditorRequestBus::Handler
        , public AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler
        , private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
        , public EditorInspectorComponentNotificationBus::MultiHandler
        , private ComponentModeFramework::EditorComponentModeNotificationBus::Handler
        , public AZ::EntitySystemBus::Handler
        , public AZ::TickBus::Handler
        , private EditorWindowUIRequestBus::Handler
        , private ReadOnlyEntityPublicNotificationBus::Handler
        , public FocusModeNotificationBus::Handler
    {
        Q_OBJECT;
    public:

        AZ_CLASS_ALLOCATOR(EntityPropertyEditor, AZ::SystemAllocator)

        enum class ReorderState
        {
            Inactive, // No row widget reordering operation is in progress.
            DraggingComponent, // User is dragging a component editor.
            DraggingRowWidget, // User is dragging a row widget around.
            UsingMenu, // User has the context menu open and may hover over a move up/down operation.
            MenuOperationInProgress, // User has selected a move/up down menu item.
            WaitForRedraw, // Wait for rebuild of RPE.
            HighlightMovedRow // User has moved a row, highlight the new position.
        };

        enum class DropArea
        {
            Above,
            Below
        };

        EntityPropertyEditor(QWidget* pParent = NULL, Qt::WindowFlags flags = Qt::WindowFlags(), bool isLevelEntityEditor = false);
        virtual ~EntityPropertyEditor();

        void BeforeUndoRedo() override;
        void AfterUndoRedo() override;

        static void Reflect(AZ::ReflectContext* context);

        // implementation of IPropertyEditorNotify:

        // CALLED FOR UNDO PURPOSES
        void BeforePropertyModified(InstanceDataNode* pNode) override;
        void AfterPropertyModified(InstanceDataNode* pNode) override;
        void SetPropertyEditingActive(InstanceDataNode* pNode) override;
        void SetPropertyEditingComplete(InstanceDataNode* pNode) override;
        void SealUndoStack() override;

        // FocusModeNotificationBus overrides ...
        void OnEditorFocusChanged(AZ::EntityId previousFocusEntityId, AZ::EntityId newFocusEntityId) override;

        // Context menu population for entity component properties.
        void RequestPropertyContextMenu(InstanceDataNode* node, const QPoint& globalPos) override;

        /// Set filter for what appears in the "Add Components" menu.
        void SetAddComponentMenuFilter(ComponentFilter componentFilter);

        void SetAllowRename(bool allowRename);

        void SetOverrideEntityIds(const AzToolsFramework::EntityIdSet& entities);

        void SetSystemEntityEditor(bool isSystemEntityEditor);

        bool IsLockedToSpecificEntities() const { return !m_overrideSelectedEntityIds.empty(); }

        static bool AreComponentsCopyable(AZStd::span<AZ::Component* const> components, const ComponentFilter& filter);

        ReorderState GetReorderState() const;
        ComponentEditor* GetEditorForCurrentReorderRowWidget() const;
        PropertyRowWidget* GetReorderRowWidget() const;
        PropertyRowWidget* GetReorderDropTarget() const;
        DropArea GetReorderDropArea() const;
        QPixmap GetReorderRowWidgetImage() const;
        float GetMoveIndicatorAlpha() const;
        PropertyRowWidget* GetRowToHighlight();

    Q_SIGNALS:
        void SelectedEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name);

    protected:
        virtual void CloseInspectorWindow();

        virtual QString GetEntityDetailsLabelText() const;

    private:
        bool m_disabled = false;

        struct SharedComponentInfo
        {
            SharedComponentInfo(AZ::Component* component);
            AZ::Entity::ComponentArrayType m_instances;
        };

        using SharedComponentArray = AZStd::vector<SharedComponentInfo>;

        //////////////////////////////////////////////////////////////////////////
        // ToolsApplicationEvents::Bus::Handler
        void BeforeEntitySelectionChanged() override;
        void AfterEntitySelectionChanged(
            const AzToolsFramework::EntityIdList& newlySelectedEntities,
            const AzToolsFramework::EntityIdList& newlyDeselectedEntities) override;
        void InvalidatePropertyDisplay(PropertyModificationRefreshLevel level) override;
        void InvalidatePropertyDisplayForComponent(AZ::EntityComponentIdPair componentId, PropertyModificationRefreshLevel level) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        /// AzToolsFramework::EditorEntityContextNotificationBus overrides ...
        void OnStartPlayInEditor() override;
        void OnStopPlayInEditor() override;
        void OnPrepareForContextReset() override;
        //////////////////////////////////////////////////////////////////////////

        // PropertyEditorEntityChangeNotificationBus overrides ...
        void OnEntityComponentPropertyChanged(AZ::ComponentId /*componentId*/) override;

        // EditorInspectorComponentNotificationBus overides ...
        void OnComponentOrderChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::EntitySystemBus::Handler
        void OnEntityInitialized(const AZ::EntityId&) override;
        void OnEntityDestroyed(const AZ::EntityId&) override;
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        void OnEntityDeactivated(const AZ::EntityId& entityId) override;
        void OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name) override;
        void OnEntityStartStatusChanged(const AZ::EntityId& entityId) override;
        //////////////////////////////////////////////////////////////////////////

        // ViewportEditorModeNotificationsBus overrides ...
        void OnEditorModeActivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;
        void OnEditorModeDeactivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;

        // EditorComponentModeNotificationBus overrides ...
        void ActiveComponentModeChanged(const AZ::Uuid& componentType) override;

        // EntityPropertEditorRequestBus overrides ...
        void GetSelectedAndPinnedEntities(EntityIdList& selectedEntityIds) override;
        void GetSelectedEntities(EntityIdList& selectedEntityIds) override;
        void GetSelectedComponents(AZStd::unordered_set<AZ::EntityComponentIdPair>& selectedComponentEntityIds) override;
        void SetNewComponentId(AZ::ComponentId componentId) override;
        void VisitComponentEditors(const VisitComponentEditorsCallback& callback) const override;

        // EntityPropertyEditorNotificationBus overrides ...
        void OnComponentSelectionChanged(
            EntityPropertyEditor* entityPropertyEditor,
            const AZStd::unordered_set<AZ::EntityComponentIdPair>& selectedEntityComponentIds) override;

        // TickBus overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // EditorWindowRequestBus overrides ...
        void SetEditorUiEnabled(bool enable) override;

        // ReadOnlyEntityPublicNotificationBus overrides ...
        void OnReadOnlyEntityStatusChanged(const AZ::EntityId& entityId, bool readOnly) override;

        bool IsEntitySelected(const AZ::EntityId& id) const;
        bool IsSingleEntitySelected(const AZ::EntityId& id) const;

        void GotSceneSourceControlStatus(AzToolsFramework::SourceControlFileInfo& fileInfo) override;
        void PerformActionsBasedOnSceneStatus(bool sceneIsNew, bool readOnly) override;

        // enable/disable editor
        void EnableEditor(bool enabled);

        void MarkPropertyEditorBusyStart();
        void MarkPropertyEditorBusyEnd();

        void QueuePropertyRefresh();
        void ClearInstances(bool invalidateImmediately = true);

        void GetAllComponentsForEntityInOrder(const AZ::Entity* entity, AZ::Entity::ComponentArrayType& componentsOnEntity);
        void BuildSharedComponentArray(SharedComponentArray& sharedComponentArray, bool containsLayerEntity);
        void BuildSharedComponentUI(SharedComponentArray& sharedComponentArray);
        bool ComponentMatchesCurrentFilter(SharedComponentInfo& sharedComponentInfo) const;
        ComponentEditor* CreateComponentEditor();
        void UpdateEntityIcon();
        void UpdateEntityDisplay();
        static bool DoesComponentPassFilter(const AZ::Component* component, const ComponentFilter& filter);
        bool AreComponentsRemovable(AZStd::span<AZ::Component* const> components) const;
        bool AllowAnyComponentModification() const;
        bool AreComponentsDraggable(AZStd::span<AZ::Component* const> components) const;
        bool AreComponentsCopyable(AZStd::span<AZ::Component* const> components) const;

        void AddMenuOptionsForComponents(QMenu& menu, const QPoint& position);
        void AddMenuOptionsForFields(InstanceDataNode* fieldNode, InstanceDataNode* componentNode, const AZ::SerializeContext::ClassData* componentClassData, QMenu& menu);

        bool HasAnyVisibleElements(const InstanceDataNode& node);

        void GenerateRowWidgetIndexMapToChildIndex(PropertyRowWidget* parent, int destIndex);
        void ContextMenuActionMoveItemUp(ComponentEditor* componentEditor, PropertyRowWidget* rowWidget);
        void ContextMenuActionMoveItemDown(ComponentEditor* componentEditor, PropertyRowWidget* rowWidget);

        /// Given an InstanceDataNode, calculate a DataPatch address relative to the entity.
        /// @return true if successful.
        bool GetEntityDataPatchAddress(const InstanceDataNode* componentFieldNode, AZ::DataPatch::AddressType& dataPatchAddressOut, AZ::EntityId* entityIdOut = nullptr) const;

        enum class AddressRootType
        {
            RootAtComponentsContainer,  // Address will be made rooted at the entity's components container (from "Components" container down).
            RootAtEntity,               // Address will be made rooted at the entity itself (from "AZ::Entity" down).
        };

        // Input address is expected to be relative to a component, as each property editor corresponds to a component and its own data hierarchy.
        // \param componentFieldNode instance data hierarchy node corresponding to a field under a component, as provided by the component's ReflectedPropertyEditor.
        // \param rootType type of root the address should be adjusted to. See \ref AddressRootType.
        // \param outAddress output parameter to be populated with the adjusted node address.
        void CalculateAndAdjustNodeAddress(const InstanceDataNode& componentFieldNode, AddressRootType rootType, InstanceDataNode::Address& outAddress) const;

        // Custom function for determining whether InstanceDataNodes are read-only
        bool QueryInstanceDataNodeReadOnlyStatus(const InstanceDataNode* node);
        bool QueryInstanceDataNodeReadOnlySetStatus(const InstanceDataNode* node);

        // Custom function for determining whether InstanceDataNodes are hidden
        bool QueryInstanceDataNodeHiddenStatus(const InstanceDataNode* node);
        bool QueryInstanceDataNodeHiddenSetStatus(const InstanceDataNode* node);

        bool QueryInstanceDataNodeSetStatus(const InstanceDataNode* node, AZ::DataPatch::Flag testFlag);
        bool QueryInstanceDataNodeEffectStatus(const InstanceDataNode* node, AZ::DataPatch::Flag testFlag);

        const char* GetAppropriateIndicator(const InstanceDataNode* node);

        void OnDisplayComponentEditorMenu(const QPoint& position);
        void OnRequestRequiredComponents(const QPoint& position,
            const QSize& size,
            AZStd::span<const AZ::ComponentServiceType> services,
            AZStd::span<const AZ::ComponentServiceType> incompatibleServices);

        AZ::Component* ExtractMatchingComponent(AZ::Component* component, AZ::Entity::ComponentArrayType& availableComponents);

        void SetEntityIconToDefault();
        void PopupAssetBrowserForEntityIcon();

        void HideComponentPalette();
        void ShowComponentPalette(
            ComponentPaletteWidget* componentPalette,
            const QPoint& position,
            const QSize& size,
            AZStd::span<const AZ::ComponentServiceType> serviceFilter,
            AZStd::span<const AZ::ComponentServiceType> incompatibleServiceFilter);

        enum class SelectionEntityTypeInfo
        {
            None,
            OnlyStandardEntities,
            OnlyLayerEntities,
            OnlyPrefabEntities,
            Mixed,
            LevelEntity,
            ContainerEntityOfFocusedPrefab
        };
        /**
         * Returns what kinds of entities are in the current selection. This is used because mixed selection
         * and layer entities requires special logic in the property editor.
         * \param selection The selected entities.
         */
        SelectionEntityTypeInfo GetSelectionEntityTypeInfo(const EntityIdList& selection) const;

        /**
         * Returns true if a selection matching the passed in selection information allows components to be added.
         */
        bool CanAddComponentsToSelection(const SelectionEntityTypeInfo& selectionEntityTypeInfo) const;

        AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
        QVector<QAction*> m_entityComponentActions;
        AZ_POP_DISABLE_WARNING
        QAction* m_actionToAddComponents = nullptr;
        QAction* m_actionToDeleteComponents = nullptr;
        QAction* m_actionToCutComponents = nullptr;
        QAction* m_actionToCopyComponents = nullptr;
        QAction* m_actionToPasteComponents = nullptr;
        QAction* m_actionToEnableComponents = nullptr;
        QAction* m_actionToDisableComponents = nullptr;
        QAction* m_actionToMoveComponentsUp = nullptr;
        QAction* m_actionToMoveComponentsDown = nullptr;
        QAction* m_actionToMoveComponentsTop = nullptr;
        QAction* m_actionToMoveComponentsBottom = nullptr;

        AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;

        void CreateActions();
        void UpdateActions();

        bool CanPasteComponentsOnSelectedEntities() const;
        bool CanPasteComponentsOnEntity(const ComponentTypeMimeData::ClassDataContainer& classDataForComponentsToPaste, const AZ::Entity* entity) const;

        AZ::Entity::ComponentArrayType GetCopyableComponents() const;
        void DeleteComponents(AZStd::span<AZ::Component* const> components);
        void DeleteComponents();
        void CutComponents();
        void CopyComponents();
        void PasteComponents();
        void EnableComponents(AZStd::span<AZ::Component* const> components);
        void EnableComponents();
        void DisableComponents(AZStd::span<AZ::Component* const> components);
        void DisableComponents();
        void MoveComponentsUp();
        void MoveComponentsDown();
        void MoveComponentsTop();
        void MoveComponentsBottom();

        //component reorder and drag drop helpers

        //reorder source before target
        void MoveComponentBefore(const AZ::Component* sourceComponent, const AZ::Component* targetComponent, ScopedUndoBatch &undo);

        //reorder source after target
        void MoveComponentAfter(const AZ::Component* sourceComponent, const AZ::Component* targetComponent, ScopedUndoBatch &undo);

        //reorder each element of source before corresponding element of target
        void MoveComponentRowBefore(AZStd::span<AZ::Component* const> sourceComponents, AZStd::span<AZ::Component* const> targetComponents, ScopedUndoBatch &undo);

        //reorder each element of source after corresponding element of target
        void MoveComponentRowAfter(AZStd::span<AZ::Component* const> sourceComponents, AZStd::span<AZ::Component* const> targetComponents, ScopedUndoBatch &undo);

        //determine if any neighboring component editor rows can be exchanged
        bool IsMoveAllowed(const ComponentEditorVector& componentEditors) const;

        //determine if the specified component editor rows can be exchanged
        bool IsMoveAllowed(const ComponentEditorVector& componentEditors, size_t sourceComponentEditorIndex, size_t targetComponentEditorIndex) const;

        bool IsMoveComponentsUpAllowed() const;
        bool IsMoveComponentsDownAllowed() const;

        bool DoesOwnFocus() const;
        AZ::u32 GetHeightOfRowAndVisibleChildren(const PropertyRowWidget* row) const;
        QRect GetWidgetAndVisibleChildrenGlobalRect(const PropertyRowWidget* widget) const;
        PropertyRowWidget* GetRowWidgetAtSameLevelAfter(PropertyRowWidget* widget) const;
        PropertyRowWidget* GetRowWidgetAtSameLevelBefore(PropertyRowWidget* widget) const;
        QRect GetWidgetGlobalRect(const QWidget* widget) const;
        bool DoesIntersectWidget(const QRect& globalRect, const QWidget* widget) const;
        bool DoesIntersectSelectedComponentEditor(const QRect& globalRect) const;
        bool DoesIntersectNonSelectedComponentEditor(const QRect& globalRect) const;

        void ClearComponentEditorDragging();
        void ClearComponentEditorSelection();
        void SelectRangeOfComponentEditors(const AZ::s32 index1, const AZ::s32 index2, bool selected = true);
        void SelectIntersectingComponentEditors(const QRect& globalRect, bool selected = true);
        bool SelectIntersectingComponentEditorsSafe(const QRect& globalRect);
        void ToggleIntersectingComponentEditors(const QRect& globalRect);
        AZ::s32 GetComponentEditorIndex(const ComponentEditor* componentEditor) const;
        AZ::s32 GetComponentEditorIndexFromType(const AZ::Uuid& componentType) const;
        ComponentEditorVector GetIntersectingComponentEditors(const QRect& globalRect) const;

        const ComponentEditorVector& GetSelectedComponentEditors() const;
        AZStd::span<AZ::Component* const> GetSelectedComponents() const;
        const AZStd::unordered_map<AZ::EntityId, AZ::Entity::ComponentArrayType>& GetSelectedComponentsByEntityId() const;
        void UpdateSelectionCache();
        void NotifySelectionChanges();

        ComponentEditorVector m_selectedComponentEditors;
        AZ::Entity::ComponentArrayType m_selectedComponents;
        AZStd::unordered_set<AZ::EntityComponentIdPair> m_selectedEntityComponentIds;
        AZStd::unordered_map<AZ::EntityId, AZ::Entity::ComponentArrayType> m_selectedComponentsByEntityId;

        void SaveComponentEditorState();
        void SaveComponentEditorState(ComponentEditor* componentEditor);
        void LoadComponentEditorState();
        void ClearComponentEditorState();

        struct ComponentEditorSaveState
        {
            bool m_selected = false;
        };
        AZStd::unordered_map<AZ::ComponentId, ComponentEditorSaveState> m_componentEditorSaveStateTable;

        void UpdateOverlay();
        void UpdateOverrideVisualization(ComponentEditor& componentEditor);

        friend class EntityPropertyEditorOverlay;
        class EntityPropertyEditorOverlay* m_overlay = nullptr;

        //widget overrides
        void resizeEvent(QResizeEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        bool eventFilter(QObject* object, QEvent* event) override;

        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;
        void dropEvent(QDropEvent* event) override;

        bool HandleSelectionEvents(QObject* object, QEvent* event);
        bool m_selectionEventAccepted;

        bool HandleMenuEvent(QObject* object, QEvent* event);

        // drag and drop events
        QRect GetInflatedRectFromPoint(const QPoint& point, int radius) const;
        bool GetComponentsAtDropEventPosition(QDropEvent* event, AZ::Entity::ComponentArrayType& targetComponents);
        bool IsDragAllowed(const ComponentEditorVector& componentEditors) const;
        bool IsDropAllowed(const QMimeData* mimeData, const QPoint& posGlobal) const;
        bool IsDropAllowedForComponentReorder(const QMimeData* mimeData, const QPoint& posGlobal) const;
        bool CreateComponentWithAsset(const AZ::Uuid& componentType, const AZ::Data::AssetId& assetId, AZ::Entity::ComponentArrayType& createdComponents);

        // given mimedata, filter it down to only the entries that can actually be spawned in this context.
        void GetCreatableAssetEntriesFromMimeData(const QMimeData* mimeData, ProductCallback callbackFunction) const;

        ComponentEditor* GetReorderDropTarget(const QRect& globalRect) const;
        bool ResetDrag(QMouseEvent* event);
        bool FindAllowedRowWidgetReorderDropTarget(const QPoint& globalPos);
        bool UpdateRowWidgetDrag(const QPoint& localPos, Qt::MouseButtons mouseButtons, const QMimeData* mimeData);
        PropertyRowWidget* FindPropertyRowWidgetAt(QPoint globalPos);
        bool UpdateDrag(const QPoint& localPos, Qt::MouseButtons mouseButtons, const QMimeData* mimeData);
        bool StartDrag(QMouseEvent* event);
        void EndRowWidgetReorder();
        bool HandleDrop(QDropEvent* event);
        bool HandleDropForComponentTypes(QDropEvent* event);
        bool HandleDropForComponentAssets(QDropEvent* event);
        bool HandleDropForAssetBrowserEntries(QDropEvent* event);
        bool HandleDropForComponentReorder(QDropEvent* event);
        bool CanDropForComponentTypes(const QMimeData* mimeData) const;
        bool CanDropForComponentAssets(const QMimeData* mimeData) const;
        bool CanDropForAssetBrowserEntries(const QMimeData* mimeData) const;
        void SetRowWidgetHighlighted(PropertyRowWidget* rowWidget);

        AZStd::vector<AZ::s32> ExtractComponentEditorIndicesFromMimeData(const QMimeData* mimeData) const;
        ComponentEditorVector GetComponentEditorsFromIndices(const AZStd::vector<AZ::s32>& indices) const;
        ComponentEditor* GetComponentEditorsFromIndex(const AZ::s32 index) const;
        QPoint m_dragStartPosition;
        bool m_dragStarted;

        void ResetAutoScroll();
        void QueueAutoScroll();
        void UpdateAutoScroll();
        int m_autoScrollCount;
        int m_autoScrollMargin;
        bool m_autoScrollQueued;

        void UpdateInternalState();

        enum StatusType
        {
            StatusStartActive,
            StatusStartInactive,
            StatusEditorOnly,
            StatusItems
        };

        QStringList m_itemNames;

        void UpdateStatusComboBox();
        size_t StatusTypeToIndex(StatusType statusType) const;
        StatusType IndexToStatusType(size_t index) const;

        bool m_isBuildingProperties;

        Ui::EntityPropertyEditorUI* m_gui;

        // Global app serialization context, cached for internal usage during the life of the control.
        AZ::SerializeContext* m_serializeContext;

        AZ::s32 m_componentEditorLastSelectedIndex;
        AZ::s32 m_componentEditorsUsed;
        ComponentEditorVector m_componentEditors;

        using ComponentPropertyEditorMap = AZStd::unordered_map<AZ::Component*, ComponentEditor*>;
        ComponentPropertyEditorMap m_componentToEditorMap;

        ComponentPaletteWidget* m_componentPalette;

        AzToolsFramework::UndoSystem::URSequencePoint* m_currentUndoOperation;
        InstanceDataNode* m_currentUndoNode;

        bool m_sceneIsNew;

        // The busy system tracks when components are being changed, this allows
        // a refresh when the busy counter hits zero, in case multiple things are making
        // changes to an object to mark it as busy.
        int m_propertyEditBusy;

        bool m_isSystemEntityEditor;
        bool m_isLevelEntityEditor = false;

        enum class InspectorLayout
        {
            Entity = 0,                     // All selected entities are regular entities.
            Level,                          // The selected entity is the prefab container entity for the level prefab.
            ContainerEntityOfFocusedPrefab, // The selected entity is the prefab container entity for the focused prefab.
            ContainerEntity,                // The selected entity is the prefab container entity for a prefab that is not the focused prefab.
            Invalid                         // Other entities are selected alongside the level prefab container entity.
        };

        InspectorLayout GetCurrentInspectorLayout() const;

        // the spacer's job is to make sure that its always at the end of the list of components.
        QSpacerItem* m_spacer;
        bool m_isAlreadyQueuedRefresh;
        bool m_shouldScrollToNewComponents;
        bool m_shouldScrollToNewComponentsQueued;

        int m_savedVerticalScroll = -1;
        int m_savedHorizontalScroll = -1;

        AZStd::string m_filterString;

        // IDs of entities currently bound to this property editor.
        EntityIdList m_selectedEntityIds;

        ComponentFilter m_componentFilter;
        // Tracks if a custom filter has been set. A comparison operator is not available for
        // the filter, so it can't be checked if it's the default filter.
        bool m_customFilterSet = false;

        // Temporary buffer to use when calculating a data patch address.
        AZ::DataPatch::AddressType m_dataPatchAddressBuffer;

        QIcon m_emptyIcon;
        QIcon m_clearIcon;
        QIcon m_dragIcon;
        QCursor m_dragCursor;

        QStandardItem* m_comboItems[StatusItems];
        EntityIdSet m_overrideSelectedEntityIds;

        // Prefab interfaces
        Prefab::PrefabPublicInterface* m_prefabPublicInterface = nullptr;
        Prefab::InstanceUpdateExecutorInterface* m_instanceUpdateExecutorInterface = nullptr;

        ReadOnlyEntityPublicInterface* m_readOnlyEntityPublicInterface = nullptr;
        bool m_selectionContainsReadOnlyEntity = false;

        // Reordering row widgets within the RPE.
        static constexpr float MoveFadeSeconds = 0.5f;

        FocusModeInterface* m_focusModeInterface = nullptr;
        ContainerEntityInterface* m_containerEntityInterface = nullptr;

        ReorderState m_currentReorderState = ReorderState::Inactive;
        ComponentEditor* m_reorderRowWidgetEditor = nullptr;
        InstanceDataNode* m_nodeToMove = nullptr;
        PropertyRowWidget* m_reorderRowWidget = nullptr;
        PropertyRowWidget* m_reorderDropTarget = nullptr;
        DropArea m_reorderDropArea = DropArea::Above;
        QPixmap m_reorderRowImage;
        float m_moveFadeSecondsRemaining;
        AZStd::vector<int> m_indexMapOfMovedRow;

        // When m_initiatingPropertyChangeNotification is set to true, it means this EntityPropertyEditor is
        // broadcasting a change to all listeners about a property change for a given entity.  This is needed
        // so that we don't update the values twice for this inspector
        bool m_initiatingPropertyChangeNotification = false;
        void ConnectToEntityBuses(const AZ::EntityId& entityId);
        void DisconnectFromEntityBuses(const AZ::EntityId& entityId);

        void BeginMoveRowWidgetFade();
        void HighlightMovedRowWidget();

        //! Stores a component id to be focused on next time the UI updates.
        AZStd::optional<AZ::ComponentId> m_newComponentId;

        AZ::ConsoleCommandInvokedEvent::Handler m_commandInvokedHandler;

    private slots:
        void OnPropertyRefreshRequired(); // refresh is needed for a property.
        void UpdateContents();
        void OnAddComponent();
        void OnEntityNameChanged();
        void ScrollToNewComponent();
        void QueueScrollToNewComponent();
        void OnStatusChanged(int index);
        void OnSearchContextMenu(const QPoint& pos);
        void BuildEntityIconMenu();
        void OnComponentOverrideContextMenu(const QPoint& position);

        void OnSearchTextChanged();
        void ClearSearchFilter();

        void OpenPinnedInspector();

        void DragStopped();

        AZ::Entity* GetSelectedEntityById(AZ::EntityId& entityId) const;
    };

    void SortComponentsByOrder(AZ::EntityId entityId, AZ::Entity::ComponentArrayType& componentsOnEntity);
    void SaveComponentOrder(AZ::EntityId entityId, AZStd::span<AZ::Component* const> componentsInOrder);

} // namespace AzToolsFramework

class StatusComboBox : public QComboBox
{
    Q_OBJECT
public:
    StatusComboBox(QWidget* parent = nullptr);

    void setHeaderOverride(QString overrideString);
    void setItalic(bool italic);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showPopup() override;
    void wheelEvent(QWheelEvent* e) override;

    QString m_headerOverride = "";
    bool m_italic = false;
};

#endif
