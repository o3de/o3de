/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ComponentEditor.hxx"
#include "ComponentEditorHeader.hxx"

#include <AzCore/Console/IConsole.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabComponentAdapter.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Slice/SliceEntityBus.h>

#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/CardNotification.h>
#include <AzQtComponents/Utilities/QtViewPaneEffects.h>

#include <QDesktopWidget>
#include <QMenu>
#include <QPushButton>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QVBoxLayout>
AZ_POP_DISABLE_WARNING
#include <QGraphicsEffect>
AZ_PUSH_DISABLE_WARNING(4251 4244, "-Wunknown-warning-option") // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
                                                               // 4244: 'return': conversion from 'qreal' to 'int', possible loss of data
#include <QContextMenuEvent>
AZ_POP_DISABLE_WARNING
#include <QtWidgets/QApplication>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    namespace ComponentEditorConstants
    {
        static const int kPropertyLabelWidth = 160; // Width of property label column in property grid.
        static const char* kUnknownComponentTitle = "-";

        // names for widgets so they can be found in stylesheet
        static const char* kPropertyEditorId = "PropertyEditor";

        static const int kAddComponentMenuHeight = 150;
    }

    template<typename T, typename Alloc>
    void AddUniqueItemToContainer(AZStd::vector<T, Alloc>& container, const T& element)
    {
        if (AZStd::find(container.begin(), container.end(), element) == container.end())
        {
            container.push_back(element);
        }
    }

    template<typename ContainerType1, typename ContainerType2>
    bool DoContainersIntersect(const ContainerType1& container1, const ContainerType2& container2)
    {
        return AZStd::find_first_of(container1.begin(), container1.end(), container2.begin(), container2.end()) != container1.end();
    }

    //generate a list of all valid or pending sibling components that are service-incompatible with the specified component
    AZStd::unordered_set<AZ::Component*> GetRelatedIncompatibleComponents(const AZ::Component* component1)
    {
        AZStd::unordered_set<AZ::Component*> incompatibleComponents;

        auto entity = component1 ? component1->GetEntity() : nullptr;
        if (entity)
        {
            auto componentDescriptor1 = AzToolsFramework::GetComponentDescriptor(component1);
            if (componentDescriptor1)
            {
                AZ::ComponentDescriptor::DependencyArrayType providedServices1;
                AZ::ComponentDescriptor::DependencyArrayType providedServices2;
                AZ::ComponentDescriptor::DependencyArrayType incompatibleServices1;
                AZ::ComponentDescriptor::DependencyArrayType incompatibleServices2;

                //get the list of required and incompatible services from the primary component
                componentDescriptor1->GetProvidedServices(providedServices1, nullptr);
                componentDescriptor1->GetIncompatibleServices(incompatibleServices1, nullptr);

                //build a list of all components attached to the entity
                AZ::Entity::ComponentArrayType allComponents = entity->GetComponents();

                //also include invalid components waiting for requirements to be met
                EditorPendingCompositionRequestBus::Event(entity->GetId(), &EditorPendingCompositionRequests::GetPendingComponents, allComponents);

                //for every component related to the entity, determine if its services are incompatible with the primary component
                for (auto component2 : allComponents)
                {
                    //don't test against itself
                    if (component1 == component2)
                    {
                        continue;
                    }

                    auto componentDescriptor2 = AzToolsFramework::GetComponentDescriptor(component2);
                    if (!componentDescriptor2)
                    {
                        continue;
                    }

                    //get the list of required and incompatible services for the comparison component
                    providedServices2.clear();
                    componentDescriptor2->GetProvidedServices(providedServices2, nullptr);

                    incompatibleServices2.clear();
                    componentDescriptor2->GetIncompatibleServices(incompatibleServices2, nullptr);

                    //if provided services overlap incompatible services for either component then add it to the list
                    if (DoContainersIntersect(providedServices1, incompatibleServices2) ||
                        DoContainersIntersect(providedServices2, incompatibleServices1))
                    {
                        incompatibleComponents.insert(component2);
                    }
                }
            }
        }
        return incompatibleComponents;
    }


    //generate a list of all valid or pending sibling components that are service-incompatible with the specified set of component
    AZStd::unordered_set<AZ::Component*> GetRelatedIncompatibleComponents(AZStd::span<AZ::Component* const> components)
    {
        AZStd::unordered_set<AZ::Component*> allIncompatibleComponents;
        for (auto component : components)
        {
            const AZStd::unordered_set<AZ::Component*> incompatibleComponents = GetRelatedIncompatibleComponents(component);
            allIncompatibleComponents.insert(incompatibleComponents.begin(), incompatibleComponents.end());
        }
        return allIncompatibleComponents;
    }

    ComponentEditor::ComponentEditor(
        AZ::SerializeContext* context,
        IPropertyEditorNotify* notifyTarget /* = nullptr */,
        QWidget* parent /* = nullptr */,
        ComponentAdapterFactory adapterFactory /* nullptr factory */)
        : AzQtComponents::Card(new ComponentEditorHeader(), parent)
        , m_serializeContext(context)
        , m_adapterFactory(adapterFactory)
    {
        GetHeader()->SetTitle(ComponentEditorConstants::kUnknownComponentTitle);

        m_adapter = m_adapterFactory();
        if (m_adapter)
        {
            m_filterAdapter = AZStd::make_shared<AZ::DocumentPropertyEditor::ValueStringFilter>();
            m_dpe = new DocumentPropertyEditor(this);

            // this DPE is going into a scroll area, don't allow it to provide its own scrollbar
            m_dpe->SetAllowVerticalScroll(false);
            m_dpe->SetEnforceMinWidth(false); // allow the DPE to get as small as the user prefers
            m_filterAdapter->SetSourceAdapter(m_adapter);
            m_dpe->SetAdapter(m_filterAdapter);
            connect(m_dpe, &DocumentPropertyEditor::ExpanderChangedByUser, this, &ComponentEditor::OnExpansionContractionDone);
            connect(m_dpe, &DocumentPropertyEditor::RequestSizeUpdate, this, &ComponentEditor::OnSizeUpdateRequested);
            setContentWidget(m_dpe);
        }
        else
        {
            // create property editor
            m_propertyEditor = new ReflectedPropertyEditor(this);
            m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
            m_propertyEditor->setObjectName(ComponentEditorConstants::kPropertyEditorId);
            m_propertyEditor->Setup(context, notifyTarget, false, ComponentEditorConstants::kPropertyLabelWidth, this);
            m_propertyEditor->SetHideRootProperties(true);
            m_propertyEditor->setProperty("ComponentBlock", true); // used by stylesheet
            connect(m_propertyEditor, &ReflectedPropertyEditor::OnExpansionContractionDone, this, &ComponentEditor::OnExpansionContractionDone);

            setContentWidget(m_propertyEditor);
        }

        m_savedKeySeed = AZ_CRC_CE("WorldEditorEntityEditor_Component");
        connect(this, &AzQtComponents::Card::expandStateChanged, this, &ComponentEditor::OnExpanderChanged);
        connect(GetHeader(), &ComponentEditorHeader::OnContextMenuClicked, this, &ComponentEditor::OnContextMenuClicked);
        connect(GetHeader(), &ComponentEditorHeader::iconLabelClicked, this, &ComponentEditor::OnIconLabelClicked);

        SetExpanded(true);
        SetSelected(false);
        SetDragged(false);
        SetDropTarget(false);
    }

    ComponentEditor::~ComponentEditor()
    {
    }

    void ComponentEditor::AddInstance(AZ::Component* componentInstance, AZ::Component* aggregateInstance, AZ::Component* compareInstance)
    {
        if (!componentInstance)
        {
            return;
        }

        // Note that in the case of a GenericComponentWrapper we give the editor
        // the GenericComponentWrapper rather than the underlying type.
        AZ::Uuid instanceTypeId = azrtti_typeid(componentInstance);

        m_components.push_back(componentInstance);

        if (m_adapter)
        {
            // aggregateInstance will be null unless we're adding to multi-edit
            if (!aggregateInstance)
            {
                // not in multi-edit, just set the adapter component to this instance.
                m_adapter->SetComponent(componentInstance);

                // if we were in multi-edit before, set the DPE back to the regular adapter and destroy the aggregate adapter
                if (m_aggregateAdapter)
                {
                    m_filterAdapter->SetSourceAdapter(m_adapter);
                    m_aggregateAdapter.reset();
                }
            }
            else
            {
                // there's an aggregateInstance, so we're in multi-edit. Create the AggregateAdapter if it doesn't exist yet
                if (!m_aggregateAdapter)
                {
                    m_aggregateAdapter = AZStd::make_shared<AZ::DocumentPropertyEditor::LabeledRowAggregateAdapter>();

                    // for now, disable "values differ rows", since there are so many pointer and opaque types in the Inspector
                    // and the output is noisy and unpleasant.
                    m_aggregateAdapter->SetGenerateDiffRows(false);

                    // add the original adapter which was already set in a prior AddInstance with a null aggregateInstance
                    m_aggregateAdapter->AddAdapter(m_adapter);
                    m_filterAdapter->SetSourceAdapter(m_aggregateAdapter);
                }

                // create the new ComponentAdapter for the componentInstance using the factory from our constructor
                auto newAdapter = m_adapterFactory();
                AZ_Assert(newAdapter, "m_adapterFactory should always return a valid ComponentAdapter in DPE mode!");
                newAdapter->SetComponent(componentInstance);
                m_aggregateAdapter->AddAdapter(newAdapter);
            }
        }
        else
        {
            // Use our seed to make a unique save state key for each entity, so the property editor is stored per-entity for the component
            // editor
            auto entityUniqueSavedStateKey = m_savedKeySeed;
            auto entityId = m_components[0]->GetEntityId();
            entityUniqueSavedStateKey.Add(reinterpret_cast<void*>(&entityId), sizeof(entityId));
            GetPropertyEditor()->SetSavedStateKey(entityUniqueSavedStateKey);

            GetPropertyEditor()->AddInstance(componentInstance, instanceTypeId, aggregateInstance, compareInstance);
        }

        // When first instance is set, use its data to fill out the header.
        if (m_componentType.IsNull())
        {
            SetComponentType(*componentInstance);
        }

        GetHeader()->setToolTip(BuildHeaderTooltip());
    }

    void ComponentEditor::ClearInstances(bool invalidateImmediately)
    {
        GetPropertyEditor()->SetDynamicEditDataProvider(nullptr);

        //clear warning flag and icon
        GetHeader()->SetWarning(false);
        GetHeader()->SetReadOnly(false);

        ClearNotifications();
        //clear component cache
        m_components.clear();

        GetPropertyEditor()->ClearInstances();
        if (invalidateImmediately)
        {
            GetPropertyEditor()->InvalidateAll();
        }

        InvalidateComponentType();

        GetHeader()->setToolTip(BuildHeaderTooltip());
    }

    void ComponentEditor::AddNotifications()
    {
        ClearNotifications();

        const auto& forwardPendingComponentInfo = GetPendingComponentInfoForAllComponents();
        bool hasForwardPCI =
            !forwardPendingComponentInfo.m_validComponentsThatAreIncompatible.empty() ||
            !forwardPendingComponentInfo.m_missingRequiredServices.empty() ||
            !forwardPendingComponentInfo.m_pendingComponentsWithRequiredServices.empty();

        const auto& reversePendingComponentInfo = GetPendingComponentInfoForAllComponentsInReverse();
        bool hasReversePCI =
            !reversePendingComponentInfo.m_validComponentsThatAreIncompatible.empty() ||
            !reversePendingComponentInfo.m_missingRequiredServices.empty() ||
            !reversePendingComponentInfo.m_pendingComponentsWithRequiredServices.empty();

        //if there were any issues then treat them as warnings
        bool isWarning = hasForwardPCI;
        bool isDisabled = AreAnyComponentsDisabled();
        bool isReadOnly = isWarning || isDisabled;

        //display warning icon if component isn't valid
        GetHeader()->SetWarning(isWarning);
        GetHeader()->SetReadOnly(isReadOnly);

        // If the Component is ReadOnly, set the mock disabled state on the Card
        mockDisabledState(isReadOnly);

        if (m_components.empty())
        {
            return;
        }

        if (m_components.size() > 1)
        {
            //if multiple components are represented then display a generic message
            if (hasForwardPCI || hasReversePCI)
            {
                CreateNotification("There is a component error on one or more entities in the selection. The selection cannot be modified until all errors have been resolved.");
            }
            return;
        }

        AZStd::map<QString, AZ::Entity::ComponentArrayType > uniqueMessages;
        //Go through all of the warnings and printing them as notifications.
        for (const auto& warning : forwardPendingComponentInfo.m_warnings)
        {
            CreateNotificationForWarningComponents(warning.c_str());
        }

        //display notification for conflicting components, including duplicates
        if (!reversePendingComponentInfo.m_validComponentsThatAreIncompatible.empty())
        {
            CreateNotificationForConflictingComponents(tr("This component conflicts with one or more duplicate or incompatible components on this entity. Those components have been disabled."), reversePendingComponentInfo.m_validComponentsThatAreIncompatible);
        }

        //display notification for conflicting components, including duplicates
        for (auto conflict : forwardPendingComponentInfo.m_validComponentsThatAreIncompatible)
        {
            QString message = tr("This component has been disabled because it is incompatible with \"%1\".").arg(AzToolsFramework::GetFriendlyComponentName(conflict).c_str());
            uniqueMessages[message].push_back(conflict);
        }
        for (const auto& message : uniqueMessages)
        {
            CreateNotificationForConflictingComponents(message.first, forwardPendingComponentInfo.m_validComponentsThatAreIncompatible);
        }
        uniqueMessages.clear();

        //display notification that one pending component might have requirements satisfied by another
        for (auto conflict : forwardPendingComponentInfo.m_pendingComponentsWithRequiredServices)
        {
            QString message = tr("This component is disabled pending \"%1\". This component will be enabled when the required component is resolved.").arg(AzToolsFramework::GetFriendlyComponentName(conflict).c_str());
            uniqueMessages[message].push_back(conflict);
        }
        for (const auto& message : uniqueMessages)
        {
            CreateNotification(message.first);
        }
        uniqueMessages.clear();

        //display notification with option to add components with required services
        if (!forwardPendingComponentInfo.m_missingRequiredServices.empty())
        {
            QString message = tr("This component is missing a required component service and has been disabled.");
            CreateNotificationForMissingComponents(
                message, 
                forwardPendingComponentInfo.m_missingRequiredServices, 
                forwardPendingComponentInfo.m_incompatibleServices);
        }
    }

    void ComponentEditor::ClearNotifications()
    {
        AzQtComponents::Card::clearNotifications();
    }

    AzQtComponents::CardNotification* ComponentEditor::CreateNotification(const QString& message)
    {
        return AzQtComponents::Card::addNotification(message);
    }

    AzQtComponents::CardNotification* ComponentEditor::CreateNotificationForConflictingComponents(const QString& message, const AZ::Entity::ComponentArrayType& conflictingComponents)
    {
        //TODO ask about moving all of these to the context menu instead
        auto notification = CreateNotification(message);
        auto featureButton = notification->addButtonFeature(tr("Delete component"));
        connect(featureButton, &QPushButton::clicked, this, [this]()
        {
            emit OnRequestRemoveComponents(GetComponents());
        });

#if 0 //disabling features until UX/UI is reworked or actions added to menu as stated above
        featureButton = notification->addButtonFeature(tr("Disable this component"));
        connect(featureButton, &QPushButton::clicked, this, [this]()
        {
            emit OnRequestDisableComponents(GetComponents());
        });

        featureButton = notification->addButtonFeature(tr("Remove incompatible components"));
        connect(featureButton, &QPushButton::clicked, this, [this]()
        {
            const AZStd::unordered_set<AZ::Component*> incompatibleComponents = GetRelatedIncompatibleComponents(GetComponents());
            emit OnRequestRemoveComponents(AZStd::vector<AZ::Component*>(incompatibleComponents.begin(), incompatibleComponents.end()));
        });
#endif

        //Activate/Enable are overloaded terms. "Disable incompatible components"
        featureButton = notification->addButtonFeature(tr("Activate this component"));

        // create a list copy that we can capture that will persist if/when the passed conflictingComponents list is destroyed
        AZ::Entity::ComponentArrayType listCopy = conflictingComponents;
        connect(featureButton, &QPushButton::clicked, this, [this, listCopy]()
        {
            emit OnRequestDisableComponents(AZStd::vector<AZ::Component*>(listCopy.begin(), listCopy.end()));
        });

        return notification;
    }

    AzQtComponents::CardNotification* ComponentEditor::CreateNotificationForMissingComponents(
        const QString& message, 
        const AZ::ComponentDescriptor::DependencyArrayType& services,
        const AZ::ComponentDescriptor::DependencyArrayType& incompatibleServices)
    {
        auto notification = CreateNotification(message);
        auto featureButton = notification->addButtonFeature(tr("Add Required Component \342\226\276"));
        connect(featureButton, &QPushButton::clicked, this, [this, featureButton, services, incompatibleServices]()
        {
            QRect screenRect(qApp->desktop()->availableGeometry(featureButton));
            QRect menuRect(
                featureButton->mapToGlobal(featureButton->rect().topLeft()),
                featureButton->mapToGlobal(featureButton->rect().bottomRight()));

            //place palette above or below button depending on how close to screen edge
            if (menuRect.bottom() + ComponentEditorConstants::kAddComponentMenuHeight > screenRect.bottom())
            {
                menuRect.setTop(menuRect.top() - ComponentEditorConstants::kAddComponentMenuHeight);
            }
            else
            {
                menuRect.setBottom(menuRect.bottom() + ComponentEditorConstants::kAddComponentMenuHeight);
            }

            emit OnRequestRequiredComponents(menuRect.topLeft(), menuRect.size(), services, incompatibleServices);
        });

        return notification;
    }

    AzQtComponents::CardNotification* ComponentEditor::CreateNotificationForWarningComponents(const QString& message)
    {
        AzQtComponents::CardNotification * notification = CreateNotification(message);

        return notification;
    }

    bool ComponentEditor::AreAnyComponentsDisabled() const
    {
        for (auto component : m_components)
        {
            auto entity = component->GetEntity();
            if (entity)
            {
                AZ::Entity::ComponentArrayType disabledComponents;
                EditorDisabledCompositionRequestBus::Event(entity->GetId(), &EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
                if (AZStd::find(disabledComponents.begin(), disabledComponents.end(), component) != disabledComponents.end())
                {
                    return true;
                }
            }
        }

        return false;
    }

    AzToolsFramework::EntityCompositionRequests::PendingComponentInfo ComponentEditor::GetPendingComponentInfoForAllComponents() const
    {
        AzToolsFramework::EntityCompositionRequests::PendingComponentInfo combinedPendingComponentInfo;
        for (auto component : m_components)
        {
            AzToolsFramework::EntityCompositionRequests::PendingComponentInfo pendingComponentInfo;
            AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &AzToolsFramework::EntityCompositionRequests::GetPendingComponentInfo, component);

            for (auto conflict : pendingComponentInfo.m_validComponentsThatAreIncompatible)
            {
                AddUniqueItemToContainer(combinedPendingComponentInfo.m_validComponentsThatAreIncompatible, conflict);
            }

            for (auto conflict : pendingComponentInfo.m_pendingComponentsWithRequiredServices)
            {
                AddUniqueItemToContainer(combinedPendingComponentInfo.m_pendingComponentsWithRequiredServices, conflict);
            }

            for (auto service : pendingComponentInfo.m_missingRequiredServices)
            {
                AddUniqueItemToContainer(combinedPendingComponentInfo.m_missingRequiredServices, service);
            }

            for (auto service : pendingComponentInfo.m_incompatibleServices)
            {
                AddUniqueItemToContainer(combinedPendingComponentInfo.m_incompatibleServices, service);
            }

            for (const auto &warning : pendingComponentInfo.m_warnings)
            {
                AddUniqueItemToContainer(combinedPendingComponentInfo.m_warnings, warning);
            }
        }

        return combinedPendingComponentInfo;
    }

    AzToolsFramework::EntityCompositionRequests::PendingComponentInfo ComponentEditor::GetPendingComponentInfoForAllComponentsInReverse() const
    {
        AzToolsFramework::EntityCompositionRequests::PendingComponentInfo combinedPendingComponentInfo;

        //gather all entities from current selection to find any components with issues referring to components represented by this editor
        EntityIdList selectedEntityIds;
        ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        AZ::Entity::ComponentArrayType componentsOnEntity;

        for (auto entityId : selectedEntityIds)
        {
            auto entity = GetEntityById(entityId);
            if (!entity)
            {
                continue;
            }

            //build a set of all active and pending components associated with the current entity
            componentsOnEntity.clear();
            AzToolsFramework::GetAllComponentsForEntity(entity, componentsOnEntity);

            for (auto component : componentsOnEntity)
            {
                //for each component, get any pending info/warnings
                AzToolsFramework::EntityCompositionRequests::PendingComponentInfo pendingComponentInfo;
                AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &AzToolsFramework::EntityCompositionRequests::GetPendingComponentInfo, component);

                //if pending info/warnings references displayed components then save the current component for reverse lookup
                if (DoContainersIntersect(pendingComponentInfo.m_validComponentsThatAreIncompatible, m_components))
                {
                    AddUniqueItemToContainer(combinedPendingComponentInfo.m_validComponentsThatAreIncompatible, component);
                }

                if (DoContainersIntersect(pendingComponentInfo.m_pendingComponentsWithRequiredServices, m_components))
                {
                    AddUniqueItemToContainer(combinedPendingComponentInfo.m_pendingComponentsWithRequiredServices, component);
                }
            }
        }

        return combinedPendingComponentInfo;
    }

    bool ComponentEditor::HasContents()
    {
        if (m_dpe)
        {
            return !m_filterAdapter->FilterIsActive() || !m_filterAdapter->IsEmpty() || GetHeader()->TitleMatchesFilter();
        }
        else
        {
            return (!GetPropertyEditor()->HasFilteredOutNodes() || GetPropertyEditor()->HasVisibleNodes());
        }
    }

    void ComponentEditor::SetFilterString(AZStd::string filterString)
    {
        if (m_dpe)
        {
            m_filterAdapter->SetFilterString(filterString);
        }
        else
        {
            GetPropertyEditor()->SetFilterString(filterString);
        }
        GetHeader()->SetFilterString(filterString);
    }

    void ComponentEditor::InvalidateAll(const char* filter)
    {
        GetPropertyEditor()->InvalidateAll(filter);
    }

    void ComponentEditor::QueuePropertyEditorInvalidation(PropertyModificationRefreshLevel refreshLevel)
    {
        GetPropertyEditor()->QueueInvalidation(refreshLevel);
    }

    void ComponentEditor::QueuePropertyEditorInvalidationForComponent(AZ::EntityComponentIdPair entityComponentIdPair, PropertyModificationRefreshLevel refreshLevel)
    {
        for (const auto component : m_components)
        {
            if ((component->GetId() == entityComponentIdPair.GetComponentId()) 
             && (component->GetEntityId() == entityComponentIdPair.GetEntityId()))
            {
                GetPropertyEditor()->QueueInvalidation(refreshLevel);
                break;
            }
        }
    }

    void ComponentEditor::CancelQueuedRefresh()
    {
        GetPropertyEditor()->CancelQueuedRefresh();
    }

    void ComponentEditor::PreventRefresh(bool shouldPrevent)
    {
        GetPropertyEditor()->PreventDataAccess(shouldPrevent);
    }

    void ComponentEditor::SetComponentOverridden(const bool overridden)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const auto entityId = m_components[0]->GetEntityId();
        AZ::SliceComponent::SliceInstanceAddress sliceInstanceAddress;
        AzFramework::SliceEntityRequestBus::EventResult(sliceInstanceAddress, entityId,
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

        const auto wasOverridden = GetHeader()->titleLabel()->property("IsOverridden");
        if (wasOverridden.toBool() != overridden)
        {
            auto sliceInstance = sliceInstanceAddress.GetInstance();
            if (sliceInstance)
            {
                GetHeader()->setTitleProperty("IsOverridden", overridden);
                GetHeader()->RefreshTitle();
            }
            else
            {
                if (wasOverridden.toBool() || !wasOverridden.isValid())
                {
                    GetHeader()->setTitleProperty("IsOverridden", false);
                    GetHeader()->RefreshTitle();
                }
            }
        }
    }

    void ComponentEditor::SetComponentType(const AZ::Component& componentInstance)
    {
        const AZ::Uuid& componentType = GetUnderlyingComponentType(componentInstance);

        auto classData = m_serializeContext->FindClassData(componentType);
        if (!classData || !classData->m_editData)
        {
            AZ_Warning("ComponentEditor", false, "Could not find reflection for component '%s'", classData->m_name);
            InvalidateComponentType();
            return;
        }

        m_componentType = componentType;

        GetPropertyEditor()->SetSavedStateKey(AZ::Crc32(componentType.ToString<AZStd::string>().data()), "componenteditor");

        GetHeader()->SetTitle(GetFriendlyComponentName(&componentInstance).c_str());

        GetHeader()->ClearHelpURL();

        if (classData->m_editData)
        {
            if (auto editorData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto nameAttribute = editorData->FindAttribute(AZ::Edit::Attributes::HelpPageURL))
                {
                    AZStd::string helpUrl;
                    AZ::AttributeReader nameReader(const_cast<AZ::Component*>(&componentInstance), nameAttribute);
                    nameReader.Read<AZStd::string>(helpUrl);
                    GetHeader()->SetHelpURL(helpUrl);
                }
            }
        }

        AZStd::string iconPath;
        AzToolsFramework::EditorRequestBus::BroadcastResult(iconPath, &AzToolsFramework::EditorRequests::GetComponentEditorIcon, componentType, const_cast<AZ::Component*>(&componentInstance));
        GetHeader()->SetIcon(QIcon(iconPath.c_str()));

        bool isExpanded = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isExpanded, componentInstance.GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsComponentExpanded, componentInstance.GetId());
        GetHeader()->SetExpanded(isExpanded);
    }

    void ComponentEditor::InvalidateComponentType()
    {
        m_componentType = AZ::Uuid::CreateNull();

        GetHeader()->SetTitle(ComponentEditorConstants::kUnknownComponentTitle);
        GetHeader()->SetIcon(QIcon());
    }

    QString ComponentEditor::BuildHeaderTooltip()
    {
        // Don't display tooltip if multiple entities / component instances are being displayed.
        if (m_components.size() != 1)
        {
            return QString();
        }

        // Prepare a tooltip showing useful metadata about this particular component,
        // such as dependencies on other services (and components) on the entity.

        AZ::Component* thisComponent = m_components.front();

        if (!thisComponent)
        {
            return QString();
        }

        AZ::ComponentDescriptor* componentDescriptor = nullptr;
        AZ::ComponentDescriptorBus::EventResult(
            componentDescriptor, thisComponent->RTTI_GetType(), &AZ::ComponentDescriptorBus::Events::GetDescriptor);

        if (!componentDescriptor)
        {
            return QString();
        }

        AZ::Entity* entity = thisComponent->GetEntity();

        if (!entity)
        {
            return QString();
        }

        const auto& components = entity->GetComponents();

        // Gather required services for the component.
        AZ::ComponentDescriptor::DependencyArrayType requiredServices;
        componentDescriptor->GetRequiredServices(requiredServices, thisComponent);

        QString tooltip;

        AZStd::list<AZStd::string> dependsOnComponents;

        // For each service, identify the component on the entity that is currently satisfying the requirement.
        for (const AZ::ComponentServiceType service : requiredServices)
        {
            for (AZ::Component* otherComponent : components)
            {
                if (otherComponent == thisComponent)
                {
                    continue;
                }

                AZ::ComponentDescriptor* otherDescriptor = nullptr;
                AZ::ComponentDescriptorBus::EventResult(
                    otherDescriptor, otherComponent->RTTI_GetType(), &AZ::ComponentDescriptorBus::Events::GetDescriptor);

                if (otherDescriptor)
                {
                    AZ::ComponentDescriptor::DependencyArrayType providedServices;
                    otherDescriptor->GetProvidedServices(providedServices, otherComponent);

                    if (AZStd::find(providedServices.begin(), providedServices.end(), service) != providedServices.end())
                    {
                        dependsOnComponents.emplace_back(GetFriendlyComponentName(otherComponent));
                        break;
                    }
                }
            }
        }

        if (!dependsOnComponents.empty())
        {
            tooltip += tr("Component has required services satisfied by the following components on this entity:");
            tooltip += "\n";

            for (const AZStd::string& componentName : dependsOnComponents)
            {
                tooltip += "\n\t";
                tooltip += componentName.c_str();
            }
        }

        return tooltip;
    }

    void ComponentEditor::OnExpanderChanged(bool expanded)
    {
        for (auto component : m_components)
        {
            EditorEntityInfoRequestBus::Event(component->GetEntityId(), &EditorEntityInfoRequestBus::Events::SetComponentExpanded, component->GetId(), expanded);
        }

        emit OnExpansionContractionDone();
    }

    void ComponentEditor::OnContextMenuClicked(const QPoint& position)
    {
        emit OnDisplayComponentEditorMenu(position);
    }

    void ComponentEditor::contextMenuEvent(QContextMenuEvent *event)
    {
        OnContextMenuClicked(event->globalPos());
        event->accept();
    }

    void ComponentEditor::OnIconLabelClicked(const QPoint& position)
    {
        emit OnComponentIconClicked(position);
    }

    void ComponentEditor::UpdateExpandability()
    {
        //updating whether or not expansion is allowed and forcing collapse if it's not
        bool isExpandable = IsExpandable();
        GetHeader()->SetExpandable(isExpandable);
        SetExpanded(IsExpanded() && isExpandable);
    }

    void ComponentEditor::SetExpanded(bool expanded)
    {
        AzQtComponents::Card::setExpanded(expanded);
    }

    bool ComponentEditor::IsExpanded() const
    {
        return GetHeader()->IsExpanded();
    }

    bool ComponentEditor::IsExpandable() const
    {
        //if there are any notifications, expansion must be allowed
        if (getNotificationCount())
        {
            return true;
        }

        //if any component class data has data elements or fields then it should be expandable
        int classElementCount = 0;
        int dataElementCount = 0;

        for (auto component : m_components)
        {
            auto typeId = GetComponentTypeId(component);

            //build a vector of all class info associated with typeId
            //EnumerateBase doesn't include class data for requested type so it must be added manually
            AZStd::vector<const AZ::SerializeContext::ClassData*> classDataVec = { GetComponentClassData(component) };

            //get all class info for any base classes (light components share a common base class where all properties are set)
            m_serializeContext->EnumerateBase(
                [&classDataVec](const AZ::SerializeContext::ClassData* classData, AZ::Uuid) { classDataVec.push_back(classData); return true; }, typeId);

            for (auto classData : classDataVec)
            {
                if (!classData || !classData->m_editData)
                {
                    continue;
                }

                //count all visible class and data elements
                for (const auto& element : classData->m_editData->m_elements)
                {
                    bool visible = true;
                    if (auto visibilityAttribute = element.FindAttribute(AZ::Edit::Attributes::Visibility))
                    {
                        AzToolsFramework::PropertyAttributeReader reader(component, visibilityAttribute);
                        AZ::u32 visibilityValue;
                        if (reader.Read<AZ::u32>(visibilityValue))
                        {
                            if (visibilityValue == AZ_CRC_CE("PropertyVisibility_Hide"))
                            {
                                visible = false;
                            }
                        }
                    }

                    if (visible)
                    {
                        if (element.IsClassElement())
                        {
                            classElementCount++;
                        }
                        else
                        {
                            dataElementCount++;
                        }
                    }

                    if (classElementCount > 1 || dataElementCount > 0)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void ComponentEditor::SetSelected(bool selected)
    {
        // do not allow cards to be selected when they are disabled
        // (show the yellow outline)
        if (!isEnabled())
        {
            return;
        }

        AzQtComponents::Card::setSelected(selected);
    }

    bool ComponentEditor::IsSelected() const
    {
        return AzQtComponents::Card::isSelected();
    }

    void ComponentEditor::SetDragged(bool dragged)
    {
        AzQtComponents::Card::setDragging(dragged);
    }

    bool ComponentEditor::IsDragged() const
    {
        return AzQtComponents::Card::isDragging();
    }

    void ComponentEditor::SetDropTarget(bool dropTarget)
    {
        AzQtComponents::Card::setDropTarget(dropTarget);
    }

    bool ComponentEditor::IsDropTarget() const
    {
        return AzQtComponents::Card::isDropTarget();
    }

    ComponentEditorHeader* ComponentEditor::GetHeader() const
    {
        return static_cast<ComponentEditorHeader*>(header());
    }

    AzToolsFramework::IPropertyEditor* ComponentEditor::GetPropertyEditor()
    {
        if (m_propertyEditor)
        {
            return m_propertyEditor;
        }
        else
        {
            return m_dpe;
        }
    }

    AZStd::vector<AZ::Component*>& ComponentEditor::GetComponents()
    {
        return m_components;
    }

    const AZStd::vector<AZ::Component*>& ComponentEditor::GetComponents() const
    {
        return m_components;
    }

    void ComponentEditor::VisitComponentAdapterContents(const VisitComponentAdapterContentsCallback& callback) const
    {
        callback(m_adapter->GetContents());
    }

    bool ComponentEditor::HasComponentWithId(AZ::ComponentId componentId)
    {
        for (AZ::Component* component : m_components)
        {
            if (component->GetId() == componentId)
            {
                return true;
            }
        }

        return false;
    }

    void ComponentEditor::EnteredComponentMode(const AZStd::vector<AZ::Uuid>& componentModeTypes)
    {
        if (AZStd::find(componentModeTypes.begin(), componentModeTypes.end(), m_componentType) == componentModeTypes.end())
        {
            // disable all component cards that aren't in the active component mode.
            SetWidgetInteractEnabled(this, false);
        }

        // for components that *are* in the active component mode, try to set the first one
        // to selected, and the rest to unselected.
        SetSelected(componentModeTypes.front() == m_componentType);
    }

    void ComponentEditor::LeftComponentMode(const AZStd::vector<AZ::Uuid>& componentModeTypes)
    {
        // restore all component cards to normal when leaving ComponentMode
        if (AZStd::find(
            componentModeTypes.begin(),
            componentModeTypes.end(), m_componentType) == componentModeTypes.end())
        {
            SetWidgetInteractEnabled(this, true);
        }

        SetSelected(false);
    }

    void ComponentEditor::ActiveComponentModeChanged(const AZ::Uuid& componentType)
    {
        // refresh which Component Editor/Card looks selected in the Entity Outliner
        SetSelected(componentType == m_componentType);
    }

    void ComponentEditor::ConnectPropertyChangeHandler(
        const AZStd::function<void(const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo& changeInfo)>& callback)
    {
        m_propertyChangeHandler = AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeEvent::Handler(callback);
        m_adapter->ConnectPropertyChangeHandler(m_propertyChangeHandler);
    }
}

#include "UI/PropertyEditor/moc_ComponentEditor.cpp"
