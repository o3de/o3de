/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyEntityIdCtrl.hxx"
#include "PropertyQTConstants.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Casting/lossy_cast.h>

#include <AzQtComponents/Components/Widgets/LineEdit.h>

#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityIdQLineEdit.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextPickingBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorPickEntitySelection.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtWidgets/QHBoxLayout>
AZ_POP_DISABLE_WARNING
#include <QtCore/QMimeData>
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
#include <QDragEnterEvent>
AZ_POP_DISABLE_WARNING
#include <QDropEvent>
#include <QToolButton>
#include <QtWidgets/QApplication>

//just a test to see how it would work to pop a dialog

namespace AzToolsFramework
{
    PropertyEntityIdCtrl::PropertyEntityIdCtrl(QWidget* pParent)
        : QWidget(pParent)
        , m_acceptedEntityContextId(AzFramework::EntityContextId::CreateNull())
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);

        pLayout->setContentsMargins(0, 0, 0, 0);
        pLayout->setSpacing(2);

        m_entityIdLineEdit = aznew EntityIdQLineEdit(this);
        m_entityIdLineEdit->setFocusPolicy(Qt::StrongFocus);

        m_pickButton = new QToolButton(this);
        m_pickButton->setCheckable(true);
        m_pickButton->setChecked(false);
        m_pickButton->setAutoRaise(true);
        m_pickButton->setContentsMargins(0, 0, 0, 0);
        m_pickButton->setIcon(QIcon(":/stylesheet/img/UI20/picker.svg"));
        m_pickButton->setToolTip("Pick an object in the viewport");
        m_pickButton->setMouseTracking(true);

        pLayout->addWidget(m_entityIdLineEdit);
        pLayout->addWidget(m_pickButton);

        setFocusProxy(m_entityIdLineEdit);
        setFocusPolicy(m_entityIdLineEdit->focusPolicy());

        setLayout(pLayout);

        setAcceptDrops(true);

        m_pickerIcon = QIcon(":/Cursors/Pointer.svg");

        connect(m_entityIdLineEdit, &EntityIdQLineEdit::RequestPickObject, this, [this]()
        {
            if (!m_pickButton->isChecked())
            {
                StartEntityPickMode();
            }
            else
            {
                StopEntityPickMode();
            }
        });

        connect(m_pickButton, &QToolButton::clicked, this, [this]()
        {
            if (m_pickButton->isChecked())
            {
                StartEntityPickMode();
            }
            else
            {
                StopEntityPickMode();
            }
        });

        QToolButton* clearButton = AzQtComponents::LineEdit::getClearButton(m_entityIdLineEdit);
        assert(clearButton);
        connect(clearButton, &QToolButton::clicked, this, &PropertyEntityIdCtrl::ClearEntityId);
    }

    void PropertyEntityIdCtrl::StartEntityPickMode()
    {
        EditorPickModeRequestBus::Broadcast(&EditorPickModeRequests::StopEntityPickMode);

        QApplication::setOverrideCursor(QCursor(m_pickerIcon.pixmap(16, 16), 4, 1));
        if (!EditorPickModeRequestBus::Handler::BusIsConnected())
        {
            AzFramework::EntityContextId pickModeEntityContextId = GetPickModeEntityContextId();

            emit OnPickStart();
            m_pickButton->setChecked(true);
            EditorPickModeRequestBus::Handler::BusConnect(pickModeEntityContextId);
            EditorEventsBus::Handler::BusConnect();

            // Replace the default input handler with one specific for dealing with entity selection in the viewport,
            // but only if we're not in component mode.
            // If we're in component mode, we should continue to use the default handler and let the component mode handling
            // manage the entity picking.
            if (!AzToolsFramework::ComponentModeFramework::InComponentMode())
            {
                EditorInteractionSystemViewportSelectionRequestBus::Event(
                    GetEntityContextId(),
                    &EditorInteractionSystemViewportSelection::SetHandler,
                    [](const EditorVisibleEntityDataCacheInterface* entityDataCache,
                       ViewportEditorModeTrackerInterface* viewportEditorModeTracker)
                    {
                        return AZStd::make_unique<EditorPickEntitySelection>(entityDataCache, viewportEditorModeTracker);
                    });
            }

            if (!pickModeEntityContextId.IsNull())
            {
                EditorPickModeNotificationBus::Event(
                    pickModeEntityContextId, &EditorPickModeNotifications::OnEntityPickModeStarted);
            }
            else
            {
                // Broadcast if the entity context is unknown
                EditorPickModeNotificationBus::Broadcast(&EditorPickModeNotifications::OnEntityPickModeStarted);
            }
        }
    }

    AzFramework::EntityContextId PropertyEntityIdCtrl::GetPickModeEntityContextId()
    {
        return m_acceptedEntityContextId;
    }

    void PropertyEntityIdCtrl::StopEntityPickMode()
    {
        QApplication::restoreOverrideCursor();

        if (EditorPickModeRequestBus::Handler::BusIsConnected())
        {
            m_pickButton->setChecked(false);
            EditorPickModeRequestBus::Handler::BusDisconnect();
            EditorEventsBus::Handler::BusDisconnect();
            emit OnPickComplete();

            // If we changed the handler to the pick handler (i.e. we're not in component mode),
            // then return to the default viewport editor selection now that we're done picking.
            if (!AzToolsFramework::ComponentModeFramework::InComponentMode())
            {
                EditorInteractionSystemViewportSelectionRequestBus::Event(
                    GetEntityContextId(), &EditorInteractionSystemViewportSelection::SetDefaultHandler);
            }

            EditorPickModeNotificationBus::Broadcast(&EditorPickModeNotifications::OnEntityPickModeStopped);
        }
    }

    void PropertyEntityIdCtrl::PickModeSelectEntity(const AZ::EntityId id)
    {
        if (id.IsValid())
        {
            SetCurrentEntityId(id, true, "");
        }
    }

    void PropertyEntityIdCtrl::OnEscape()
    {
        StopEntityPickMode();
    }

    void PropertyEntityIdCtrl::dragLeaveEvent(QDragLeaveEvent* event)
    {
        (void)event;
        AzQtComponents::LineEdit::removeDropTargetStyle(m_entityIdLineEdit);
    }

    void PropertyEntityIdCtrl::dragEnterEvent(QDragEnterEvent* event)
    {
        if (event == nullptr)
        {
            return;
        }

        if (!AllowsDrop())
        {
            return;
        }

        const bool isValidDropTarget = IsCorrectMimeData(event->mimeData()) &&
            EntityIdsFromMimeData(*event->mimeData());

        AzQtComponents::LineEdit::applyDropTargetStyle(m_entityIdLineEdit, isValidDropTarget);

        if (isValidDropTarget)
        {
            event->acceptProposedAction();
        }
    }

    void PropertyEntityIdCtrl::dropEvent(QDropEvent* event)
    {
        AzQtComponents::LineEdit::removeDropTargetStyle(m_entityIdLineEdit);

        if (!AllowsDrop())
        {
            return;
        }

        if (event == nullptr)
        {
            return;
        }
        if (!IsCorrectMimeData(event->mimeData()))
        {
            return;
        }

        AzToolsFramework::EditorEntityIdContainer entityIdListContainer;

        if (EntityIdsFromMimeData(*event->mimeData(), &entityIdListContainer))
        {
            SetCurrentEntityId(entityIdListContainer.m_entityIds[0], true, "");

            if (entityIdListContainer.m_entityIds.size() > 1)
            {
                // Pop the first element off the list (handled above) and then request new elements for the rest of the list
                entityIdListContainer.m_entityIds.erase(entityIdListContainer.m_entityIds.begin());

                // Need to pass this up to my parent if it is a container to properly handle this request
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::AddElementsToParentContainer, const_cast<PropertyEntityIdCtrl*>(this), entityIdListContainer.m_entityIds.size(),
                    [&entityIdListContainer](void* dataPtr, const AZ::SerializeContext::ClassElement* classElement, bool noDefaultData, AZ::SerializeContext*) -> bool
                {
                    (void)noDefaultData;
                    if (classElement->m_typeId == azrtti_typeid<AZ::EntityId>())
                    {
                        *reinterpret_cast<AZ::EntityId*>(dataPtr) = entityIdListContainer.m_entityIds[0];
                        entityIdListContainer.m_entityIds.erase(entityIdListContainer.m_entityIds.begin());
                        return true;
                    }
                    return false;
                }
                );
            }

            event->acceptProposedAction();
        }
    }

    bool PropertyEntityIdCtrl::IsCorrectMimeData(const QMimeData* mimeData) const
    {
        if (mimeData == nullptr ||
            !mimeData->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()))
        {
            return false;
        }

        return true;
    }

    bool PropertyEntityIdCtrl::EntityIdsFromMimeData(const QMimeData& mimeData, AzToolsFramework::EditorEntityIdContainer* entityIdListContainer) const
    {
        if (!IsCorrectMimeData(&mimeData))
        {
            return false;
        }

        QByteArray arrayData = mimeData.data(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

        AzToolsFramework::EditorEntityIdContainer tempContainer;

        if (!entityIdListContainer)
        {
            entityIdListContainer = &tempContainer;
        }

        if (!entityIdListContainer->FromBuffer(arrayData.constData(), arrayData.size()))
        {
            return false;
        }

        if (entityIdListContainer->m_entityIds.empty())
        {
            return false;
        }

        if (!m_acceptedEntityContextId.IsNull())
        {
            // Check that the entity's owning context matches the one that this control accepts
            AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
            AzFramework::EntityIdContextQueryBus::EventResult(contextId, entityIdListContainer->m_entityIds[0], &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);

            if (contextId != m_acceptedEntityContextId)
            {
                return false;
            }
        }

        return true;
    }

    AZ::EntityId PropertyEntityIdCtrl::GetEntityId() const
    {
        return m_entityIdLineEdit->GetEntityId();
    }

    PropertyEntityIdCtrl::~PropertyEntityIdCtrl()
    {
        StopEntityPickMode();
    }

    QWidget* PropertyEntityIdCtrl::GetFirstInTabOrder()
    {
        return m_entityIdLineEdit;
    }
    QWidget* PropertyEntityIdCtrl::GetLastInTabOrder()
    {
        return m_entityIdLineEdit;
    }

    void PropertyEntityIdCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }

    bool PropertyEntityIdCtrl::SetChildWidgetsProperty(const char* name, const QVariant& variant)
    {
        bool success = m_entityIdLineEdit->setProperty(name, variant);
        success = m_pickButton->setProperty(name, variant) && success;

        return success;
    }

    void PropertyEntityIdCtrl::SetCurrentEntityId(const AZ::EntityId& newEntityId, bool emitChange, const AZStd::string& nameOverride)
    {
        m_entityIdLineEdit->setClearButtonEnabled(HasClearButton());
        m_entityIdLineEdit->SetEntityId(newEntityId, nameOverride);
        m_componentsSatisfyingServices.clear();

        if (!HasPickButton())
        {
            m_pickButton->hide();
        }
        else
        {
            m_pickButton->show();
        }

        if (!m_requiredServices.empty() || !m_incompatibleServices.empty())
        {
            bool servicesMismatch = false;

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, newEntityId);

            if (entity)
            {
                AZ::ComponentDescriptor::DependencyArrayType unmatchedServices(m_requiredServices);
                bool foundIncompatibleService = false;
                const auto& components = entity->GetComponents();
                for (const AZ::Component* component : components)
                {
                    AZ::ComponentDescriptor* componentDescriptor = nullptr;
                    AZ::ComponentDescriptorBus::EventResult(
                        componentDescriptor, component->RTTI_GetType(), &AZ::ComponentDescriptorBus::Events::GetDescriptor);
                    AZ::ComponentDescriptor::DependencyArrayType providedServices;
                    componentDescriptor->GetProvidedServices(providedServices, component);
                    for (int serviceIdx = azlossy_cast<int>(unmatchedServices.size() - 1); serviceIdx >= 0; --serviceIdx)
                    {
                        AZ::ComponentServiceType service = unmatchedServices[serviceIdx];
                        if (AZStd::find(providedServices.begin(), providedServices.end(), service) != providedServices.end())
                        {
                            m_componentsSatisfyingServices.emplace_back(AzToolsFramework::GetFriendlyComponentName(component));
                            unmatchedServices.erase(unmatchedServices.begin() + serviceIdx);
                        }
                    }

                    for (const auto& service : providedServices)
                    {
                        if (AZStd::find(m_incompatibleServices.begin(), m_incompatibleServices.end(), service) != m_incompatibleServices.end())
                        {
                            foundIncompatibleService = true;
                            break;
                        }
                    }

                    if (foundIncompatibleService)
                    {
                        break;
                    }
                }

                servicesMismatch = !unmatchedServices.empty() || foundIncompatibleService;
            }

            SetMismatchedServices(servicesMismatch);
        }

        QString tip = BuildTooltip();
        if (!tip.isEmpty())
        {
            m_entityIdLineEdit->setToolTip(tip);
        }

        if (emitChange)
        {
            emit OnEntityIdChanged(newEntityId);
        }
    }

    void PropertyEntityIdCtrl::ClearEntityId()
    {
        SetCurrentEntityId(AZ::EntityId(), true, "");
    }

    QString PropertyEntityIdCtrl::BuildTooltip()
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_entityIdLineEdit->GetEntityId());

        if (!entity)
        {
            return QString();
        }

        QString tooltip;

        QVariant mismatchedServices = property("MismatchedServices");
        if (mismatchedServices == true)
        {
            tooltip = "Service requirements are not met by the referenced entity.";
        }
        else if (!m_componentsSatisfyingServices.empty())
        {
            tooltip = tr("Required services are satisfied by the following components on ");
            tooltip += entity->GetName().c_str();
            tooltip += ":\n";

            for (const AZStd::string& componentName : m_componentsSatisfyingServices)
            {
                tooltip += "\n\t";
                tooltip += componentName.c_str();
            }
        }

        return tooltip;
    }

    void PropertyEntityIdCtrl::SetRequiredServices(AZStd::span<const AZ::ComponentServiceType> requiredServices)
    {
        m_requiredServices.assign(requiredServices.begin(), requiredServices.end());
    }

    void PropertyEntityIdCtrl::SetIncompatibleServices(AZStd::span<const AZ::ComponentServiceType> incompatibleServices)
    {
        m_incompatibleServices.assign(incompatibleServices.begin(), incompatibleServices.end());
    }

    void PropertyEntityIdCtrl::SetMismatchedServices(bool mismatchedServices)
    {
        if (property("MismatchedServices") != mismatchedServices)
        {
            setProperty("MismatchedServices", mismatchedServices);
            style()->unpolish(this);
            style()->polish(this);
            update();

            m_entityIdLineEdit->style()->unpolish(m_entityIdLineEdit);
            m_entityIdLineEdit->style()->polish(m_entityIdLineEdit);
            m_entityIdLineEdit->update();
        }
    }

    void PropertyEntityIdCtrl::SetAcceptedEntityContext(AzFramework::EntityContextId contextId)
    {
        m_acceptedEntityContextId = contextId;

        // Update pick button visibility depending on whether the entity context supports viewport picking
        bool supportsViewportEntityIdPicking = true;
        if (!m_acceptedEntityContextId.IsNull())
        {
            AzToolsFramework::EditorEntityContextPickingRequestBus::EventResult(
                supportsViewportEntityIdPicking,
                m_acceptedEntityContextId,
                &AzToolsFramework::EditorEntityContextPickingRequestBus::Events::SupportsViewportEntityIdPicking);
        }
        m_pickButton->setVisible(supportsViewportEntityIdPicking);
    }

    QWidget* EntityIdPropertyHandler::CreateGUI(QWidget* pParent)
    {
        PropertyEntityIdCtrl* newCtrl = aznew PropertyEntityIdCtrl(pParent);
        connect(newCtrl, &PropertyEntityIdCtrl::OnEntityIdChanged, this, [newCtrl](AZ::EntityId)
            {
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });
        return newCtrl;
    }

    void EntityIdPropertyHandler::ConsumeAttribute(PropertyEntityIdCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        (void)GUI;
        (void)attrib;
        (void)attrValue;
        (void)debugName;

        if (attrib == AZ::Edit::Attributes::RequiredService)
        {
            AZ::ComponentServiceType requiredService = 0;
            AZ::ComponentDescriptor::DependencyArrayType requiredServices;
            if (attrValue->template Read<AZ::ComponentServiceType>(requiredService))
            {
                GUI->SetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType({ requiredService }));
            }
            else if (attrValue->template Read<decltype(requiredServices)>(requiredServices))
            {
                GUI->SetRequiredServices(requiredServices);
            }
        }
        else if (attrib == AZ::Edit::Attributes::IncompatibleService)
        {
            AZ::ComponentServiceType incompatibleService = 0;
            AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
            if (attrValue->template Read<AZ::ComponentServiceType>(incompatibleService))
            {
                GUI->SetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType({ incompatibleService }));
            }
            else if (attrValue->template Read<decltype(incompatibleServices)>(incompatibleServices))
            {
                GUI->SetIncompatibleServices(incompatibleServices);
            }
        }
        else if (attrib == AZ::Edit::Attributes::ShowClearButtonHandler)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->SetHasClearButton(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::ShowPickButton)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->SetHasPickButton(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::AllowDrop)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->SetAllowsDrop(value);
            }
        }
    }

    void EntityIdPropertyHandler::WriteGUIValuesIntoProperty(size_t index, PropertyEntityIdCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        instance = GUI->GetEntityId();
    }

    bool EntityIdPropertyHandler::ReadValuesIntoGUI(size_t index, PropertyEntityIdCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        // Find the entity that is associated with the property
        AZ::EntityId entityId;
        while (node)
        {
            if ((node->GetClassMetadata()) && (node->GetClassMetadata()->m_azRtti))
            {
                AZ::IRttiHelper* rtti = node->GetClassMetadata()->m_azRtti;
                if (rtti->IsTypeOf<AZ::Component>())
                {
                    AZ::Entity* entity = rtti->Cast<AZ::Component>(node->GetInstance(index))->GetEntity();
                    if (entity)
                    {
                        entityId = entity->GetId();
                        if (entityId.IsValid())
                        {
                            break;
                        }
                    }
                }
            }

            node = node->GetParent();
        }

        // Tell the GUI which entity context it should accept
        AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
        if (entityId.IsValid())
        {
            AzFramework::EntityIdContextQueryBus::EventResult(
                contextId, entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);
        }
        GUI->SetAcceptedEntityContext(contextId);

        GUI->SetCurrentEntityId(instance, false, "");
        return false;
    }

    void RegisterEntityIdPropertyHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew EntityIdPropertyHandler());
    }

}

#include "UI/PropertyEditor/moc_PropertyEntityIdCtrl.cpp"
