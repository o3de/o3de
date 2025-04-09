/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Color.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Utils.h>

#include <MCore/Source/LogManager.h>

#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/TwoStringEventData.h>

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/Commands.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MotionEventPresetManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>

#include <MysticQt/Source/MysticQtManager.h>

#include <QSettings>

namespace EMStudio
{
    MotionEventPreset::MotionEventPreset(const AZStd::string& name, EMotionFX::EventDataSet&& eventDatas, AZ::Color color, const AZStd::string& comment)
        : m_eventDatas(AZStd::move(eventDatas))
        , m_name(name)
        , m_color(color)
        , m_comment(comment)
        , m_isDefault(false)
    {
    }


    void MotionEventPreset::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionEventPreset>()
            ->Version(2)
            ->Field("name", &MotionEventPreset::m_name)
            ->Field("color", &MotionEventPreset::m_color)
            ->Field("eventDatas", &MotionEventPreset::m_eventDatas)
            ->Field("comment", &MotionEventPreset::m_comment)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<MotionEventPreset>("MotionEventPreset", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MotionEventPreset::m_name, "Name", "Name of this preset")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MotionEventPreset::m_color, "Color", "Color to use for events that use this preset")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(
                AZ::Edit::UIHandlers::MultiLineEdit, &MotionEventPreset::m_comment, "Comment", "Leave a comment to describe this event data preset.")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ;
    }

    //-----------------------------------

    const AZ::Color MotionEventPresetManager::s_unknownEventColor = AZ::Color::CreateFromRgba(193, 195, 196, 255);

    MotionEventPresetManager::MotionEventPresetManager()
        : m_dirtyFlag(false)
    {
        m_fileName = GetManager()->GetAppDataFolder() + "EMStudioDefaultEventPresets.cfg";
    }


    MotionEventPresetManager::~MotionEventPresetManager()
    {
        Save(false);
        Clear();
    }


    void MotionEventPresetManager::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionEventPresetManager>()
            ->Version(1)
            ->Field("eventPresets", &MotionEventPresetManager::m_eventPresets)
            ;
    }

    void MotionEventPresetManager::Clear()
    {
        for (MotionEventPreset* eventPreset : m_eventPresets)
        {
            delete eventPreset;
        }

        m_eventPresets.clear();
    }


    size_t MotionEventPresetManager::GetNumPresets() const
    {
        return m_eventPresets.size();
    }


    bool MotionEventPresetManager::IsEmpty() const
    {
        return m_eventPresets.empty(); 
    }


    void MotionEventPresetManager::AddPreset(MotionEventPreset* preset)
    {
        m_eventPresets.emplace_back(preset);
        m_dirtyFlag = true;
    }


    void MotionEventPresetManager::RemovePreset(size_t index)
    {
        delete m_eventPresets[index];
        m_eventPresets.erase(m_eventPresets.begin() + index);
        m_dirtyFlag = true;
    }


    MotionEventPreset* MotionEventPresetManager::GetPreset(size_t index) const
    {
        return m_eventPresets[index];
    }


    void MotionEventPresetManager::CreateDefaultPresets()
    {
        EMotionFX::EventDataPtr leftFootData = EMotionFX::GetEventManager().FindOrCreateEventData<EMotionFX::TwoStringEventData>("LeftFoot", "", "RightFoot");
        EMotionFX::EventDataPtr rightFootData = EMotionFX::GetEventManager().FindOrCreateEventData<EMotionFX::TwoStringEventData>("RightFoot", "", "LeftFoot");
        MotionEventPreset* leftFootPreset =
            aznew MotionEventPreset("LeftFoot", { AZStd::move(leftFootData) }, AZ::Color(AZ::u8(255), 0, 0, 255), "");
        MotionEventPreset* rightFootPreset =
            aznew MotionEventPreset("RightFoot", { AZStd::move(rightFootData) }, AZ::Color(AZ::u8(0), 255, 0, 255), "");
        leftFootPreset->SetIsDefault(true);
        rightFootPreset->SetIsDefault(true);
        m_eventPresets.emplace(m_eventPresets.begin(), leftFootPreset);
        m_eventPresets.emplace(AZStd::next(m_eventPresets.begin(), 1), rightFootPreset);
    }


    void MotionEventPresetManager::Load(const AZStd::string& filename)
    {
        m_fileName = filename;

        // Clear the old event presets.
        Clear();

        if (!LoadLYSerializedFormat())
        {
            LoadLegacyQSettingsFormat();
        }

        // LoadLYSerializedFormat() will clear m_eventPresets, so default
        // presets have to be made afterwards
        CreateDefaultPresets();

        m_dirtyFlag = false;

        // Update the default preset settings filename so that next startup the presets get auto-loaded.
        SaveToSettings();
    }


    bool MotionEventPresetManager::LoadLegacyQSettingsFormat()
    {
        QSettings settings(m_fileName.c_str(), QSettings::IniFormat, GetManager()->GetMainWindow());

        if (settings.status() != QSettings::Status::NoError)
        {
            return false;
        }

        const uint32 numMotionEventPresets = settings.value("numMotionEventPresets").toInt();
        if (numMotionEventPresets == 0)
        {
            return true;
        }

        // Add the new motion event presets.
        AZStd::string eventType;
        AZStd::string mirrorType;
        AZStd::string eventParameter;
        AZ::Color color;
        for (uint32 i = 0; i < numMotionEventPresets; ++i)
        {
            settings.beginGroup(AZStd::string::format("%i", i).c_str());
            color = RenderOptions::StringToColor(settings.value("MotionEventPresetColor").toString());

            eventType       = settings.value("MotionEventPresetType").toString().toUtf8().data();
            mirrorType      = settings.value("MotionEventPresetMirrorType").toString().toUtf8().data();
            eventParameter  = settings.value("MotionEventPresetParameter").toString().toUtf8().data();

            settings.endGroup();
                
            EMotionFX::EventDataPtr eventData = EMotionFX::GetEventManager().FindOrCreateEventData<EMotionFX::TwoStringEventData>(eventType, eventParameter, mirrorType);
            MotionEventPreset* preset = new MotionEventPreset(eventType, { AZStd::move(eventData) }, color, "");
            AddPreset(preset);
        }
        return true;
    }


    bool MotionEventPresetManager::LoadLYSerializedFormat()
    {
        return AZ::Utils::LoadObjectFromFileInPlace(m_fileName, azrtti_typeid(m_eventPresets), &m_eventPresets);
    }


    void MotionEventPresetManager::SaveAs(const AZStd::string& filename, bool showNotification)
    {
        m_fileName = filename;

        // Skip saving the built-in presets
        AZStd::vector<MotionEventPreset*> presets;
        presets.reserve(m_eventPresets.size());
        for (MotionEventPreset* preset : m_eventPresets)
        {
            if (preset->GetIsDefault())
            {
                continue;
            }
            presets.emplace_back(preset);
        }

        bool fileExisted = false;
        AZStd::string checkoutResultString;
        if (!SourceControlCommand::CheckOutFile(filename.c_str(), fileExisted, checkoutResultString, /*useSourceControl=*/true, /*add=*/false))
        {
            AZ_Warning("EMotionFX", false, checkoutResultString.c_str());
        }

        // Check if the settings correctly saved.
        if (AZ::Utils::SaveObjectToFile(filename, AZ::DataStream::ST_XML, &presets))
        {
            m_dirtyFlag = false;

            // Add file in case it did not exist before (when saving it the first time).
            if (!SourceControlCommand::CheckOutFile(filename.c_str(), fileExisted, checkoutResultString, /*useSourceControl=*/true, /*add=*/true))
            {
                AZ_Warning("EMotionFX", false, checkoutResultString.c_str());
            }

            if (showNotification)
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "Motion event presets <font color=green>successfully</font> saved");
            }
        }
        else
        {
            if (showNotification)
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "Motion event presets <font color=red>failed</font> to save");
            }
        }

        // Update the default preset settings filename so that next startup the presets get auto-loaded.
        SaveToSettings();
    }


    void MotionEventPresetManager::SaveToSettings()
    {
        if (!m_fileName.empty())
        {
            QSettings settings(GetManager()->GetMainWindow());
            settings.beginGroup("EMotionFX");
            settings.setValue("lastEventPresetFile", m_fileName.c_str());
            settings.endGroup();
        }
    }


    void MotionEventPresetManager::LoadFromSettings()
    {
        QSettings settings(GetManager()->GetMainWindow());
        settings.beginGroup("EMotionFX");
        AZStd::string filename = FromQtString(settings.value("lastEventPresetFile", QVariant("")).toString());
        settings.endGroup();

        if (!filename.empty())
        {
            m_fileName = AZStd::move(filename);
        }
    }

    // Check if motion event with this configuration exists and return color.
    AZ::Color MotionEventPresetManager::GetEventColor(const EMotionFX::EventDataSet& eventDatas) const
    {
        for (const MotionEventPreset* preset : m_eventPresets)
        {
            const EMotionFX::EventDataSet& presetDatas = preset->GetEventDatas();

            const size_t numEventDatas = eventDatas.size();
            if (numEventDatas == presetDatas.size())
            {
                for (size_t i = 0; i < numEventDatas; ++i)
                {
                    const EMotionFX::EventDataPtr& eventData = eventDatas[i];
                    const EMotionFX::EventDataPtr& presetData = presetDatas[i];

                    if (eventData && presetData &&
                        eventData->RTTI_GetType() == presetData->RTTI_GetType() &&
                        *eventData == *presetData)
                    {
                        return preset->GetEventColor();
                    }
                }
            }
        }

        // Use the same color for all events that are not from a preset.
        return s_unknownEventColor;
    }
} // namespace EMStudio
