/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Mappings/InputMappingOr.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputMappingOr::Config::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<InputMappingOr::Config, InputMapping::ConfigBase>()
                ->Version(0)
                ->Field("Source Input Channel Names", &Config::m_sourceInputChannelNames)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<InputMappingOr::Config>("Input Mapping: Or",
                    "Maps multiple different input sources to a single output using 'OR' logic.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &InputMappingOr::Config::GetNameLabelOverride)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Config::m_sourceInputChannelNames, "Source Input Channel Names",
                        "The source input channel names that will be mapped to the output input channel name.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::shared_ptr<InputMapping> InputMappingOr::Config::CreateInputMapping(const InputContext& inputContext) const
    {
        if (m_outputInputChannelName.empty())
        {
            AZ_Error("InputMappingOr::Config", false, "Cannot create input mapping with empty name.");
            return nullptr;
        }

        if (InputChannelRequests::FindInputChannel(InputChannelId(m_outputInputChannelName.c_str())))
        {
            AZ_Error("InputMappingOr::Config", false,
                     "Cannot create input mapping '%s' with non-unique name.", m_outputInputChannelName.c_str());
            return nullptr;
        }

        if (m_sourceInputChannelNames.empty())
        {
            AZ_Error("InputMappingOr::Config", false,
                     "Cannot create input mapping '%s' with no source inputs.", m_outputInputChannelName.c_str());
            return nullptr;
        }

        const InputChannelId outputInputChannelId(m_outputInputChannelName.c_str());
        AZStd::shared_ptr<InputMappingOr> inputMapping = AZStd::make_shared<InputMappingOr>(outputInputChannelId,
                                                                                            inputContext);
        for (const InputChannelNameFilteredByDeviceType& sourceInputChannelName : m_sourceInputChannelNames)
        {
            const InputChannelId sourceInputChannelId(sourceInputChannelName.GetInputChannelName().c_str());
            inputMapping->AddSourceInput(sourceInputChannelId);
        }
        return inputMapping;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputMappingOr::InputMappingOr(const InputChannelId& inputChannelId, const InputContext& inputContext)
        : InputMapping(inputChannelId, inputContext)
        , m_sourceInputChannelIds()
        , m_currentlyActiveSource(EmptySnapshot())
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMappingOr::AddSourceInput(const InputChannelId& inputSourceId)
    {
        const auto it = m_sourceInputChannelIds.find(inputSourceId);
        if (it != m_sourceInputChannelIds.end())
        {
            AZ_Warning("InputMappingOr", false,
                       "Input mapping (%s) already contains an input source with id: %s, cannot add",
                       GetInputChannelId().GetName(), inputSourceId.GetName());
            return false;
        }

        m_sourceInputChannelIds.insert(inputSourceId);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMappingOr::RemoveSourceInput(const InputChannelId& inputSourceId)
    {
        const auto it = m_sourceInputChannelIds.find(inputSourceId);
        if (it == m_sourceInputChannelIds.end())
        {
            AZ_Warning("InputMappingOr", false,
                       "Input mapping (%s) does not contain an input source with id: %s, cannot remove",
                       GetInputChannelId().GetName(), inputSourceId.GetName());
            return false;
        }

        m_sourceInputChannelIds.erase(it);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputMappingOr::GetValue() const
    {
        return m_currentlyActiveSource.m_value;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputMappingOr::GetDelta() const
    {
        return m_currentlyActiveSource.m_delta;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputMappingOr::ResetState()
    {
        m_currentlyActiveSource = EmptySnapshot();
        InputChannel::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMappingOr::IsSourceInput(const InputChannel& inputChannel)
    {
        const InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
        for (const InputChannelId& sourceInputChannelId : m_sourceInputChannelIds)
        {
            if (inputChannelId == sourceInputChannelId)
            {
                return true;
            }
        }

        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMappingOr::OnSourceInputEvent(const InputChannel& inputChannel)
    {
        // Capture the source input even when in the 'Ended' state in order to capture the delta.
        m_currentlyActiveSource = inputChannel.IsStateIdle() ?
                                  EmptySnapshot() :
                                  InputChannel::Snapshot(inputChannel);
        UpdateState(inputChannel.IsActive());
        return true;
    }
} // namespace AzFramework
