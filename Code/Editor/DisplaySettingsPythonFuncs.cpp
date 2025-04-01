/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "DisplaySettingsPythonFuncs.h"

// AzCore
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

// Editor
#include "DisplaySettings.h"

namespace
{
    int PyGetMiscDisplaySettings()
    {
        return GetIEditor()->GetDisplaySettings()->GetSettings();
    }

    void PySetMiscDisplaySettings(int flags)
    {
        GetIEditor()->GetDisplaySettings()->SetSettings(flags);
    }
}

namespace AzToolsFramework
{
    bool DisplaySettingsState::operator==(const DisplaySettingsState& rhs) const
    {
        return this->m_noCollision == rhs.m_noCollision &&
            this->m_noLabels == rhs.m_noLabels &&
            this->m_simulate == rhs.m_simulate &&
            this->m_hideTracks == rhs.m_hideTracks &&
            this->m_hideLinks == rhs.m_hideLinks &&
            this->m_hideHelpers == rhs.m_hideHelpers &&
            this->m_showDimensionFigures == rhs.m_showDimensionFigures;
    }

    AZStd::string DisplaySettingsState::ToString() const
    {
        auto boolToPyString = [](bool v)
        {
            return v ? "True" : "False";
        };

        return AZStd::string::format("(no_collision=%s, no_labels=%s, simulate=%s, hide_tracks=%s, hide_links=%s, hide_helpers=%s, show_dimension_figures=%s)",
            boolToPyString(this->m_noCollision),
            boolToPyString(this->m_noLabels),
            boolToPyString(this->m_simulate),
            boolToPyString(this->m_hideTracks),
            boolToPyString(this->m_hideLinks),
            boolToPyString(this->m_hideHelpers),
            boolToPyString(this->m_showDimensionFigures)
            );
    }

    void DisplaySettingsPythonFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.settings' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Settings")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.settings");
            };
            addLegacyGeneral(behaviorContext->Method("get_misc_editor_settings", PyGetMiscDisplaySettings, nullptr, "Get miscellaneous Editor settings (camera collides with terrain, AI/Physics enabled, etc)."));
            addLegacyGeneral(behaviorContext->Method("set_misc_editor_settings", PySetMiscDisplaySettings, nullptr, "Set miscellaneous Editor settings (camera collides with terrain, AI/Physics enabled, etc)."));

            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_NOCOLLISION>("DisplaySettings_NoCollision")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_NOLABELS>("DisplaySettings_NoLabels")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_PHYSICS>("DisplaySettings_Physics")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_HIDE_TRACKS>("DisplaySettings_HideTracks")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_HIDE_LINKS>("DisplaySettings_HideLinks")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_HIDE_HELPERS>("DisplaySettings_HideHelpers")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_SHOW_DIMENSIONFIGURES>("DisplaySettings_ShowDimensionFigures")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            behaviorContext->EnumProperty<EDisplaySettingsFlags::SETTINGS_SERIALIZABLE_FLAGS_MASK>("DisplaySettings_SerializableFlagsMask")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
        }
    }

    void DisplaySettingsComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DisplaySettingsComponent, AZ::Component>();

            serializeContext->Class<DisplaySettingsState>()
                ->Version(1)
                ->Field("NoCollision", &DisplaySettingsState::m_noCollision)
                ->Field("NoLabels", &DisplaySettingsState::m_noLabels)
                ->Field("Simulate", &DisplaySettingsState::m_simulate)
                ->Field("HideTracks", &DisplaySettingsState::m_hideTracks)
                ->Field("HideLinks", &DisplaySettingsState::m_hideLinks)
                ->Field("HideHelpers", &DisplaySettingsState::m_hideHelpers)
                ->Field("ShowDimensionFigures", &DisplaySettingsState::m_showDimensionFigures)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DisplaySettingsState>("DisplaySettingsState")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "DisplaySettings")
                ->Attribute(AZ::Script::Attributes::Module, "display_settings")
                ->Property("NoCollision", BehaviorValueProperty(&DisplaySettingsState::m_noCollision))
                    ->Attribute(AZ::Script::Attributes::Alias, "no_collision")
                ->Property("NoLabels", BehaviorValueProperty(&DisplaySettingsState::m_noLabels))
                    ->Attribute(AZ::Script::Attributes::Alias, "no_labels")
                ->Property("Simulate", BehaviorValueProperty(&DisplaySettingsState::m_simulate))
                    ->Attribute(AZ::Script::Attributes::Alias, "simulate")
                ->Property("HideTracks", BehaviorValueProperty(&DisplaySettingsState::m_hideTracks))
                    ->Attribute(AZ::Script::Attributes::Alias, "hide_tracks")
                ->Property("HideLinks", BehaviorValueProperty(&DisplaySettingsState::m_hideLinks))
                    ->Attribute(AZ::Script::Attributes::Alias, "hide_links")
                ->Property("HideHelpers", BehaviorValueProperty(&DisplaySettingsState::m_hideHelpers))
                    ->Attribute(AZ::Script::Attributes::Alias, "hide_helpers")
                ->Property("ShowDimensionFigures", BehaviorValueProperty(&DisplaySettingsState::m_showDimensionFigures))
                    ->Attribute(AZ::Script::Attributes::Alias, "show_dimension_figures")
                ->Method("ToString", &DisplaySettingsState::ToString)
                ;

            behaviorContext->EBus<DisplaySettingsBus>("DisplaySettingsBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "DisplaySettings")
                ->Attribute(AZ::Script::Attributes::Module, "display_settings")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("GetSettingsState", &DisplaySettingsRequests::GetSettingsState)
                ->Event("SetSettingsState", &DisplaySettingsRequests::SetSettingsState)
            ;
        }
    }

    void DisplaySettingsComponent::Activate()
    {
        DisplaySettingsBus::Handler::BusConnect();
    }

    void DisplaySettingsComponent::Deactivate()
    {
        DisplaySettingsBus::Handler::BusDisconnect();
    }

    DisplaySettingsState DisplaySettingsComponent::GetSettingsState() const
    {
        return ConvertToSettings(GetIEditor()->GetDisplaySettings()->GetSettings());
    }

    void DisplaySettingsComponent::SetSettingsState(const DisplaySettingsState& settingsState)
    {
        uint flags = ConvertToFlags(settingsState);
        GetIEditor()->GetDisplaySettings()->SetSettings(flags);
    }

    int DisplaySettingsComponent::ConvertToFlags(const DisplaySettingsState& state) const
    {
        int flags = 0;
        flags |= state.m_noCollision ? SETTINGS_NOCOLLISION : 0x0;
        flags |= state.m_noLabels ? SETTINGS_NOLABELS : 0x0;
        flags |= state.m_simulate ? SETTINGS_PHYSICS : 0x0;
        flags |= state.m_hideTracks ? SETTINGS_HIDE_TRACKS : 0x0;
        flags |= state.m_hideLinks ? SETTINGS_HIDE_LINKS : 0x0;
        flags |= state.m_hideHelpers ? SETTINGS_HIDE_HELPERS : 0x0;
        flags |= state.m_showDimensionFigures ? SETTINGS_SHOW_DIMENSIONFIGURES : 0x0;

        return flags;
    }

    DisplaySettingsState DisplaySettingsComponent::ConvertToSettings(int settings) const
    {
        DisplaySettingsState state;
        state.m_noCollision = settings & SETTINGS_NOCOLLISION;
        state.m_noLabels = settings & SETTINGS_NOLABELS;
        state.m_simulate = settings & SETTINGS_PHYSICS;
        state.m_hideTracks = settings & SETTINGS_HIDE_TRACKS;
        state.m_hideLinks = settings & SETTINGS_HIDE_LINKS;
        state.m_hideHelpers = settings & SETTINGS_HIDE_HELPERS;
        state.m_showDimensionFigures = settings & SETTINGS_SHOW_DIMENSIONFIGURES;

        return state;
    }
}
