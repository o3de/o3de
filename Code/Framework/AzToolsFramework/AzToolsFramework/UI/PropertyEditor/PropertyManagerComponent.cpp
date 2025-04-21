/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyManagerComponent.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrlTypes.h>
#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>

namespace AzToolsFramework
{
    void RegisterIntSpinBoxHandlers();
    void RegisterIntSliderHandlers();
    void RegisterFilePathHandler();
    void RegisterDoubleSpinBoxHandlers();
    void RegisterDoubleSliderHandlers();
    void RegisterColorPropertyHandlers();
    void RegisterStringLineEditHandler();
    void RegisterBoolComboBoxHandler();
    void RegisterCheckBoxHandlers();
    void RegisterBoolRadioButtonsHandler();
    void RegisterEnumComboBoxHandler();
    void RegisterStringComboBoxHandler();
    void RegisterAssetPropertyHandler();
    void RegisterAudioPropertyHandler();
    void RegisterEntityIdPropertyHandler();
    void RegisterVectorHandlers();
    void RegisterButtonPropertyHandlers();
    void RegisterMultiLineEditHandler();
    void RegisterCrcHandler();
    void ReflectPropertyEditor(AZ::ReflectContext* context);
    void RegisterExeSelectPropertyHandler();
    void RegisterLabelHandler();

    namespace Components
    {
        PropertyManagerComponent::PropertyManagerComponent()
        {
        }
        PropertyManagerComponent::~PropertyManagerComponent()
        {
        }

        void PropertyManagerComponent::Init()
        {
        }

        void PropertyManagerComponent::Activate()
        {
            PropertyTypeRegistrationMessages::Bus::Handler::BusConnect();
            PropertyEditorGUIMessages::Bus::Handler::BusConnect();
            m_dpeSystem = AZStd::make_unique<PropertyEditorToolsSystem>();

            CreateBuiltInHandlers();
        }

        void PropertyManagerComponent::Deactivate()
        {
            // Delete all remaining auto-delete or built-in handlers.
            for (auto it = m_builtInHandlers.begin(); it != m_builtInHandlers.end(); ++it)
            {
#ifdef AZ_DEBUG_BUILD
                // For debug builds, we'll take the extra time to delete each handler that we're deleting from m_Handlers.
                // We loop through m_Handlers below to ensure that we don't have any other handlers still registered after
                // we've deleted these.
                AZStd::erase_if(m_Handlers, [it](const auto& item) {
                    auto const& [key, value] = item;
                    return (key == (*it)->GetHandlerName()) && (value == (*it));
                });
#endif

                delete *it;
            }

            m_builtInHandlers.clear();


#ifdef AZ_DEBUG_BUILD
            // Loop through all the remaining registered handlers (if any) and print out an error, as these all are probably memory
            // leaks.  UnregisterPropertyType should have been called on these already, and their pointers should have been deleted
            // by the caller.
            auto it = m_Handlers.begin();
            while (it != m_Handlers.end())
            {
                AZ_Error("PropertyManager", false, "Property Handler 0x%08x is still registered during shutdown", it->first);
                ++it;
            }
#endif

            m_Handlers.clear();
            m_DefaultHandlers.clear();

            m_dpeSystem.reset();
            PropertyEditorGUIMessages::Bus::Handler::BusDisconnect();
            PropertyTypeRegistrationMessages::Bus::Handler::BusDisconnect();
        }

        void PropertyManagerComponent::RegisterPropertyType(PropertyHandlerBase* pHandler)
        {
    #ifdef _DEBUG
            auto it = m_Handlers.find(pHandler->GetHandlerName());

            while ((it != m_Handlers.end()) && (it->first == pHandler->GetHandlerName()))
            {
                const PropertyHandlerBase* currentHandler = it->second;
                if ((currentHandler->GetHandledType() == pHandler->GetHandledType()) && (currentHandler->Priority() == pHandler->Priority()))
                {
                    AZ_Error("PropertyManager", false, "Attempt to register another handler for the exact same type and name and priority as 0x%08x", pHandler->GetHandlerName());
                    return;
                }
                ++it;
            }
    #endif
            pHandler->RegisterDpeHandler();

            auto propertyEditorSystemInterface = AZ::Interface<AZ::DocumentPropertyEditor::PropertyEditorSystemInterface>::Get();
            AZ_Assert(propertyEditorSystemInterface,
                "PropertyEditorSystemInterface was nullptr when attempting to register property handler adapter elements");
            pHandler->RegisterWithPropertySystem(propertyEditorSystemInterface);

            m_Handlers.insert(AZStd::make_pair(pHandler->GetHandlerName(), pHandler));

            if (pHandler->IsDefaultHandler())
            {
                m_DefaultHandlers.insert(AZStd::make_pair(pHandler->GetHandledType(), pHandler));
            }

            if (pHandler->AutoDelete())
            {
                m_builtInHandlers.push_back(pHandler);
            }
        }

        void PropertyManagerComponent::UnregisterPropertyType(PropertyHandlerBase* pHandler)
        {
            pHandler->UnregisterDpeHandler();

            bool foundIt = false;
            auto it = m_Handlers.find(pHandler->GetHandlerName());
            while ((it != m_Handlers.end()) && (it->first == pHandler->GetHandlerName()))
            {
                if (it->second == pHandler)
                {
                    foundIt = true;
                    m_Handlers.erase(it);
                    break;
                }
                ++it;
            }

            if (!foundIt)
            {
                AZ_Error("PropertyManager", false, "Attempt to unregister handler 0x%08x which is not currently registered", pHandler->GetHandlerName());
            }

            auto defaultIt = m_DefaultHandlers.find(pHandler->GetHandledType());
            while ((defaultIt != m_DefaultHandlers.end()) && (defaultIt->first == pHandler->GetHandledType()))
            {
                if (defaultIt->second == pHandler)
                {
                    m_DefaultHandlers.erase(defaultIt);
                    break;
                }
                ++defaultIt;
            }

            if (pHandler->AutoDelete())
            {
                m_builtInHandlers.erase(AZStd::remove(m_builtInHandlers.begin(), m_builtInHandlers.end(), pHandler),
                    m_builtInHandlers.end());
                AZ_Assert(false,
                    "Handlers with AutoDelete set should not call UnregisterPropertyType.  To fix, do one of the following:\n"
                    "  1. Set AutoDelete to false in the handler, call UnregisterPropertyType, and the caller should delete the handler.\n"
                    "  2. Set AutoDelete to true in the handler and do NOT call UnregisterPropertyType or delete the handler.");
            }
        }

        void PropertyManagerComponent::RequestWrite(QWidget* editorGUI)
        {
            if (m_currentUndoBatch)
            {
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                    m_currentUndoBatch,
                    &AzToolsFramework::ToolsApplicationRequests::ResumeUndoBatch,
                    m_currentUndoBatch,
                    "Modify Property");
            }
            else
            {
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                    m_currentUndoBatch, &AzToolsFramework::ToolsApplicationRequests::BeginUndoBatch, "Modify Property");
            }

            IndividualPropertyHandlerEditNotifications::Bus::Event(
                editorGUI, &IndividualPropertyHandlerEditNotifications::Bus::Events::OnValueChanged,
                AZ::DocumentPropertyEditor::Nodes::ValueChangeType::InProgressEdit);
        }

        void PropertyManagerComponent::OnEditingFinished(QWidget* editorGUI)
        {
            IndividualPropertyHandlerEditNotifications::Bus::Event(
                editorGUI, &IndividualPropertyHandlerEditNotifications::Bus::Events::OnValueChanged,
                AZ::DocumentPropertyEditor::Nodes::ValueChangeType::FinishedEdit);

            if (m_currentUndoBatch)
            {
                AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::EndUndoBatch);
                m_currentUndoBatch = nullptr;
            }
        }

        void PropertyManagerComponent::RequestPropertyNotify(QWidget* editorGUI)
        {
            IndividualPropertyHandlerEditNotifications::Bus::Event(
                editorGUI, &IndividualPropertyHandlerEditNotifications::Bus::Events::OnRequestPropertyNotify);
        }

        void PropertyManagerComponent::CreateBuiltInHandlers()
        {
            RegisterCrcHandler();
            RegisterIntSpinBoxHandlers();
            RegisterIntSliderHandlers();
            RegisterFilePathHandler();
            RegisterDoubleSpinBoxHandlers();
            RegisterDoubleSliderHandlers();
            RegisterColorPropertyHandlers();
            RegisterStringLineEditHandler();
            RegisterBoolComboBoxHandler();
            RegisterCheckBoxHandlers();
            RegisterBoolRadioButtonsHandler();
            RegisterEnumComboBoxHandler();
            RegisterStringComboBoxHandler();
            RegisterAssetPropertyHandler();
            RegisterAudioPropertyHandler();
            RegisterEntityIdPropertyHandler();
            RegisterVectorHandlers();
            RegisterButtonPropertyHandlers();
            RegisterMultiLineEditHandler();
            RegisterExeSelectPropertyHandler();
            RegisterLabelHandler();

            // GenericComboBoxHandlers
            RegisterGenericComboBoxHandler<AZ::Crc32>();
        }

        PropertyHandlerBase* PropertyManagerComponent::ResolvePropertyHandler(AZ::u32 handlerName, const AZ::Uuid& handlerType)
        {
            auto it = m_Handlers.find(handlerName);

            int highestPriorityFound = -1;
            PropertyHandlerBase* pHandlerFound = nullptr;
            while ((it != m_Handlers.end()) && (it->first == handlerName))
            {
                // Don't need to check against the handlerType if its null, which would only happen
                // for non-data elements (e.g. UIElement) where the handler was requested specifically by name
                if ((it->second->Priority() > highestPriorityFound) && (handlerType.IsNull() || it->second->HandlesType(handlerType)))
                {
                    highestPriorityFound = it->second->Priority();
                    pHandlerFound = it->second;
                }

                ++it;
            }

            if (!pHandlerFound)
            {
                // defaults.
                auto defaultIt = m_DefaultHandlers.find(handlerType);
                while ((defaultIt != m_DefaultHandlers.end()) && (defaultIt->first == handlerType))
                {
                    if (defaultIt->second->Priority() > highestPriorityFound)
                    {
                        highestPriorityFound = defaultIt->second->Priority();
                        pHandlerFound = defaultIt->second;
                    }
                    ++defaultIt;
                }
            }

            if (!pHandlerFound)
            {
                // does a base class have a handler?
                AZ::SerializeContext* sc = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(sc, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZStd::vector<const AZ::SerializeContext::ClassData*> classes;

                sc->EnumerateBase(
                    [&classes](const AZ::SerializeContext::ClassData* classData, AZ::Uuid)
                    {
                        if (classData)
                        {
                            classes.push_back(classData);
                        }

                        return true;
                    },
                    handlerType);
                for (auto cls = classes.begin(); cls != classes.end(); ++cls)
                {
                    pHandlerFound = ResolvePropertyHandler(handlerName, (*cls)->m_typeId);
                    if (pHandlerFound)
                    {
                        return pHandlerFound;
                    }
                }
                if (const auto* genericInfo = sc->FindGenericClassInfo(handlerType))
                {
                    if (genericInfo->GetGenericTypeId() != handlerType)
                    {
                        return ResolvePropertyHandler(handlerName, genericInfo->GetGenericTypeId());
                    }
                }
            }

            return pHandlerFound;
        }

        void PropertyManagerComponent::Reflect(AZ::ReflectContext* context)
        {
            EditorEntityIdContainer::Reflect(context);
            AzToolsFramework::CReflectedVarAudioControl::Reflect(context);
            ReflectPropertyEditor(context);

            AZ::DocumentPropertyEditor::ExpanderSettings::Reflect(context);

            // reflect data for script, serialization, editing...
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PropertyManagerComponent, AZ::Component>()
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<PropertyManagerComponent>(
                        "Property Manager", "Provides services for registration of property editors")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                        ;
                }
            }
        }
    } // namespace Components
} // namespace AzToolsFramework
