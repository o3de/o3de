/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Mappings/InputMapping.h>
#include <AzFramework/Input/Contexts/InputContext.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/sort.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputMapping::InputChannelNameFilteredByDeviceType::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<InputMapping::InputChannelNameFilteredByDeviceType>()
                ->Version(0)
                ->Field("Input Device Type", &InputChannelNameFilteredByDeviceType::m_inputDeviceType)
                ->Field("Input Channel Name", &InputChannelNameFilteredByDeviceType::m_inputChannelName)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<InputChannelNameFilteredByDeviceType>("InputChannelNameFilteredByDeviceType",
                    "An input channel name (filtered by an input device type) to add to the input mapping.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &InputChannelNameFilteredByDeviceType::GetNameLabelOverride)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &InputChannelNameFilteredByDeviceType::m_inputDeviceType, "Input Device Type",
                        "The type of input device by which to filter input channel names.")
                        ->Attribute(AZ::Edit::Attributes::StringList, &InputChannelNameFilteredByDeviceType::GetValidInputDeviceTypes)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &InputChannelNameFilteredByDeviceType::OnInputDeviceTypeSelected)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &InputChannelNameFilteredByDeviceType::m_inputChannelName, "Input Channel Name",
                        "The input channel name to add to the input mapping.")
                        ->Attribute(AZ::Edit::Attributes::StringList, &InputChannelNameFilteredByDeviceType::GetValidInputChannelNamesBySelectedDevice)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                ;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputMapping::InputChannelNameFilteredByDeviceType::InputChannelNameFilteredByDeviceType()
    {
        // Try initialize the selected input device type and input channel name to something valid.
        if (m_inputDeviceType.empty())
        {
            const AZStd::vector<AZStd::string> validInputDeviceTypes = GetValidInputDeviceTypes();
            if (!validInputDeviceTypes.empty())
            {
                m_inputDeviceType = validInputDeviceTypes[0];
                OnInputDeviceTypeSelected();
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Crc32 InputMapping::InputChannelNameFilteredByDeviceType::OnInputDeviceTypeSelected()
    {
        const AZStd::vector<AZStd::string> validInputNames = GetValidInputChannelNamesBySelectedDevice();
        if (!validInputNames.empty())
        {
            m_inputChannelName = validInputNames[0];
        }
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string InputMapping::InputChannelNameFilteredByDeviceType::GetNameLabelOverride() const
    {
        return m_inputChannelName.empty() ? "<Select>" : m_inputChannelName;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::vector<AZStd::string> InputMapping::InputChannelNameFilteredByDeviceType::GetValidInputDeviceTypes() const
    {
        AZStd::set<AZStd::string> uniqueInputDeviceTypes;
        InputDeviceRequests::InputDeviceByIdMap availableInputDevicesById;
        InputDeviceRequestBus::Broadcast(&InputDeviceRequests::GetInputDevicesById,
                                         availableInputDevicesById);
        for (const auto& inputDeviceById : availableInputDevicesById)
        {
            // Filter out input contexts so that mappings can only be created from 'raw' input events.
            if (!azrtti_istypeof<InputContext*>(inputDeviceById.second))
            {
                uniqueInputDeviceTypes.insert(inputDeviceById.first.GetName());
            }
        }
        return AZStd::vector<AZStd::string>(uniqueInputDeviceTypes.begin(), uniqueInputDeviceTypes.end());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::vector<AZStd::string> InputMapping::InputChannelNameFilteredByDeviceType::GetValidInputChannelNamesBySelectedDevice() const
    {
        AZStd::vector<AZStd::string> validInputChannelNames;
        InputDeviceId selectedDeviceId(m_inputDeviceType.c_str());
        InputDeviceRequests::InputChannelIdSet validInputChannelIds;
        InputDeviceRequestBus::Event(selectedDeviceId,
                                     &InputDeviceRequests::GetInputChannelIds,
                                     validInputChannelIds);
        for (const InputChannelId& inputChannelId : validInputChannelIds)
        {
            validInputChannelNames.push_back(inputChannelId.GetName());
        }
        AZStd::sort(validInputChannelNames.begin(), validInputChannelNames.end());
        return validInputChannelNames;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputMapping::ConfigBase::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<InputMapping::ConfigBase>()
                ->Version(0)
                ->Field("Output Input Channel Name", &InputMapping::ConfigBase::m_outputInputChannelName)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<InputMapping::ConfigBase>("Input Mapping: Base",
                    "Maps multiple different input sources to a single output input channel.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ConfigBase::m_outputInputChannelName, "Output Input Channel Name",
                        "The unique input channel name (ie. input event name) output by the input mapping.\n"
                        "This will be truncated if its length exceeds that of InputChannelId::MAX_NAME_LENGTH = 64")
                        ->Attribute(AZ::Edit::Attributes::Max, InputChannelId::MAX_NAME_LENGTH)
                ;
            }
        }

        InputChannelNameFilteredByDeviceType::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::shared_ptr<InputMapping> InputMapping::ConfigBase::CreateInputMappingAndAddToContext(InputContext& inputContext) const
    {
        AZStd::shared_ptr<InputMapping> inputMapping = CreateInputMapping(inputContext);
        if (inputMapping)
        {
            inputContext.AddInputMapping(inputMapping);
        }
        return inputMapping;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string InputMapping::ConfigBase::GetNameLabelOverride() const
    {
        return m_outputInputChannelName.empty() ? "<Input Mapping>" : m_outputInputChannelName;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputMapping::InputMapping(const InputChannelId& inputChannelId, const InputContext& inputContext)
        : InputChannel(inputChannelId, inputContext)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMapping::ProcessPotentialSourceInputEvent(const InputChannel& inputChannel)
    {
        return IsSourceInput(inputChannel) ? OnSourceInputEvent(inputChannel) : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputMapping::OnTick()
    {
        // The 'Ended' state for any input channel only lasts for one tick, but we will not receive
        // any events when a source input transitions from Ended->Idle, and we need to do that here.
        if (IsStateEnded())
        {
            ResetState();
        }
    }
} // namespace AzFramework
