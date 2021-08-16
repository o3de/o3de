/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioResourceSelectors.h>
#include <ATLControlsResourceDialog.h>
#include <AudioControlsEditorPlugin.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <UI/PropertyEditor/PropertyAudioCtrlTypes.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    AudioControlSelectorHandler::AudioControlSelectorHandler()
    {
        for (AZ::u32 type = 0; type < static_cast<AZ::u32>(AzToolsFramework::AudioPropertyType::NumTypes); ++type)
        {
            AzToolsFramework::AudioControlSelectorRequestBus::MultiHandler::BusConnect(
                static_cast<AzToolsFramework::AudioPropertyType>(type));
        }
    }

    AudioControlSelectorHandler::~AudioControlSelectorHandler()
    {
        AzToolsFramework::AudioControlSelectorRequestBus::MultiHandler::BusDisconnect();
    }

    AZStd::string AudioControlSelectorHandler::SelectResource(AZStd::string_view previousValue)
    {
        using namespace AzToolsFramework;
        if (auto busId = AudioControlSelectorRequestBus::GetCurrentBusId();
            busId != nullptr)
        {
            auto controlType = static_cast<EACEControlType>(*busId);
            QWidget* parentWidget = nullptr;
            AzToolsFramework::EditorRequestBus::BroadcastResult(parentWidget, &AzToolsFramework::EditorRequestBus::Events::GetMainWindow);

            AZStd::string levelName;
            AzToolsFramework::EditorRequestBus::BroadcastResult(levelName, &AzToolsFramework::EditorRequests::GetLevelName);

            ATLControlsDialog dialog(parentWidget, controlType);
            dialog.SetScope(levelName);
            return dialog.ChooseItem(previousValue.data());
        }
        return previousValue;
    }

} // namespace AudioControls
