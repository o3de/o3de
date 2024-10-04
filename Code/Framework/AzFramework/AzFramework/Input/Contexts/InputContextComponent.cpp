/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Contexts/InputContextComponent.h>
#include <AzFramework/Input/Mappings/InputMappingAnd.h>
#include <AzFramework/Input/Mappings/InputMappingOr.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputContextComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("InputContextService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputContextComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<InputContextComponent, AZ::Component>()
                ->Version(0)
                ->Field("Unique Name", &InputContextComponent::m_uniqueName)
                ->Field("Input Mappings", &InputContextComponent::m_inputMappings)
                ->Field("Local Player Index", &InputContextComponent::m_localPlayerIndex)
                ->Field("Input Listener Priority", &InputContextComponent::m_inputListenerPriority)
                ->Field("Consumes Processed Input", &InputContextComponent::m_consumesProcessedInput)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<InputContextComponent>("Input Context",
                    "An input context is a collection of input mappings, which map 'raw' input to custom input channels (ie. events).")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Category, "Input")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &InputContextComponent::m_uniqueName, "Unique Name",
                        "The name of the input context, unique among all active input contexts and input devices.\n"
                        "This will be truncated if its length exceeds that of InputDeviceId::MAX_NAME_LENGTH = 64")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &InputContextComponent::m_inputMappings, "Input Mappings",
                        "The list of all input mappings that will be created when the input context is activated.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &InputContextComponent::m_localPlayerIndex, "Local Player Index",
                        "The local player index that this context will receive input from (0 based, -1 means all controllers).\n"
                        "Will only work on platforms such as PC where the local user id corresponds to the local player index.\n"
                        "For other platforms, SetLocalUserId must be called at runtime with the id of a logged in user.")
                        ->Attribute(AZ::Edit::Attributes::Min, -1)
                        ->Attribute(AZ::Edit::Attributes::Max, 3)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &InputContextComponent::m_inputListenerPriority, "Input Listener Priority",
                        "The priority used to sort the input context relative to all other input event listeners.\n"
                        "Higher numbers indicate greater priority.")
                        ->Attribute(AZ::Edit::Attributes::Min, InputChannelEventListener::GetPriorityLast())
                        ->Attribute(AZ::Edit::Attributes::Max, InputChannelEventListener::GetPriorityFirst())
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &InputContextComponent::m_consumesProcessedInput, "Consumes Processed Input",
                        "Should the input context consume input that is processed by any of its input mappings?")
                ;
            }
        }

        InputMapping::ConfigBase::Reflect(context);
        InputMappingAnd::Config::Reflect(context);
        InputMappingOr::Config::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputContextComponent::~InputContextComponent()
    {
        Deactivate();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputContextComponent::Init()
    {
        // The local player index that this component will receive input from (0 base, -1 wildcard)
        // can be set from data, but will only work on platforms where the local user id corresponds
        // to a local player index. For other platforms SetLocalUserId must be called at runtime with
        // the id of a logged in local user, which will overwrite anything that is set here from data.
        const LocalUserId localUserId = (m_localPlayerIndex == -1) ? LocalUserIdAny : aznumeric_cast<AZ::u32>(m_localPlayerIndex);
        SetLocalUserId(localUserId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputContextComponent::Activate()
    {
        InputContextComponentRequestBus::Handler::BusConnect(GetEntityId());
        CreateInputContext();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputContextComponent::Deactivate()
    {
        ResetInputContext();
        InputContextComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputContextComponent::SetLocalUserId(LocalUserId localUserId)
    {
        // Create a new filter, or reset any existing one if we have been passed LocalUserIdAny.
        if (localUserId != LocalUserIdAny)
        {
            m_localUserIdFilter = AZStd::make_shared<InputChannelEventFilterInclusionList>(InputChannelEventFilter::AnyChannelNameCrc32,
                                                                                           InputChannelEventFilter::AnyDeviceNameCrc32,
                                                                                           aznumeric_cast<AZ::u32>(m_localPlayerIndex));
        }
        else
        {
            m_localUserIdFilter.reset();
        }

        // Set the filter if the input context has already been created.
        if (m_inputContext)
        {
            m_inputContext->SetFilter(m_localUserIdFilter);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputContextComponent::CreateInputContext()
    {
        if (m_uniqueName.empty())
        {
            AZ_Error("InputContextComponent", false, "Cannot create input context with empty name.");
            return;
        }

        if (InputDeviceRequests::FindInputDevice(InputDeviceId(m_uniqueName.c_str())))
        {
            AZ_Error("InputContextComponent", false,
                     "Cannot create input context '%s' with non-unique name.", m_uniqueName.c_str());
            return;
        }

        if (m_inputMappings.empty())
        {
            AZ_Error("InputContextComponent", false,
                     "Cannot create input context '%s' with no input mappings.", m_uniqueName.c_str());
            return;
        }

        // Create the input context.
        InputContext::InitData initData;
        initData.autoActivate = true;
        initData.filter = m_localUserIdFilter;
        initData.priority = m_inputListenerPriority;
        initData.consumesProcessedInput = m_consumesProcessedInput;
        m_inputContext = AZStd::make_unique<InputContext>(m_uniqueName.c_str(), initData);

        // Create and add all input mappings.
        for (const InputMapping::ConfigBase* inputMapping : m_inputMappings)
        {
            inputMapping->CreateInputMappingAndAddToContext(*m_inputContext);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputContextComponent::ResetInputContext()
    {
        m_inputContext.reset();
    }
} // namespace AzFramework
