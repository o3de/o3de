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
#include <Include/IResourceSelectorHost.h>
#include <QAudioControlEditorIcons.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

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
    static SStaticResourceSelectorEntry audioTriggerSelector(
        "AudioTrigger", AudioTriggerSelector, ":/Icons/Trigger_Icon.svg");
    static SStaticResourceSelectorEntry audioSwitchSelector(
        "AudioSwitch", AudioSwitchSelector, ":/Icons/Switch_Icon.svg");
    static SStaticResourceSelectorEntry audioStateSelector(
        "AudioSwitchState", AudioSwitchStateSelector, ":/Icons/Property_Icon.png");
    static SStaticResourceSelectorEntry audioRtpcSelector(
        "AudioRTPC", AudioRTPCSelector, ":/Icons/RTPC_Icon.svg");
    static SStaticResourceSelectorEntry audioEnvironmentSelector(
        "AudioEnvironment", AudioEnvironmentSelector, ":/Icons/Environment_Icon.svg");
    static SStaticResourceSelectorEntry audioPreloadSelector(
        "AudioPreloadRequest", AudioPreloadRequestSelector, ":/Icons/Bank_Icon.png");

    //-------------------------------------------------------------------------------------------//
    void RegisterAudioControlsResourceSelectors()
    {
        if (IResourceSelectorHost* host = GetIEditor()->GetResourceSelectorHost();
            host != nullptr)
        {
            host->RegisterResourceSelector(&audioTriggerSelector);
            host->RegisterResourceSelector(&audioSwitchSelector);
            host->RegisterResourceSelector(&audioStateSelector);
            host->RegisterResourceSelector(&audioRtpcSelector);
            host->RegisterResourceSelector(&audioEnvironmentSelector);
            host->RegisterResourceSelector(&audioPreloadSelector);
        }
    }

} // namespace AudioControls
