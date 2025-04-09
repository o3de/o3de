/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiEditorBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Slice/SliceBus.h>
#include <AzCore/std/containers/vector.h>

#include "UiSerialize.h"
#include <LyShine/UiComponentTypes.h>

#include <AzCore/std/containers/intrusive_slist.h>

class UiCanvasComponent;
class UiTransform2dComponent;
class UiRenderInterface;
class UiRenderControlInterface;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiElementComponent
    : public AZ::Component
    , public UiElementBus::Handler
    , public UiEditorBus::Handler
    , public AZ::SliceEntityHierarchyRequestBus::Handler
    , public AZ::EntityBus::Handler
    , public AZStd::intrusive_slist_node<UiElementComponent>
{
    friend class UiCanvasComponent;

public: // types

    // used to map old EntityId's to new EntityId's when generating new ids for a paste or prefab
    typedef LyShine::EntityIdMap EntityIdMap;

public: // member functions

    AZ_COMPONENT(UiElementComponent, LyShine::UiElementComponentUuid, AZ::Component, AZ::SliceEntityHierarchyInterface);

    //! Construct an uninitialized element component
    UiElementComponent();
    ~UiElementComponent() override;

    // UiElementInterface

    void RenderElement(LyShine::IRenderGraph* renderGraph, bool isInGame) override;

    LyShine::ElementId GetElementId() override;
    LyShine::NameType GetName() override;

    AZ::EntityId GetCanvasEntityId() override;
    AZ::Entity* GetParent() override;
    AZ::EntityId GetParentEntityId() override;

    int GetNumChildElements() override;
    AZ::Entity* GetChildElement(int index) override;
    AZ::EntityId GetChildEntityId(int index) override;
    UiElementInterface* GetChildElementInterface(int index) override;

    int GetIndexOfChild(const AZ::Entity* child) override;
    int GetIndexOfChildByEntityId(AZ::EntityId childId) override;

    LyShine::EntityArray GetChildElements() override;
    AZStd::vector<AZ::EntityId> GetChildEntityIds() override;

    AZ::Entity* CreateChildElement(const LyShine::NameType& name) override;
    void DestroyElement() override;
    void DestroyElementOnFrameEnd() override;
    void Reparent(AZ::Entity* newParent, AZ::Entity* insertBefore = nullptr) override;
    void ReparentByEntityId(AZ::EntityId newParent, AZ::EntityId insertBefore) override;
    void AddToParentAtIndex(AZ::Entity* newParent, int index = -1) override;
    void RemoveFromParent() override;

    AZ::Entity* FindFrontmostChildContainingPoint(AZ::Vector2 point, bool isInGame) override;
    LyShine::EntityArray FindAllChildrenIntersectingRect(const AZ::Vector2& bound0, const AZ::Vector2& bound1, bool isInGame) override;

    AZ::EntityId FindInteractableToHandleEvent(AZ::Vector2 point) override;
    AZ::EntityId FindParentInteractableSupportingDrag(AZ::Vector2 point) override;

    AZ::Entity* FindChildByName(const LyShine::NameType& name) override;
    AZ::Entity* FindDescendantByName(const LyShine::NameType& name) override;
    AZ::EntityId FindChildEntityIdByName(const LyShine::NameType& name) override;
    AZ::EntityId FindDescendantEntityIdByName(const LyShine::NameType& name) override;
    AZ::Entity* FindChildByEntityId(AZ::EntityId id) override;
    AZ::Entity* FindDescendantById(LyShine::ElementId id) override;
    void FindDescendantElements(AZStd::function<bool(const AZ::Entity*)> predicate, LyShine::EntityArray& result) override;
    void CallOnDescendantElements(AZStd::function<void(const AZ::EntityId)> callFunction) override;

    bool IsAncestor(AZ::EntityId id) override;

    bool IsEnabled() override;
    void SetIsEnabled(bool isEnabled) override;

    bool GetAreElementAndAncestorsEnabled() override;

    bool IsRenderEnabled() override;
    void SetIsRenderEnabled(bool isRenderEnabled) override;

    // ~UiElementInterface

    // UiEditorInterface
    //! The UiElementComponent implements the editor interface in order to store the state with the element on save
    bool GetIsVisible() override;
    void SetIsVisible(bool isVisible) override;
    bool GetIsSelectable() override;
    void SetIsSelectable(bool isSelectable) override;
    bool GetIsSelected() override;
    void SetIsSelected(bool isSelected) override;
    bool GetIsExpanded() override;
    void SetIsExpanded(bool isExpanded) override;
    bool AreAllAncestorsVisible() override;
    // ~UiEditorInterface

    // EntityEvents
    void OnEntityActivated(const AZ::EntityId&) override;
    void OnEntityDeactivated(const AZ::EntityId&) override;
    // ~EntityEvents

    void AddChild(AZ::Entity* child, AZ::Entity* insertBefore = nullptr);
    void RemoveChild(AZ::Entity* child);
    void RemoveChild(AZ::EntityId child);

    //! Only to be used by UiCanvasComponent when creating the root element
    void SetCanvas(UiCanvasComponent* canvas, LyShine::ElementId elementId);

    //! Only to be used by UiCanvasComponent when loading, cloning etc
    bool FixupPostLoad(AZ::Entity* entity, UiCanvasComponent* canvas, AZ::Entity* parent, bool makeNewElementIds);

    //! Get the cached UiTransform2dComponent pointer (for optimization)
    UiTransform2dComponent* GetTransform2dComponent() const;

    //! Get the cached UiElementComponent pointer for the parent (for optimization)
    UiElementComponent* GetParentElementComponent() const;

    //! Get the cached UiElementComponent pointer for the child (for optimization)
    UiElementComponent* GetChildElementComponent(int index) const;

    //! Get the cached UiCanvasComponent pointer for the canvas this element belongs to (for optimization)
    UiCanvasComponent* GetCanvasComponent() const;

    // Used to check that FixupPostLoad has been called
    bool IsFullyInitialized() const;

    // Used to check that cached child pointers are setup
    bool AreChildPointersValid() const;

    // SliceEntityHierarchyRequestBus
    AZ::EntityId GetSliceEntityParentId() override;
    AZStd::vector<AZ::EntityId> GetSliceEntityChildren() override;
    // ~SliceEntityHierarchyRequestBus

public: // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiElementService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiElementService"));
    }

    static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    static void Reflect(AZ::ReflectContext* context);

    static void Initialize();

    //! Helper function used during conversion of old format canvas files. Called from
    //! UiCanvasFileObject::VersionConverter and PrefabFileObject::VersionConverter.
    //! In the old format child entities were referenced by Entity* rather than EntityId so each entity
    //! had all of its children nested under it in the file. In the newer format, that was introduced when
    //! slice support was added, the entities are owned by the root slice and referenced by entity id.
    //! An index of -1 is used when this is called on the root element of the canvas, otherwise index
    //! is the index of the child entity withing the children container.
    static bool MoveEntityAndDescendantsToListAndReplaceWithEntityId(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& elementNode,
        int index,
        AZStd::vector<AZ::SerializeContext::DataElementNode>& entities);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

private: // member functions

    //! Send out notifications to elements whose "effective" enabled state has changed
    void DoRecursiveEnabledNotification(bool newIsEnabledValue);

private: //types

    //! ChildOrderSerializationEvents intercepts the serialization events for patching.
    //! This allows us to do some fixup after patching is done on a UiElementComponent.
    class ChildOrderSerializationEvents
        : public AZ::SerializeContext::IEventHandler
    {
        //! Called right after we finish writing data to the instance pointed at by classPtr.
        void OnPatchEnd(void* classPtr, const AZ::DataPatchNodeInfo& patchInfo) override
        {
            UiElementComponent* component = reinterpret_cast<UiElementComponent*>(classPtr);
            component->OnPatchEnd(patchInfo);
        }
    };

    //! ChildEntityIdOrderEntry stores the entity id and the sort index (which is the absolute sort index relative to
    //! the other entries, 0 is the first, 1 is the second, so on). We serialize out the order data in this fashion
    //! because the slice data patching system will traditionally use the vector index to know what data goes where.
    //! In the case of this data, it does not make sense to data patch by vector index since the underlying data may
    //! have changed and the data patch will create duplicate or incorrect data. The slice data patch system has the
    //! concept of a "Persistent ID" which can be used instead such that data patches will try to match persistent
    //! ids which can be identified regardless of vector index. In this way, our vector order no longer matters and
    //! the EntityId is now the identifier which the data patcher will use to update the sort index.

    struct ChildEntityIdOrderEntry
    {
        AZ_TYPE_INFO(ChildEntityIdOrderEntry, "{D6F3CC55-6C7C-4D64-818F-FA3378EC8DA2}");
        AZ::EntityId m_entityId;
        AZ::u64 m_sortIndex;

        bool operator==(const ChildEntityIdOrderEntry& rhs) const
        {
            return m_entityId == rhs.m_entityId && m_sortIndex == rhs.m_sortIndex;
        }

        bool operator!=(const ChildEntityIdOrderEntry& rhs) const
        {
            return m_entityId != rhs.m_entityId || m_sortIndex != rhs.m_sortIndex;
        }

        bool operator<(const ChildEntityIdOrderEntry& rhs) const
        {
            return m_sortIndex < rhs.m_sortIndex || (m_sortIndex == rhs.m_sortIndex && m_entityId < rhs.m_entityId);
        }
    };
    using ChildEntityIdOrderArray = AZStd::vector<ChildEntityIdOrderEntry>;

private: // member functions

    // Display a warning that the component is not yet fully initialized
    void EmitNotInitializedWarning() const;

    // helper function for setting the multiple parent reference that we store
    void SetParentReferences(AZ::Entity* parent, UiElementComponent* parentElementComponent);

    //! Ensures m_childEntityIdOrder is updated for any data patches to the old m_children
    void OnPatchEnd(const AZ::DataPatchNodeInfo& patchInfo);

    //! Recalculate the sort indices in m_childEntityIdOrder to match the order in the vector
    void ResetChildEntityIdSortOrders();

    //! Destroys children of UiElement, removes UiElement from parent, and sends OnUiElementBeingDestroyed
    void PrepareElementForDestroy();

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

    //! Destroy UI element entity
    static void DestroyElementEntity(AZ::EntityId entityId);

private: // data

    AZ_DISABLE_COPY_MOVE(UiElementComponent);

    LyShine::ElementId m_elementId = 0;

    AZ::Entity* m_parent = nullptr;
    AZ::EntityId m_parentId;    // Stored in order to do error checking when m_parent could have been deleted
    UiCanvasComponent* m_canvas = nullptr;    // currently we store a pointer to the canvas component rather than an entity ID

    //! Pointers directly to components that are cached for performance to avoid ebus use in critical paths
    UiElementComponent* m_parentElementComponent = nullptr;
    UiTransform2dComponent* m_transformComponent = nullptr;
    AZStd::vector<UiElementComponent*> m_childElementComponents;
    UiRenderInterface* m_renderInterface = nullptr;
    UiRenderControlInterface* m_renderControlInterface = nullptr;

    bool m_isEnabled = true;
    bool m_isRenderEnabled = true;

    // this data is only relevant when running in the editor, it is accessed through UiEditorBus
    bool m_isVisibleInEditor = true;
    bool m_isSelectableInEditor = true;
    bool m_isSelectedInEditor = false;
    bool m_isExpandedInEditor = true;

    // New children array that uses persistent IDs. Required because slices/datapatches do not handle things well
    // for the old m_children because it doesn't use persistent IDs.
    // Note: once loaded and patched this vector is always in the correct order and the sort indices start at zero
    // and are contiguous. OnPatchEnd enforces this after any patching.
    ChildEntityIdOrderArray m_childEntityIdOrder;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Inline method implementations
////////////////////////////////////////////////////////////////////////////////////////////////////

inline UiTransform2dComponent* UiElementComponent::GetTransform2dComponent() const
{
    AZ_Assert(m_transformComponent, "UiElementComponent: m_transformComponent used when not initialized");
    return m_transformComponent;
}

inline UiElementComponent* UiElementComponent::GetParentElementComponent() const
{
    AZ_Assert(m_parentElementComponent || !m_parent, "UiElementComponent: m_parentElementComponent used when not initialized");
    return m_parentElementComponent;
}

inline UiElementComponent* UiElementComponent::GetChildElementComponent(int index) const
{
    AZ_Assert(index >= 0 && index < m_childElementComponents.size(), "UiElementComponent: index to m_childElementComponents out of bounds");
    AZ_Assert(m_childElementComponents[index], "UiElementComponent: m_childElementComponents used when not initialized");
    return m_childElementComponents[index];
}

inline UiCanvasComponent* UiElementComponent::GetCanvasComponent() const
{
    AZ_Assert(m_canvas, "UiElementComponent: m_canvas used when not initialized");
    return m_canvas;
}

inline bool UiElementComponent::IsFullyInitialized() const
{
    return (m_canvas && m_transformComponent && AreChildPointersValid());
}

inline bool UiElementComponent::AreChildPointersValid() const
{
    if (m_childElementComponents.size() == m_childEntityIdOrder.size())
    {
        return true;
    }

    AZ_Assert(m_childElementComponents.empty(), "Cached child pointers exist but are a different size to m_children");

    return false;
}
