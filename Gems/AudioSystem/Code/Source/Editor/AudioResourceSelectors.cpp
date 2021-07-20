/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ATLControlsResourceDialog.h>
#include <AudioControlsEditorPlugin.h>
#include <IEditor.h>
#include <Include/IResourceSelectorHost.h>
#include <QAudioControlEditorIcons.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

using namespace AudioControls;

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    QString ShowSelectDialog(const SResourceSelectorContext& context, const QString& pPreviousValue, const EACEControlType controlType)
    {
        AZ_Assert(CAudioControlsEditorPlugin::GetATLModel() != nullptr, "AudioResourceSelectors - ATL Model is null!");

        AZStd::string levelName;
        AzToolsFramework::EditorRequestBus::BroadcastResult(levelName, &AzToolsFramework::EditorRequests::GetLevelName);

        ATLControlsDialog dialog(context.parentWidget, controlType);
        dialog.SetScope(levelName);
        return dialog.ChooseItem(pPreviousValue.toUtf8().constData());
    }

    //-------------------------------------------------------------------------------------------//
    QString AudioTriggerSelector(const SResourceSelectorContext& context, const QString& pPreviousValue)
    {
        return ShowSelectDialog(context, pPreviousValue, eACET_TRIGGER);
    }

    //-------------------------------------------------------------------------------------------//
    QString AudioSwitchSelector(const SResourceSelectorContext& context, const QString& pPreviousValue)
    {
        return ShowSelectDialog(context, pPreviousValue, eACET_SWITCH);
    }

    //-------------------------------------------------------------------------------------------//
    QString AudioSwitchStateSelector(const SResourceSelectorContext& context, const QString& pPreviousValue)
    {
        return ShowSelectDialog(context, pPreviousValue, eACET_SWITCH_STATE);
    }

    //-------------------------------------------------------------------------------------------//
    QString AudioRTPCSelector(const SResourceSelectorContext& context, const QString& pPreviousValue)
    {
        return ShowSelectDialog(context, pPreviousValue, eACET_RTPC);
    }

    //-------------------------------------------------------------------------------------------//
    QString AudioEnvironmentSelector(const SResourceSelectorContext& context, const QString& pPreviousValue)
    {
        return ShowSelectDialog(context, pPreviousValue, eACET_ENVIRONMENT);
    }

    //-------------------------------------------------------------------------------------------//
    QString AudioPreloadRequestSelector(const SResourceSelectorContext& context, const QString& pPreviousValue)
    {
        return ShowSelectDialog(context, pPreviousValue, eACET_PRELOAD);
    }

    //-------------------------------------------------------------------------------------------//
    REGISTER_RESOURCE_SELECTOR("AudioTrigger", AudioTriggerSelector, ":/AudioControlsEditor/Icons/Trigger_Icon.png");
    REGISTER_RESOURCE_SELECTOR("AudioSwitch", AudioSwitchSelector, ":/AudioControlsEditor/Icons/Switch_Icon.png");
    REGISTER_RESOURCE_SELECTOR("AudioSwitchState", AudioSwitchStateSelector, ":/AudioControlsEditor/Icons/State_Icon.png");
    REGISTER_RESOURCE_SELECTOR("AudioRTPC", AudioRTPCSelector, ":/AudioControlsEditor/Icons/RTPC_Icon.png");
    REGISTER_RESOURCE_SELECTOR("AudioEnvironment", AudioEnvironmentSelector, ":/AudioControlsEditor/Icons/Environment_Icon.png");
    REGISTER_RESOURCE_SELECTOR("AudioPreloadRequest", AudioPreloadRequestSelector, ":/AudioControlsEditor/Icons/Bank_Icon.png");
} // namespace AudioControls
