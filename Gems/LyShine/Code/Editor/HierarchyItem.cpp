/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include <AzCore/Asset/AssetManager.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <LyShine/UiComponentTypes.h>

#define UICANVASEDITOR_HIERARCHY_ICON_OPEN                  ":/Icons/Eye_Open.tif"
#define UICANVASEDITOR_HIERARCHY_ICON_OPEN_HIDDEN           ":/Icons/Eye_Open_Hidden.tif"
#define UICANVASEDITOR_HIERARCHY_ICON_OPEN_HOVER            ":/Icons/Eye_Open_Hover.tif"
#define UICANVASEDITOR_HIERARCHY_ICON_PADLOCK_ENABLED_HOVER ":/Icons/Padlock_Enabled_Hover.tif"
#define UICANVASEDITOR_HIERARCHY_ICON_PADLOCK_ENABLED       ":/Icons/Padlock_Enabled.tif"

HierarchyItem::HierarchyItem(EditorWindow* editWindow,
    QTreeWidgetItem& parent,
    int childIndex,
    const QString label,
    AZ::Entity* optionalElement)
    : QObject()
    , QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(label), HierarchyItem::RttiType)
    , m_editorWindow(editWindow)
    , m_elementId(optionalElement ? optionalElement->GetId() : AZ::EntityId())
    , m_mark(false)
    , m_preMoveChildRow(-1)
    , m_mouseIsHovering(false)
    , m_nonSnappedOffsets()
    , m_nonSnappedZRotation(0.0f)
{
    // Add this hierarchy item to its parent at the specified child index
    QTreeWidgetItem* child = static_cast<QTreeWidgetItem*>(this);
    if (childIndex >= 0)
    {
        parent.insertChild(childIndex, child);
    }
    else
    {
        parent.addChild(child);
    }

    // If an optional existing element is specified, we don't need to create a
    // new element. This occurs when building the tree from an existing canvas
    if (!optionalElement)
    {
        // Create a new element as the last child of the specified parent element
        AZ::Entity* element = nullptr;
        HierarchyItem* parentHierarchyItem = HierarchyItem::RttiCast(&parent);
        if (parentHierarchyItem)
        {
            UiElementBus::EventResult(
                element, parentHierarchyItem->GetEntityId(), &UiElementBus::Events::CreateChildElement, label.toStdString().c_str());
        }  
        else
        {
            UiCanvasBus::EventResult(
                element, editWindow->GetCanvas(), &UiCanvasBus::Events::CreateChildElement, label.toStdString().c_str());
        }

        if (element->GetState() == AZ::Entity::State::Active)
        {
            element->Deactivate();    // deactivate so that we can add components
        }

        // add a transform component to the element - all UI elements have a transform
        element->CreateComponent(LyShine::UiTransform2dComponentUuid);

        if (element->GetState() == AZ::Entity::State::Constructed)
        {
            element->Init();      // init
        }

        if (element->GetState() == AZ::Entity::State::Init)
        {
            element->Activate();      // activate
        }

        m_elementId = element->GetId();

        // Move the new child element to the desired child index
        if (childIndex >= 0)
        {
            AZ::EntityId parentEntityId;
            if (parentHierarchyItem)
            {
                parentEntityId = parentHierarchyItem->GetEntityId();
            }

            AZ::EntityId insertBeforeEntityId;
            UiElementBus::EventResult(insertBeforeEntityId, parentEntityId, &UiElementBus::Events::GetChildEntityId, childIndex);

            UiElementBus::Event(m_elementId, &UiElementBus::Events::ReparentByEntityId, parentEntityId, insertBeforeEntityId);
        }
    }

    AZ_Assert(m_elementId.IsValid(), "Invalid element ID");

    // Connect signals.
    {
        // Register with the entity map for quick lookup.

        QObject::connect(this,
            SIGNAL(SignalItemAdd(HierarchyItem*)),
            m_editorWindow->GetHierarchy(),
            SLOT(HandleItemAdd(HierarchyItem*)));

        QObject::connect(this,
            SIGNAL(SignalItemRemove(HierarchyItem*)),
            m_editorWindow->GetHierarchy(),
            SLOT(HandleItemRemove(HierarchyItem*)));
    }

    // Add to the entity map for quick lookup.
    //
    // IMPORTANT: This MUST be done BEFORE changing the
    // behavior and look of this class.
    SignalItemAdd(this);

    // Behavior and look.
    //
    // IMPORTANT: This MUST be done AFTER SignalItemAdd().
    {
        setFlags(flags() |
            Qt::ItemIsEditable |
            Qt::ItemIsDragEnabled |
            Qt::ItemIsDropEnabled);

        UpdateIcon();
        UpdateSliceInfo();
        UpdateEditorOnlyInfo();
    }
}

HierarchyItem::~HierarchyItem()
{
    DeleteElement();

    // Remove from quick lookup entity map.
    SignalItemRemove(this);
}

void HierarchyItem::DeleteElement()
{
    // IMPORTANT: DeleteElement() can be called from ~HierarchyItem().
    // Parent HierarchyItem are destroyed BEFORE their children.
    // When a parent HierarchyItem is destroyed, all its AZ::Entity
    // children are destroyed. Therefore, it's NECESSARY to use
    // SAFE_DELETE(). That's because our AZ::Entity might have already
    // been deleted. In which case GetElement() will return nullptr.
    // ~HierarchyItem() is the ONLY place where GetElement() is allowed
    // return nullptr.
    UiElementBus::Event(m_elementId, &UiElementBus::Events::DestroyElement);
}

AZ::Entity* HierarchyItem::GetElement() const
{
    // IMPORTANT: "element" will NEVER be nullptr, EXCEPT in ~HierarchyItem().
    // In the ~HierarchyItem(), deleting the parent of our element will cause
    // our own element to be destroyed. ~HierarchyItem() is the ONLY place
    // where we CAN'T assume that our m_elementId is always valid.
    return EntityHelpers::GetEntity(m_elementId);
}

AZ::EntityId HierarchyItem::GetEntityId() const
{
    return m_elementId;
}

void HierarchyItem::ClearEntityId()
{
    m_elementId.SetInvalid();
}

void HierarchyItem::SetMouseIsHovering(bool isHovering)
{
    m_mouseIsHovering = isHovering;

    UpdateIcon();
}

void HierarchyItem::SetIsExpanded(bool isExpanded)
{
    // Runtime-side.
    UiEditorBus::Event(m_elementId, &UiEditorBus::Events::SetIsExpanded, isExpanded);

    // Editor-side.
    setExpanded(isExpanded);
}

void HierarchyItem::ApplyElementIsExpanded()
{
    bool isExpanded = false;
    UiEditorBus::EventResult(isExpanded, m_elementId, &UiEditorBus::Events::GetIsExpanded);

    setExpanded(isExpanded);
}

void HierarchyItem::SetIsSelectable(bool isSelectable)
{
    // Runtime-side.
    UiEditorBus::Event(m_elementId, &UiEditorBus::Events::SetIsSelectable, isSelectable);

    // Editor-side.
    UpdateIcon();
    UpdateChildIcon();
    m_editorWindow->GetViewport()->Refresh();
}

void HierarchyItem::SetIsSelected(bool isSelected)
{
    // Runtime-side.
    UiEditorBus::Event(m_elementId, &UiEditorBus::Events::SetIsSelected, isSelected);

    // Editor-side.
    setSelected(isSelected);
    UpdateIcon();
    m_editorWindow->GetViewport()->Refresh();
}

void HierarchyItem::SetIsVisible(bool isVisible)
{
    // Runtime-side.
    UiEditorBus::Event(m_elementId, &UiEditorBus::Events::SetIsVisible, isVisible);

    // Editor-side.
    UpdateIcon();
    UpdateChildIcon();
    m_editorWindow->GetViewport()->Refresh();
}

void HierarchyItem::UpdateIcon()
{
    // Eye icon.
    {
        const char* textureName = nullptr;

        bool isVisible = false;
        UiEditorBus::EventResult(isVisible, m_elementId, &UiEditorBus::Events::GetIsVisible);

        if (isVisible)
        {
            // This item is visible.

            bool areAllAncestorsVisible = true;
            UiEditorBus::EventResult(areAllAncestorsVisible, m_elementId, &UiEditorBus::Events::AreAllAncestorsVisible);

            textureName = (m_mouseIsHovering ? UICANVASEDITOR_HIERARCHY_ICON_OPEN_HOVER : (areAllAncestorsVisible ? UICANVASEDITOR_HIERARCHY_ICON_OPEN : UICANVASEDITOR_HIERARCHY_ICON_OPEN_HIDDEN));
        }
        else
        {
            // This item is NOT visible.
            textureName = (m_mouseIsHovering ? UICANVASEDITOR_HIERARCHY_ICON_OPEN_HIDDEN : "");
        }

        setIcon(kHierarchyColumnIsVisible, QIcon(textureName).pixmap(UICANVASEDITOR_HIERARCHY_HEADER_ICON_SIZE, UICANVASEDITOR_HIERARCHY_HEADER_ICON_SIZE));
    }

    // Padlock icon.
    {
        const char* textureName = nullptr;

        bool isSelectable = false;
        UiEditorBus::EventResult(isSelectable, m_elementId, &UiEditorBus::Events::GetIsSelectable);

        if (isSelectable)
        {
            // This item is NOT locked.
            textureName = (m_mouseIsHovering ? UICANVASEDITOR_HIERARCHY_ICON_PADLOCK_ENABLED : "");
        }
        else
        {
            // This item is locked.
            textureName = (m_mouseIsHovering ? UICANVASEDITOR_HIERARCHY_ICON_PADLOCK_ENABLED_HOVER : UICANVASEDITOR_HIERARCHY_ICON_PADLOCK_ENABLED);
        }

        setIcon(kHierarchyColumnIsSelectable, QIcon(textureName).pixmap(UICANVASEDITOR_HIERARCHY_HEADER_ICON_SIZE, UICANVASEDITOR_HIERARCHY_HEADER_ICON_SIZE));
    }
}

void HierarchyItem::UpdateChildIcon()
{
    // Seed the list.
    HierarchyItemRawPtrList items;
    HierarchyHelpers::AppendAllChildrenToEndOfList(this, items);

    // Update child icons.
    HierarchyHelpers::TraverseListAndAllChildren(items,
        [](HierarchyItem* childItem)
        {
            childItem->UpdateIcon();
        });
}

HierarchyItem* HierarchyItem::Parent() const
{
    // It's ok to return a nullptr.
    // nullptr normally happens when we've reached the invisibleRootItem(),
    // We DON'T consider the invisibleRootItem() the parent of a HierarchyItem.
    return HierarchyItem::RttiCast(QTreeWidgetItem::parent());
}

HierarchyItem* HierarchyItem::Child(int i) const
{
    HierarchyItem* item = HierarchyItem::RttiCast(QTreeWidgetItem::child(i));
    AZ_Assert(item, "There's an item in the Hierarchy that isn't a HierarchyItem.");

    return item;
}

void HierarchyItem::SetMark(bool m)
{
    m_mark = m;
}

bool HierarchyItem::GetMark()
{
    return m_mark;
}

void HierarchyItem::SetPreMove(AZ::EntityId parentId, int childRow)
{
    m_preMoveParentId = parentId;
    m_preMoveChildRow = childRow;
}

AZ::EntityId HierarchyItem::GetPreMoveParentId()
{
    return m_preMoveParentId;
}

int HierarchyItem::GetPreMoveChildRow()
{
    return m_preMoveChildRow;
}

void HierarchyItem::ReplaceElement(const AZStd::string& xml, const AZStd::unordered_set<AZ::Data::AssetId>& referencedSliceAssets)
{
    AZ_Assert(!xml.empty(), "XML is empty");

    AZ::Entity* parentEntity = Parent() ? Parent()->GetElement() : nullptr;
    AZ::Entity* replaceEntity = GetElement();

    // find the element after the one to be replaced
    AZ::Entity* insertBeforeEntity = nullptr;
    {
        LyShine::EntityArray childElements;
        if (parentEntity)
        {
            UiElementBus::EventResult(childElements, parentEntity->GetId(), &UiElementBus::Events::GetChildElements);
        }
        else
        {
            UiCanvasBus::EventResult(childElements, m_editorWindow->GetCanvas(), &UiCanvasBus::Events::GetChildElements);
        }

        // find the enity we are replacing in the list (it must exist)
        auto iter = std::find(childElements.begin(), childElements.end(), replaceEntity);
        AZ_Assert(iter != childElements.end(), "Entity not found");

        // if there is an element after the replace element then that is the one to insert before
        if (++iter != childElements.end())
        {
            insertBeforeEntity = *iter;
        }
    }

    // If restoring to a slice, keep a reference to the slice asset so it isn't released when the entity
    // is deleted, only to immediately reload upon restoring.
    AZStd::vector<AZ::Data::Asset<AZ::SliceAsset>> preservedAssetsRefs;
    for (auto assetId : referencedSliceAssets)
    {
        preservedAssetsRefs.push_back(AZ::Data::AssetManager::Instance().FindAsset(assetId, AZ::Data::AssetLoadBehavior::Default));
    }

    // Discard the old element.
    DeleteElement();

    // Load the new element.
    SerializeHelpers::RestoreSerializedElements(m_editorWindow->GetCanvas(),
        parentEntity,
        insertBeforeEntity,
        m_editorWindow->GetEntityContext(),
        xml,
        false,
        nullptr);

    // Update any visual information that may have changed with this element or any of its descendants
    UpdateEditorOnlyInfoRecursive();
}

void HierarchyItem::UpdateSliceInfo()
{
    // This is deliberately slightly different to the blue color used for hover, so that we can still see a change on hover
    static const QColor sliceForegroundColor(117, 156, 254);

    //determine if entity belongs to a slice
    AZ::SliceComponent::SliceInstanceAddress sliceAddress;
    AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, m_elementId,
        &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);
    bool isSliceEntity = sliceAddress.IsValid();

    AZStd::string sliceAssetName;
    if (isSliceEntity)
    {
        auto sliceReference = sliceAddress.GetReference();
        auto sliceInstance = sliceAddress.GetInstance();

        // determine slice asset name (for tooltip display)
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetName, &AZ::Data::AssetCatalogRequests::GetAssetPathById, sliceReference->GetSliceAsset().GetId());

        //determine if entity parent belongs to a slice
        AZ::EntityId parentId;
        UiElementBus::EventResult(parentId, m_elementId, &UiElementBus::Events::GetParentEntityId);

        AZ::SliceComponent::SliceInstanceAddress parentSliceAddress;
        AzFramework::SliceEntityRequestBus::EventResult(parentSliceAddress, parentId,
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

        //we're a slice root if our parent doesn't have a slice reference or instance or our parent slice reference or instances don't match ours
        auto parentSliceReference = parentSliceAddress.GetReference();
        auto parentSliceInstance = parentSliceAddress.GetInstance();
        bool isSliceRoot = !parentSliceReference || !parentSliceInstance || (sliceReference != parentSliceReference) || (sliceInstance->GetId() != parentSliceInstance->GetId());

        // set the text color to the slice color
        setForeground(0, sliceForegroundColor);

        // use bold or italic to indicate whether this is the root of the slice instance or a child entity within an instance
        auto itemFont = font(0);
        if (isSliceRoot)
        {
            itemFont.setBold(true);
        }
        else
        {
            itemFont.setItalic(true);
        }
        setFont(0, itemFont);
    }
    else
    {
        // get the normal text color from the palette
        auto parentWidgetPtr = static_cast<QWidget*>(m_editorWindow);
        setForeground(0, QBrush(parentWidgetPtr->palette().color(QPalette::Text)));

        // set to normal font
        auto itemFont = font(0);
        itemFont.setBold(false);
        itemFont.setItalic(false);
        setFont(0, itemFont);
    }

    // Set tooltip to indicate which slice this is part of (if any)
    QString tooltip = !sliceAssetName.empty() ? QString("Slice asset: %1").arg(sliceAssetName.data()) : QString("Slice asset: This entity is not part of a slice.");
    setToolTip(0, tooltip);
}

void HierarchyItem::UpdateEditorOnlyInfo()
{
    bool isEditorOnly = false;
    AzToolsFramework::EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, m_elementId, &AzToolsFramework::EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);

    if (isEditorOnly)
    {
        static const QColor editorOnlyBackgroundColor(60, 0, 0);
        setBackground(0, editorOnlyBackgroundColor);
    }
    else
    {
        setBackground(0, Qt::transparent);
    }
}

void HierarchyItem::UpdateEditorOnlyInfoRecursive()
{
    UpdateEditorOnlyInfo();
    for (int i = 0; i < childCount(); ++i)
    {
        HierarchyItem* item = HierarchyItem::RttiCast(child(i));
        item->UpdateEditorOnlyInfoRecursive();
    }
}

void HierarchyItem::SetNonSnappedOffsets(UiTransform2dInterface::Offsets offsets)
{
    m_nonSnappedOffsets = offsets;
}

UiTransform2dInterface::Offsets HierarchyItem::GetNonSnappedOffsets()
{
    return m_nonSnappedOffsets;
}

void HierarchyItem::SetNonSnappedZRotation(float rotation)
{
    m_nonSnappedZRotation = rotation;
}

float HierarchyItem::GetNonSnappedZRotation()
{
    return m_nonSnappedZRotation;
}

#include <moc_HierarchyItem.cpp>
