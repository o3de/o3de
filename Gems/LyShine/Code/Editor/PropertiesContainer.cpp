/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "CanvasHelpers.h"
#include <AzQtComponents/Components/Style.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <LyShine/Bus/UiSystemBus.h>

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QContextMenuEvent>
//-------------------------------------------------------------------------------

//we require an overlay widget to act as a canvas to draw on top of everything in the inspector
//attaching to inspector rather than component editors so we can draw outside of bounds
class PropertyContainerOverlay : public QWidget
{
public:
    PropertyContainerOverlay(PropertiesContainer* editor, QWidget* parent)
        : QWidget(parent)
        , m_editor(editor)
        , m_dropIndicatorOffset(8)
    {
        setPalette(Qt::transparent);
        setWindowFlags(Qt::FramelessWindowHint);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        const int TopMargin = 1;
        const int RightMargin = 2;
        const int BottomMargin = 5;
        const int LeftMargin = 2;

        QWidget::paintEvent(event);

        QPainter painter(this);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        QRect currRect;
        bool drag = false;
        bool drop = false;

        for (auto componentEditor : m_editor->m_componentEditors)
        {
            if (!componentEditor->isVisible())
            {
                continue;
            }

            QRect globalRect = m_editor->GetWidgetGlobalRect(componentEditor);

            currRect = QRect(
                QPoint(mapFromGlobal(globalRect.topLeft()) + QPoint(LeftMargin, TopMargin)),
                QPoint(mapFromGlobal(globalRect.bottomRight()) - QPoint(RightMargin, BottomMargin))
            );

            currRect.setWidth(currRect.width() - 1);
            currRect.setHeight(currRect.height() - 1);

            if (componentEditor->IsDragged())
            {
                QStyleOption opt;
                opt.init(this);
                opt.rect = currRect;
                static_cast<AzQtComponents::Style*>(style())->drawDragIndicator(&opt, &painter, this);
                drag = true;
            }

            if (componentEditor->IsDropTarget())
            {
                QRect dropRect = currRect;
                dropRect.setTop(currRect.top() - m_dropIndicatorOffset);
                dropRect.setHeight(0);

                QStyleOption opt;
                opt.init(this);
                opt.rect = dropRect;
                style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, &painter, this);

                drop = true;
            }
        }

        if (drag && !drop)
        {
            QRect dropRect = currRect;
            dropRect.setTop(currRect.top() - m_dropIndicatorOffset);
            dropRect.setHeight(0);

            QStyleOption opt;
            opt.init(this);
            opt.rect = dropRect;
            style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, &painter, this);
        }
    }

private:
    PropertiesContainer* m_editor;
    int m_dropIndicatorOffset;
};

//-------------------------------------------------------------------------------

PropertiesContainer::PropertiesContainer(PropertiesWidget* propertiesWidget, EditorWindow* editorWindow)
    : QScrollArea(propertiesWidget)
    , m_propertiesWidget(propertiesWidget)
    , m_editorWindow(editorWindow)
    , m_selectedEntityDisplayNameWidget(nullptr)
    , m_selectionHasChanged(false)
    , m_isCanvasSelected(false)
    , m_selectionEventAccepted(false)
    , m_componentEditorLastSelectedIndex(-1)
    , m_serializeContext(nullptr)
{
    setFocusPolicy(Qt::ClickFocus);
    setFrameShape(QFrame::NoFrame);
    setFrameShadow(QFrame::Plain);
    setLineWidth(0);
    setWidgetResizable(true);

    m_componentListContents = new QWidget();
    m_componentListContents->setGeometry(QRect(0, 0, 382, 537));
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sizePolicy.setHeightForWidth(m_componentListContents->sizePolicy().hasHeightForWidth());
    m_componentListContents->setSizePolicy(sizePolicy);
    m_rowLayout = new QVBoxLayout(m_componentListContents);
    m_rowLayout->setSpacing(10);
    m_rowLayout->setContentsMargins(0, 0, 0, 0);
    m_rowLayout->setAlignment(Qt::AlignTop);

    setWidget(m_componentListContents);

    m_overlay = new PropertyContainerOverlay(this, m_componentListContents);
    UpdateOverlay();

    CreateActions();

    // Get the serialize context.
    AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    AZ_Assert(m_serializeContext, "We should have a valid context!");

    QObject::connect(m_editorWindow->GetHierarchy(),
        &HierarchyWidget::editorOnlyStateChangedOnSelectedElements,
        [this]()
    {
        UpdateEditorOnlyCheckbox();
    });
}

void PropertiesContainer::resizeEvent(QResizeEvent* event)
{
    QScrollArea::resizeEvent(event);
    UpdateOverlay();
}

void PropertiesContainer::contextMenuEvent(QContextMenuEvent* event)
{
    OnDisplayUiComponentEditorMenu(event->globalPos());
    event->accept();
}

//overridden to intercept application level mouse events for component editor selection
bool PropertiesContainer::eventFilter(QObject* object, QEvent* event)
{
    HandleSelectionEvents(object, event);
    return false;
}

bool PropertiesContainer::HandleSelectionEvents(QObject* object, QEvent* event)
{
    (void)object;
    if (m_selectedEntities.empty())
    {
        return false;
    }

    if (event->type() != QEvent::MouseButtonPress &&
        event->type() != QEvent::MouseButtonDblClick &&
        event->type() != QEvent::MouseButtonRelease)
    {
        return false;
    }

    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

    //selection now occurs on mouse released
    //reset selection flag when mouse is clicked to allow additional selection changes
    if (event->type() == QEvent::MouseButtonPress)
    {
        m_selectionEventAccepted = false;
        return false;
    }

    //reject input if selection already occurred for this click
    if (m_selectionEventAccepted)
    {
        return false;
    }

    //reject input if a popup or modal window is active
    if (QApplication::activeModalWidget() != nullptr ||
        QApplication::activePopupWidget() != nullptr)
    {
        return false;
    }

    const QRect globalRect(mouseEvent->globalPos(), mouseEvent->globalPos());

    //reject input outside of the inspector's component list
    if (!DoesOwnFocus() ||
        !DoesIntersectWidget(globalRect, this))
    {
        return false;
    }

    //reject input from other buttons
    if ((mouseEvent->button() != Qt::LeftButton) &&
        (mouseEvent->button() != Qt::RightButton))
    {
        return false;
    }

    //right click is allowed if the component editor under the mouse is not selected
    if (mouseEvent->button() == Qt::RightButton)
    {
        if (DoesIntersectSelectedComponentEditor(globalRect))
        {
            return false;
        }

        ClearComponentEditorSelection();
        SelectIntersectingComponentEditors(globalRect, true);
    }
    else if (mouseEvent->button() == Qt::LeftButton)
    {
        //if shift or control is pressed this is a multi=-select operation, otherwise reset the selection
        if (mouseEvent->modifiers() & Qt::ControlModifier)
        {
            ToggleIntersectingComponentEditors(globalRect);
        }
        else if (mouseEvent->modifiers() & Qt::ShiftModifier)
        {
            ComponentEditorVector intersections = GetIntersectingComponentEditors(globalRect);
            if (!intersections.empty())
            {
                SelectRangeOfComponentEditors(m_componentEditorLastSelectedIndex, GetComponentEditorIndex(intersections.front()), true);
            }
        }
        else
        {
            ClearComponentEditorSelection();
            SelectIntersectingComponentEditors(globalRect, true);
        }
    }

    UpdateInternalState();

    //ensure selection logic executes only once per click since eventFilter may execute multiple times for a single click
    m_selectionEventAccepted = true;
    return true;
}

AZ::Entity::ComponentArrayType PropertiesContainer::GetSelectedComponents()
{
    ComponentEditorVector selectedComponentEditors;

    selectedComponentEditors.reserve(m_componentEditors.size());
    for (auto componentEditor : m_componentEditors)
    {
        if (componentEditor->isVisible() && componentEditor->IsSelected())
        {
            selectedComponentEditors.push_back(componentEditor);
        }
    }

    AZ::Entity::ComponentArrayType selectedComponents;

    for (auto componentEditor : selectedComponentEditors)
    {
        const auto& components = componentEditor->GetComponents();
        selectedComponents.insert(selectedComponents.end(), components.begin(), components.end());
    }

    return selectedComponents;
}

void PropertiesContainer::BuildSharedComponentList(ComponentTypeMap& sharedComponentsByType, const AzToolsFramework::EntityIdList& entitiesShown)
{
    // For single selection of a slice-instanced entity, gather the direct slice ancestor
    // so we can visualize per-component differences.
    m_compareToEntity.reset();
    if (1 == entitiesShown.size())
    {
        AZ::SliceComponent::SliceInstanceAddress address;
        AzFramework::SliceEntityRequestBus::EventResult(address, entitiesShown[0],
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);
        if (address.IsValid())
        {
            AZ::SliceComponent::EntityAncestorList ancestors;
            address.GetReference()->GetInstanceEntityAncestry(entitiesShown[0], ancestors, 1);

            if (!ancestors.empty())
            {
                m_compareToEntity = AzToolsFramework::SliceUtilities::CloneSliceEntityForComparison(*ancestors[0].m_entity, *address.GetInstance(), *m_serializeContext);
            }
        }
    }

    // Create a SharedComponentInfo for each component
    // that selected entities have in common.
    // See comments on SharedComponentInfo for more details
    for (AZ::EntityId entityId : entitiesShown)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
        AZ_Assert(entity, "Entity was selected but no such entity exists?");
        if (!entity)
        {
            continue;
        }

        // Track how many of each component-type we've seen on this entity
        AZStd::unordered_map<AZ::Uuid, size_t> entityComponentCounts;

        for (AZ::Component* component : entity->GetComponents())
        {
            const AZ::Uuid& componentType = azrtti_typeid(component);
            const AZ::SerializeContext::ClassData* classData = m_serializeContext->FindClassData(componentType);

            // Skip components without edit data
            if (!classData || !classData->m_editData)
            {
                continue;
            }

            // Skip components that are set to invisible.
            if (const AZ::Edit::ElementData* editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (AZ::Edit::Attribute* visibilityAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Visibility))
                {
                    AzToolsFramework::PropertyAttributeReader reader(component, visibilityAttribute);
                    AZ::u32 visibilityValue;
                    if (reader.Read<AZ::u32>(visibilityValue))
                    {
                        if (visibilityValue == AZ_CRC_CE("PropertyVisibility_Hide"))
                        {
                            continue;
                        }
                    }
                }
            }

            // The sharedComponentList is created based on the first entity.
            if (entityId == *entitiesShown.begin())
            {
                // Add new SharedComponentInfo
                SharedComponentInfo sharedComponent;
                sharedComponent.m_classData = classData;
                sharedComponentsByType[componentType].push_back(sharedComponent);
            }

            // Skip components that don't correspond to a type from the first entity.
            if (sharedComponentsByType.find(componentType) == sharedComponentsByType.end())
            {
                continue;
            }

            // Update entityComponentCounts (may be multiple components of this type)
            auto entityComponentCountsIt = entityComponentCounts.find(componentType);
            size_t componentIndex = (entityComponentCountsIt == entityComponentCounts.end())
                ? 0
                : entityComponentCountsIt->second;
            entityComponentCounts[componentType] = componentIndex + 1;

            // Skip component if the first entity didn't have this many.
            if (componentIndex >= sharedComponentsByType[componentType].size())
            {
                continue;
            }

            // Component accepted! Add it as an instance
            SharedComponentInfo& sharedComponent = sharedComponentsByType[componentType][componentIndex];
            sharedComponent.m_instances.push_back(component);

            // If specified, locate the corresponding component in the comparison entity to
            // visualize differences.
            if (m_compareToEntity && !sharedComponent.m_compareInstance)
            {
                size_t compareComponentIndex = 0;
                for (AZ::Component* compareComponent : m_compareToEntity.get()->GetComponents())
                {
                    const AZ::Uuid& compareComponentType = azrtti_typeid(compareComponent);
                    if (componentType == compareComponentType)
                    {
                        if (componentIndex == compareComponentIndex)
                        {
                            sharedComponent.m_compareInstance = compareComponent;
                            break;
                        }
                        compareComponentIndex++;
                    }
                }
            }
        }
    }

    // Cull any SharedComponentInfo that doesn't fit all our requirements
    ComponentTypeMap::iterator sharedComponentsByTypeIterator = sharedComponentsByType.begin();
    while (sharedComponentsByTypeIterator != sharedComponentsByType.end())
    {
        AZStd::vector<SharedComponentInfo>& sharedComponents = sharedComponentsByTypeIterator->second;

        // Remove component if it doesn't exist on every entity
        AZStd::vector<SharedComponentInfo>::iterator sharedComponentIterator = sharedComponents.begin();
        while (sharedComponentIterator != sharedComponents.end())
        {
            if (sharedComponentIterator->m_instances.size() != entitiesShown.size()
                || sharedComponentIterator->m_instances.empty())
            {
                sharedComponentIterator = sharedComponents.erase(sharedComponentIterator);
            }
            else
            {
                ++sharedComponentIterator;
            }
        }

        // Remove entry if all its components were culled
        if (sharedComponents.size() == 0)
        {
            sharedComponentsByTypeIterator = sharedComponentsByType.erase(sharedComponentsByTypeIterator);
        }
        else
        {
            ++sharedComponentsByTypeIterator;
        }
    }
}

void PropertiesContainer::BuildSharedComponentUI(ComponentTypeMap& sharedComponentsByType, const AzToolsFramework::EntityIdList& entitiesShown)
{
    (void)entitiesShown;

    // At this point in time:
    // - Each SharedComponentInfo should contain one component instance
    //   from each selected entity.
    // - Any pre-existing m_componentEditor entries should be
    //   cleared of component instances.

    // Add each component instance to its corresponding editor
    // We add them in the order that the component factories were registered in, this provides
    // a consistent order of components. It doesn't appear to be the case that components always
    // stay in the order they were added to the entity in, some of our slices do not have the
    // UiElementComponent first for example.
    const AZStd::vector<AZ::Uuid>* componentTypes;
    UiSystemBus::BroadcastResult(componentTypes, &UiSystemBus::Events::GetComponentTypesForMenuOrdering);

    // There could be components that were not registered for component ordering. We don't
    // want to hide them. So add them at the end of the list
    AZStd::vector<AZ::Uuid> componentOrdering;
    componentOrdering = *componentTypes;
    for (auto sharedComponentMapEntry : sharedComponentsByType)
    {
        if (AZStd::find(componentOrdering.begin(), componentOrdering.end(), sharedComponentMapEntry.first) == componentOrdering.end())
        {
            componentOrdering.push_back(sharedComponentMapEntry.first);
        }
    }

    m_componentEditors.clear();

    for (auto& componentType : componentOrdering)
    {
        if (sharedComponentsByType.count(componentType) <= 0)
        {
            continue; // there are no components of this type in the sharedComponentsByType map
        }

        const auto& sharedComponents = sharedComponentsByType[componentType];

        for (size_t sharedComponentIndex = 0; sharedComponentIndex < sharedComponents.size(); ++sharedComponentIndex)
        {
            const SharedComponentInfo& sharedComponent = sharedComponents[sharedComponentIndex];

            AZ_Assert(sharedComponent.m_instances.size() == entitiesShown.size()
                && !sharedComponent.m_instances.empty(),
                "sharedComponentsByType should only contain valid entries at this point");

            // Create an editor if necessary
            ComponentEditorVector& componentEditors = m_componentEditorsByType[componentType];
            if (sharedComponentIndex >= componentEditors.size())
            {
                componentEditors.push_back(CreateComponentEditor(*sharedComponent.m_instances[0]));
            }
            else
            {
                // Place existing editor in correct order
                m_rowLayout->removeWidget(componentEditors[sharedComponentIndex]);
                m_rowLayout->addWidget(componentEditors[sharedComponentIndex]);
            }

            AzToolsFramework::ComponentEditor* componentEditor = componentEditors[sharedComponentIndex];

            // Save a list of components in order shown
            m_componentEditors.push_back(componentEditor);

            // Add instances to componentEditor
            auto& componentInstances = sharedComponent.m_instances;
            for (AZ::Component* componentInstance : componentInstances)
            {
                // non-first instances are aggregated under the first instance
                AZ::Component* aggregateInstance = componentInstance != componentInstances.front() ? componentInstances.front() : nullptr;

                // Reference the slice entity if we are a slice so we can indicate differences from base
                AZ::Component* compareInstance = sharedComponent.m_compareInstance;

                componentEditor->AddInstance(componentInstance, aggregateInstance, compareInstance);
            }

            // Refresh editor
            componentEditor->InvalidateAll();
            componentEditor->show();
        }
    }
}

AzToolsFramework::ComponentEditor* PropertiesContainer::CreateComponentEditor([[maybe_unused]] const AZ::Component& componentInstance)
{
    AzToolsFramework::ComponentEditor* editor = new AzToolsFramework::ComponentEditor(m_serializeContext, m_propertiesWidget, this);
    connect(editor, &AzToolsFramework::ComponentEditor::OnDisplayComponentEditorMenu, this, &PropertiesContainer::OnDisplayUiComponentEditorMenu);

    m_rowLayout->addWidget(editor);
    editor->hide();

    return editor;
}

bool PropertiesContainer::DoesOwnFocus() const
{
    QWidget* widget = QApplication::focusWidget();
    return this == widget || isAncestorOf(widget);
}

QRect PropertiesContainer::GetWidgetGlobalRect(const QWidget* widget) const
{
    return QRect(
        widget->mapToGlobal(widget->rect().topLeft()),
        widget->mapToGlobal(widget->rect().bottomRight()));
}

bool PropertiesContainer::DoesIntersectWidget(const QRect& globalRect, const QWidget* widget) const
{
    return widget->isVisible() && globalRect.intersects(GetWidgetGlobalRect(widget));
}

bool PropertiesContainer::DoesIntersectSelectedComponentEditor(const QRect& globalRect) const
{
    for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
    {
        if (componentEditor->IsSelected())
        {
            return true;
        }
    }
    return false;
}

bool PropertiesContainer::DoesIntersectNonSelectedComponentEditor(const QRect& globalRect) const
{
    for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
    {
        if (!componentEditor->IsSelected())
        {
            return true;
        }
    }
    return false;
}

void PropertiesContainer::ClearComponentEditorSelection()
{
    AZ_PROFILE_FUNCTION(AzToolsFramework);
    for (auto componentEditor : m_componentEditors)
    {
        componentEditor->SetSelected(false);
    }

    UpdateInternalState();
}

void PropertiesContainer::SelectRangeOfComponentEditors(const AZ::s32 index1, const AZ::s32 index2, bool selected)
{
    if (index1 >= 0 && index2 >= 0)
    {
        const AZ::s32 min = AZStd::min(index1, index2);
        const AZ::s32 max = AZStd::max(index1, index2);
        for (AZ::s32 index = min; index <= max; ++index)
        {
            m_componentEditors[index]->SetSelected(selected);
        }
        m_componentEditorLastSelectedIndex = index2;
    }

    UpdateInternalState();
}

void PropertiesContainer::SelectIntersectingComponentEditors(const QRect& globalRect, bool selected)
{
    for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
    {
        componentEditor->SetSelected(selected);
        m_componentEditorLastSelectedIndex = GetComponentEditorIndex(componentEditor);
    }

    UpdateInternalState();
}

void PropertiesContainer::ToggleIntersectingComponentEditors(const QRect& globalRect)
{
    for (auto componentEditor : GetIntersectingComponentEditors(globalRect))
    {
        componentEditor->SetSelected(!componentEditor->IsSelected());
        m_componentEditorLastSelectedIndex = GetComponentEditorIndex(componentEditor);
    }

    UpdateInternalState();
}

AZ::s32 PropertiesContainer::GetComponentEditorIndex(const AzToolsFramework::ComponentEditor* componentEditor) const
{
    AZ::s32 index = 0;
    for (auto componentEditorToCompare : m_componentEditors)
    {
        if (componentEditorToCompare == componentEditor)
        {
            return index;
        }
        ++index;
    }
    return -1;
}

AZStd::vector<AzToolsFramework::ComponentEditor*> PropertiesContainer::GetIntersectingComponentEditors(const QRect& globalRect) const
{
    ComponentEditorVector intersectingComponentEditors;
    intersectingComponentEditors.reserve(m_componentEditors.size());
    for (auto componentEditor : m_componentEditors)
    {
        if (DoesIntersectWidget(globalRect, componentEditor))
        {
            intersectingComponentEditors.push_back(componentEditor);
        }
    }
    return intersectingComponentEditors;
}

void PropertiesContainer::CreateActions()
{
    QAction* seperator1 = new QAction(this);
    seperator1->setSeparator(true);
    addAction(seperator1);

    m_actionToAddComponents = new QAction(tr("Add component"), this);
    m_actionToAddComponents->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToAddComponents, &QAction::triggered, this, &PropertiesContainer::OnAddComponent);
    addAction(m_actionToAddComponents);

    m_actionToDeleteComponents = ComponentHelpers::CreateRemoveComponentsAction(this);
    addAction(m_actionToDeleteComponents);

    QAction* seperator2 = new QAction(this);
    seperator2->setSeparator(true);
    addAction(seperator2);

    m_actionToCutComponents = ComponentHelpers::CreateCutComponentsAction(this);
    addAction(m_actionToCutComponents);

    m_actionToCopyComponents = ComponentHelpers::CreateCopyComponentsAction(this);
    addAction(m_actionToCopyComponents);

    m_actionToPasteComponents = ComponentHelpers::CreatePasteComponentsAction(this);
    addAction(m_actionToPasteComponents);

    UpdateInternalState();
}

void PropertiesContainer::UpdateActions()
{
    ComponentHelpers::UpdateRemoveComponentsAction(m_actionToDeleteComponents);
    ComponentHelpers::UpdateCutComponentsAction(m_actionToCutComponents);
    ComponentHelpers::UpdateCopyComponentsAction(m_actionToCopyComponents);
    // The paste components action always remains enabled except for when the context menu is up
    // This is so a paste can be performed after a copy operation was made via the shortcut keys (since we don't know when a copy was performed)
}

void PropertiesContainer::UpdateOverlay()
{
    m_overlay->setVisible(true);
    m_overlay->setGeometry(m_componentListContents->rect());
    m_overlay->raise();
    m_overlay->update();
}

void PropertiesContainer::UpdateInternalState()
{
    UpdateActions();
    UpdateOverlay();
}

void PropertiesContainer::OnAddComponent()
{
    HierarchyMenu contextMenu(m_editorWindow->GetHierarchy(),
        HierarchyMenu::Show::kAddComponents,
        true);

    contextMenu.exec(QCursor::pos());
}

void PropertiesContainer::OnDisplayUiComponentEditorMenu(const QPoint& position)
{
    ShowContextMenu(position);
}

void PropertiesContainer::ShowContextMenu(const QPoint& position)
{
    UpdateInternalState();
    // The paste components action is only updated right before the context menu appears, otherwise it remains enabled
    ComponentHelpers::UpdatePasteComponentsAction(m_actionToPasteComponents);

    HierarchyMenu contextMenu(m_editorWindow->GetHierarchy(),
        HierarchyMenu::Show::kPushToSlice,
        false);

    contextMenu.addActions(actions());

    if (!contextMenu.isEmpty())
    {
        contextMenu.exec(position);
    }

    // Keep the paste components action enabled when the context menu is not showing in order to handle pastes after a copy was performed
    m_actionToPasteComponents->setEnabled(true);
}

void PropertiesContainer::Update()
{
    size_t selectedEntitiesAmount = m_selectedEntities.size();
    QString displayName;

    if (selectedEntitiesAmount == 0)
    {
        displayName = "No Canvas Loaded";
    }
    else if (selectedEntitiesAmount == 1)
    {
        // Either only one element selected, or none (still is 1 because it selects the canvas instead)

        // If the canvas was selected
        if (m_isCanvasSelected)
        {
            displayName = "Canvas";
        }
        // Else one element was selected
        else
        {
            // Set the name in the properties pane to the name of the element
            AZ::EntityId selectedElement = m_selectedEntities.front();
            AZStd::string selectedEntityName;
            UiElementBus::EventResult(selectedEntityName, selectedElement, &UiElementBus::Events::GetName);
            displayName = selectedEntityName.c_str();
        }
    }
    else // more than one entity selected
    {
        displayName = QString::number(selectedEntitiesAmount) + " elements selected";
    }

    // Update the selected element display name
    if (m_selectedEntityDisplayNameWidget != nullptr)
    {
        m_selectedEntityDisplayNameWidget->setText(displayName);

        // Preventing renaming entities if the canvas entity is selected or
        // multiple entities are selected.
        if (m_isCanvasSelected || selectedEntitiesAmount > 1)
        {
            m_selectedEntityDisplayNameWidget->setEnabled(false);
        }
        else
        {
            m_selectedEntityDisplayNameWidget->setEnabled(true);
        }
    }

    // Clear content.
    {
        for (int j = m_rowLayout->count(); j > 0; --j)
        {
            AzToolsFramework::ComponentEditor* editor = static_cast<AzToolsFramework::ComponentEditor*>(m_rowLayout->itemAt(j - 1)->widget());

            editor->hide();
            editor->ClearInstances(true);
        }

        m_compareToEntity.reset();
    }

    UpdateEditorOnlyCheckbox();

    if (m_selectedEntities.empty())
    {
        return; // nothing to do
    }

    ComponentTypeMap sharedComponentList;
    BuildSharedComponentList(sharedComponentList, m_selectedEntities);
    BuildSharedComponentUI(sharedComponentList, m_selectedEntities);

    UpdateInternalState();
}

void PropertiesContainer::UpdateEditorOnlyCheckbox()
{
    if (m_isCanvasSelected)
    {
        // The canvas itself can't be editor-only, so don't show the checkbox when the
        // canvas is displayed in the properties pane.
        m_editorOnlyCheckbox->setVisible(false);
    }
    else
    {
        QSignalBlocker noSignals(m_editorOnlyCheckbox);

        if (m_selectedEntities.empty())
        {
            m_editorOnlyCheckbox->setVisible(false);
            return;
        }

        m_editorOnlyCheckbox->setVisible(true);

        bool allEditorOnly = true;
        bool noneEditorOnly = true;

        for (AZ::EntityId id : m_selectedEntities)
        {
            // If any of the entities is a slice root, grey out the checkbox.
            bool isSliceRoot = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceRoot, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceRoot);
            if (isSliceRoot)
            {
                m_editorOnlyCheckbox->setChecked(false);
                m_editorOnlyCheckbox->setEnabled(false);
                return;
            }

            bool isEditorOnly = false;
            AzToolsFramework::EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, id, &AzToolsFramework::EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);

            allEditorOnly &= isEditorOnly;
            noneEditorOnly &= !isEditorOnly;
        }

        m_editorOnlyCheckbox->setEnabled(true);

        if (allEditorOnly)
        {
            m_editorOnlyCheckbox->setCheckState(Qt::CheckState::Checked);
        }
        else if (noneEditorOnly)
        {
            m_editorOnlyCheckbox->setCheckState(Qt::CheckState::Unchecked);
        }
        else // Some marked editor-only, some not
        {
            m_editorOnlyCheckbox->setCheckState(Qt::CheckState::PartiallyChecked);
        }
    }
}

void PropertiesContainer::Refresh(AzToolsFramework::PropertyModificationRefreshLevel refreshLevel, const AZ::Uuid* componentType)
{
    if (m_selectionHasChanged)
    {
        Update();
        m_selectionHasChanged = false;
    }
    else
    {
        for (auto& componentEditorsPair : m_componentEditorsByType)
        {
            if (!componentType || (*componentType == componentEditorsPair.first))
            {
                for (AzToolsFramework::ComponentEditor* editor : componentEditorsPair.second)
                {
                    if (editor->isVisible())
                    {
                        editor->QueuePropertyEditorInvalidation(refreshLevel);
                    }
                }
            }
        }

        // If the selection has not changed, but a refresh was prompted then the name of the currently selected entity might
        // have changed.
        size_t selectedEntitiesAmount = m_selectedEntities.size();
        // Check if only one entity is selected and that it is an element
        if (selectedEntitiesAmount == 1 && !m_isCanvasSelected)
        {
            // Set the name in the properties pane to the name of the element
            AZ::EntityId selectedElement = m_selectedEntities.front();
            AZStd::string selectedEntityName;
            UiElementBus::EventResult(selectedEntityName, selectedElement, &UiElementBus::Events::GetName);

            // Update the selected element display name
            if (m_selectedEntityDisplayNameWidget != nullptr)
            {
                m_selectedEntityDisplayNameWidget->setText(selectedEntityName.c_str());
            }
        }
    }
}

void PropertiesContainer::SelectionChanged(HierarchyItemRawPtrList* items)
{
    ClearComponentEditorSelection();

    m_selectedEntities.clear();
    if (items)
    {
        for (auto i : *items)
        {
            m_selectedEntities.push_back(i->GetEntityId());
        }
    }

    m_isCanvasSelected = false;

    if (m_selectedEntities.empty())
    {
        // Add the canvas
        AZ::EntityId canvasId = m_editorWindow->GetCanvas();
        if (canvasId.IsValid())
        {
            m_selectedEntities.push_back(canvasId);
            m_isCanvasSelected = true;
        }
    }

    m_selectionHasChanged = true;
}

void PropertiesContainer::SelectedEntityPointersChanged()
{
    m_selectionHasChanged = true;
    Refresh();
}

void PropertiesContainer::RequestPropertyContextMenu([[maybe_unused]] AzToolsFramework::InstanceDataNode* node, const QPoint& globalPos)
{
    ShowContextMenu(globalPos);
}

void PropertiesContainer::SetSelectedEntityDisplayNameWidget(QLineEdit* selectedEntityDisplayNameWidget)
{
    if (selectedEntityDisplayNameWidget)
    {
        if (m_selectedEntityDisplayNameWidget)
        {
            QObject::disconnect(m_selectedEntityDisplayNameWidget);
        }

        m_selectedEntityDisplayNameWidget = selectedEntityDisplayNameWidget;

        // Listen for changes to the line edit field
        QObject::connect(m_selectedEntityDisplayNameWidget,
            &QLineEdit::editingFinished,
            [this]()
            {
                // Ignore editing when this field is read-only or if there is more than one
                // entity selected.
                if (!m_selectedEntityDisplayNameWidget->isEnabled() || m_selectedEntities.size() != 1)
                {
                    return;
                }

                AZ::EntityId selectedEntityId = m_selectedEntities.front();
                AZ::Entity* selectedEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(
                    selectedEntity, &AZ::ComponentApplicationBus::Events::FindEntity, selectedEntityId);
                if (selectedEntity)
                {
                    AZStd::string currentName = selectedEntity->GetName();
                    AZStd::string newName = m_selectedEntityDisplayNameWidget->text().toUtf8().constData();

                    CommandHierarchyItemRename::Push(m_editorWindow->GetActiveStack(),
                        m_editorWindow->GetHierarchy(),
                        selectedEntityId,
                        currentName.c_str(),
                        newName.c_str());
                }
            });
    }
}

void PropertiesContainer::SetEditorOnlyCheckbox(QCheckBox* editorOnlyCheckbox)
{
    m_editorOnlyCheckbox = editorOnlyCheckbox;

    QObject::connect(m_editorOnlyCheckbox,
        &QCheckBox::stateChanged,
        [this](int value)
        {
            QSignalBlocker blocker(this);

            QMetaObject::invokeMethod(m_editorWindow->GetHierarchy(), "SetEditorOnlyForSelectedItems", Qt::QueuedConnection, Q_ARG(bool, value));
        }
    );
}

#include <moc_PropertiesContainer.cpp>
