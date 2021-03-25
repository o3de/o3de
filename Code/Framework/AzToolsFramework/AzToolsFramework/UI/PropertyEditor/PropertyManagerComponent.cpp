/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "AzToolsFramework_precompiled.h"
#include "PropertyManagerComponent.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrlTypes.h>
#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>
#include <AzToolsFramework/ToolsComponents/TransformScalePropertyHandler.h>

namespace AzToolsFramework
{
    void RegisterIntSpinBoxHandlers();
    void RegisterIntSliderHandlers();
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
    void RegisterTransformScaleHandler();
    void ReflectPropertyEditor(AZ::ReflectContext* context);

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

            CreateBuiltInHandlers();
        }

        void PropertyManagerComponent::Deactivate()
        {
            for (auto it = m_builtInHandlers.begin(); it != m_builtInHandlers.end(); ++it)
            {
                UnregisterPropertyType(*it);
                delete *it;
            }

            m_builtInHandlers.clear();


    #ifdef _DEBUG
            auto it = m_Handlers.begin();
            while (it != m_Handlers.end())
            {
                AZ_Error("PropertyManager", false, "Property Handler 0x%08x is still registered during shutdown", it->first);
                ++it;
            }
    #endif
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
        }


        void PropertyManagerComponent::CreateBuiltInHandlers()
        {
            RegisterCrcHandler();
            RegisterIntSpinBoxHandlers();
            RegisterIntSliderHandlers();
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
            RegisterTransformScaleHandler();

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
                if ((it->second->Priority() > highestPriorityFound) && (it->second->HandlesType(handlerType)))
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
                AZ::SerializeContext* sc = NULL;
                EBUS_EVENT_RESULT(sc, AZ::ComponentApplicationBus, GetSerializeContext);
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
                if (classes.empty())
                {
                    return pHandlerFound;
                }
                else
                {
                    for (auto cls = classes.begin(); cls != classes.end(); ++cls)
                    {
                        pHandlerFound = ResolvePropertyHandler(handlerName, (*cls)->m_typeId);
                        if (pHandlerFound)
                        {
                            return pHandlerFound;
                        }
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
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ;
                }
            }
        }
    } // namespace Components
} // namespace AzToolsFramework
