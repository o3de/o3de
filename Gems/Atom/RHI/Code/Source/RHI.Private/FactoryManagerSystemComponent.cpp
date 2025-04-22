/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI/ValidationLayer.h>

#include <RHI.Private/FactoryManagerSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/sort.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ::RHI
{
    void FactoryManagerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<FactoryManagerSystemComponent, AZ::Component>()
                ->Version(1)
                ->Field("factoriesPriority", &FactoryManagerSystemComponent::m_factoriesPriority)
                ->Field("validationMode", &FactoryManagerSystemComponent::m_validationMode);

            if (AZ::EditContext* ec = serializeContext->GetEditContext())
            {
                ec->Class<FactoryManagerSystemComponent>("Atom RHI Manager", "Atom Renderer")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &FactoryManagerSystemComponent::m_factoriesPriority,
                        "RHI Priority list",
                        "Priorities for RHI Implementations")
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &FactoryManagerSystemComponent::m_validationMode,
                        "Validation Layer Mode",
                        "Set the validation mode for the RHI. It only applies for non release builds")
                    ->Attribute(
                        AZ::Edit::Attributes::EnumValues,
                        AZStd::vector<AZ::Edit::EnumConstant<RHI::ValidationMode>>{
                            AZ::Edit::EnumConstant<RHI::ValidationMode>(
                                RHI::ValidationMode::Disabled, "Disable - Disables any validation."),
                            AZ::Edit::EnumConstant<RHI::ValidationMode>(
                                RHI::ValidationMode::Enabled, "Enable - Enables warnings and errors validation messages."),
                            AZ::Edit::EnumConstant<RHI::ValidationMode>(
                                RHI::ValidationMode::Verbose, "Verbose - Enables warnings, error and information messages."),
                            AZ::Edit::EnumConstant<RHI::ValidationMode>(RHI::ValidationMode::GPU, "GPU - Enables based validation."),
                        });
            }
                
        }
    }

    void FactoryManagerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(Factory::GetManagerComponentService());
    }

    void FactoryManagerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(Factory::GetManagerComponentService());
    }

    void FactoryManagerSystemComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("GraphicsProfilerService"));
        dependent.push_back(AZ_CRC_CE("DeviceAttributesSystemComponentService"));
    }

    void FactoryManagerSystemComponent::Activate()
    {
        UpdateValidationModeFromCommandline();
        FactoryManagerBus::Handler::BusConnect();
    }

    void FactoryManagerSystemComponent::Deactivate()
    {
        FactoryManagerBus::Handler::BusDisconnect();
    }

    void FactoryManagerSystemComponent::RegisterFactory(Factory* factory)
    {
        m_registeredFactories.push_back(factory);
    }

    void FactoryManagerSystemComponent::UnregisterFactory(Factory* factory)
    {
        auto findIt = AZStd::find(m_registeredFactories.begin(), m_registeredFactories.end(), factory);
        if (findIt != m_registeredFactories.end())
        {
            m_registeredFactories.erase(findIt);
            if (Factory::IsReady() && factory == &Factory::Get())
            {
                Factory::Unregister(factory);

                FactoryManagerNotificationBus::Broadcast(&FactoryManagerNotification::FactoryUnregistered);
            }
        }
        else
        {
            AZ_Error("FactoryManagerSystemComponent", false, "Trying to unregister invalid factory");
        }
    }

    void FactoryManagerSystemComponent::FactoryRegistrationFinalize()
    {
        // This will be called when all factories have finished registering.
        AZ_Assert(!m_registeredFactories.empty(), "No factories registered");

        // We first check if a factory was specified via command line.
        // If that fails we choose one from the registered factories.
        Factory* factory = GetFactoryFromCommandLine();
        if (!factory)
        {
            factory = SelectRegisteredFactory();
        }

        AZ_Assert(factory, "Could not select factory");

        Factory::Register(factory);

        FactoryManagerNotificationBus::Broadcast(&FactoryManagerNotification::FactoryRegistered);
    }

    Factory* FactoryManagerSystemComponent::GetFactoryFromCommandLine()
    {
        AZStd::string cmdLineFactory = RHI::GetCommandLineValue("rhi");
        if (!cmdLineFactory.empty())
        {
            RHI::APIType cmdLineFactoryType(cmdLineFactory.c_str());
            auto cmdLineFindIt = AZStd::find_if(m_registeredFactories.begin(), m_registeredFactories.end(), [&cmdLineFactoryType](const auto& element)
            {
                return element->GetType() == cmdLineFactoryType;
            });

            if (cmdLineFindIt != m_registeredFactories.end())
            {
                return *cmdLineFindIt;
            }
            else
            {
                AZ_Warning("FactoryManagerSystemComponent", false, "RHI %s provided by command line is not available. Ignoring argument.", cmdLineFactory.c_str());
            }
        }

        return nullptr;
    }

    Factory* FactoryManagerSystemComponent::SelectRegisteredFactory()
    {
        // If there's more that one factory registered then we need to decide which one to use.
        if (m_registeredFactories.size() > 1)
        {
            // Load factories priority list from the settings registry
            if (auto* registry = AZ::SettingsRegistry::Get(); registry != nullptr)
            {
                m_factoriesPriority.clear();
                if (registry->GetObject(m_factoriesPriority, "/O3DE/Atom/RHI/FactoryManager/factoriesPriority"))
                {
                    AZ_Printf(
                        "FactoryManagerSystemComponent",
                        "User has provided a list of factories priority. This will override the default priorities");
                }
            }

            // Build a hash table to quickly check the user defined priority for a factory.
            AZStd::unordered_map<RHI::APIType, size_t> factoryToPriorityMap;
            for (size_t i = 0; i < m_factoriesPriority.size(); ++i)
            {
                factoryToPriorityMap[RHI::APIType(m_factoriesPriority[i].c_str())] = i;
            }

            AZStd::sort(m_registeredFactories.begin(), m_registeredFactories.end(), [&factoryToPriorityMap](Factory* lhs, Factory* rhs)
            {
                auto lhsFindPriority = factoryToPriorityMap.find(lhs->GetType());
                auto rhsFindPriority = factoryToPriorityMap.find(rhs->GetType());
                if (lhsFindPriority != factoryToPriorityMap.end() && rhsFindPriority != factoryToPriorityMap.end())
                {
                    // Both factories have user defined priority, so we just compare them directly.
                    return lhsFindPriority->second < rhsFindPriority->second;
                }
                else if (lhsFindPriority == factoryToPriorityMap.end() && rhsFindPriority == factoryToPriorityMap.end())
                {
                    // None of the factories have user defined priorities, so we use the default priorities.
                    return lhs->GetDefaultPriority() < rhs->GetDefaultPriority();
                }
                else
                {
                    // If one of the factories has user defined priority, then we prefer that one.
                    return lhsFindPriority == factoryToPriorityMap.end() ? false : true;
                }
            });
        }

        return m_registeredFactories.front();
    }

    AZ::RHI::ValidationMode FactoryManagerSystemComponent::DetermineValidationMode() const
    {
        return m_validationMode;
    }

    void FactoryManagerSystemComponent::UpdateValidationModeFromCommandline()
    {
        m_validationMode = AZ::RHI::ReadValidationMode();
    }

    void FactoryManagerSystemComponent::EnumerateFactories(AZStd::function<bool(Factory* factory)> callback)
    {
        for (auto& factory : m_registeredFactories)
        {
            if (!callback(factory))
            {
                break;
            }
        }
    }
}
