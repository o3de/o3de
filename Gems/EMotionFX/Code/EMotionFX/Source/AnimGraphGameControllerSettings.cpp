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

#include <EMotionFX/Source/AnimGraphGameControllerSettings.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphGameControllerSettings, AnimGraphGameControllerSettingsAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphGameControllerSettings::ParameterInfo, AnimGraphGameControllerSettingsAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphGameControllerSettings::ButtonInfo, AnimGraphGameControllerSettingsAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphGameControllerSettings::Preset, AnimGraphGameControllerSettingsAllocator, 0)

    AnimGraphGameControllerSettings::ParameterInfo::ParameterInfo()
        : m_axis(MCORE_INVALIDINDEX8)
        , m_mode(PARAMMODE_STANDARD)
        , m_invert(true)
        , m_enabled(true)
    {
    }


    AnimGraphGameControllerSettings::ParameterInfo::ParameterInfo(const char* parameterName)
        : ParameterInfo()
    {
        m_parameterName = parameterName;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AnimGraphGameControllerSettings::ButtonInfo::ButtonInfo()
        : m_buttonIndex(MCORE_INVALIDINDEX32)
        , m_mode(AnimGraphGameControllerSettings::BUTTONMODE_NONE)
        , m_oldIsPressed(false)
        , m_enabled(true)
    {
    }


    AnimGraphGameControllerSettings::ButtonInfo::ButtonInfo(AZ::u32 buttonIndex)
        : ButtonInfo()
    {
        m_buttonIndex = buttonIndex;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AnimGraphGameControllerSettings::Preset::Preset()
    {
    }


    AnimGraphGameControllerSettings::Preset::Preset(const char* name)
        : Preset()
    {
        m_name = name;
    }


    AnimGraphGameControllerSettings::Preset::~Preset()
    {
        Clear();
    }


    void AnimGraphGameControllerSettings::Preset::Clear()
    {
        for (ParameterInfo* parameterInfo : m_parameterInfos)
        {
            delete parameterInfo;
        }
        m_parameterInfos.clear();

        for (ButtonInfo* buttonInfo : m_buttonInfos)
        {
            delete buttonInfo;
        }
        m_buttonInfos.clear();
    }


    // find parameter info by name and create one if it doesn't exist yet
    AnimGraphGameControllerSettings::ParameterInfo* AnimGraphGameControllerSettings::Preset::FindParameterInfo(const char* parameterName)
    {
        for (ParameterInfo* parameterInfo : m_parameterInfos)
        {
            // check if we have already created a parameter info for the given parameter
            if (parameterInfo->m_parameterName == parameterName)
            {
                return parameterInfo;
            }
        }

        // create a new parameter info
        ParameterInfo* parameterInfo = aznew ParameterInfo(parameterName);
        m_parameterInfos.emplace_back(parameterInfo);
        return parameterInfo;
    }


    // find the button info by index and create one if it doesn't exist
    AnimGraphGameControllerSettings::ButtonInfo* AnimGraphGameControllerSettings::Preset::FindButtonInfo(AZ::u32 buttonIndex)
    {
        for (ButtonInfo* buttonInfo : m_buttonInfos)
        {
            // check if we have already created a button info for the given button
            if (buttonInfo->m_buttonIndex == buttonIndex)
            {
                return buttonInfo;
            }
        }

        // create a new button info
        ButtonInfo* buttonInfo = aznew ButtonInfo(buttonIndex);
        m_buttonInfos.emplace_back(buttonInfo);
        return buttonInfo;
    }


    // check if the parameter with the given name is being controlled by the gamepad
    bool AnimGraphGameControllerSettings::Preset::CheckIfIsParameterButtonControlled(const char* stringName)
    {
        for (ButtonInfo* buttonInfo : m_buttonInfos)
        {
            // check if the button info is linked to the given string
            if (buttonInfo->m_string == stringName)
            {
                // return success in case this button info isn't set to the mode none
                if (buttonInfo->m_mode != BUTTONMODE_NONE)
                {
                    return true;
                }
            }
        }

        // failure, not found
        return false;
    }


    // check if any of the button infos that are linked to the given string name is enabled
    bool AnimGraphGameControllerSettings::Preset::CheckIfIsButtonEnabled(const char* stringName)
    {
        for (ButtonInfo* buttonInfo : m_buttonInfos)
        {
            // check if the button info is linked to the given string
            if (buttonInfo->m_string == stringName)
            {
                // return success in case this button info is enabled
                if (buttonInfo->m_enabled)
                {
                    return true;
                }
            }
        }

        // failure, not found
        return false;
    }


    // set all button infos that are linked to the given string name to the enabled flag
    void AnimGraphGameControllerSettings::Preset::SetButtonEnabled(const char* stringName, bool isEnabled)
    {
        for (ButtonInfo* buttonInfo : m_buttonInfos)
        {
            // check if the button info is linked to the given string and set the enabled flag in case it is linked to the given string name
            if (buttonInfo->m_string == stringName)
            {
                buttonInfo->m_enabled = isEnabled;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AnimGraphGameControllerSettings::AnimGraphGameControllerSettings()
        : m_activePresetIndex(MCORE_INVALIDINDEX32)
    {
    }


    AnimGraphGameControllerSettings::~AnimGraphGameControllerSettings()
    {
        Clear();
    }


    void AnimGraphGameControllerSettings::AddPreset(Preset* preset)
    {
        m_presets.emplace_back(preset);
    }


    void AnimGraphGameControllerSettings::RemovePreset(size_t index)
    {
        delete m_presets[index];
        m_presets.erase(m_presets.begin() + index);
    }


    void AnimGraphGameControllerSettings::SetNumPresets(size_t numPresets)
    {
        m_presets.resize(numPresets);
    }


    void AnimGraphGameControllerSettings::SetPreset(size_t index, Preset* preset)
    {
        m_presets[index] = preset;
    }


    void AnimGraphGameControllerSettings::Clear()
    {
        for (Preset* preset : m_presets)
        {
            delete preset;
        }

        m_presets.clear();
    }


    size_t AnimGraphGameControllerSettings::FindPresetIndexByName(const char* presetName) const
    {
        const size_t presetCount = m_presets.size();
        for (size_t i = 0; i < presetCount; ++i)
        {
            if (m_presets[i]->GetNameString() == presetName)
            {
                return i;
            }
        }

        // return failure
        return MCORE_INVALIDINDEX32;
    }


    size_t AnimGraphGameControllerSettings::FindPresetIndex(Preset* preset) const
    {
        const size_t presetCount = m_presets.size();
        for (size_t i = 0; i < presetCount; ++i)
        {
            if (m_presets[i] == preset)
            {
                return i;
            }
        }

        // return failure
        return MCORE_INVALIDINDEX32;
    }


    void AnimGraphGameControllerSettings::SetActivePreset(Preset* preset)
    {
        m_activePresetIndex = static_cast<AZ::u32>(FindPresetIndex(preset));
    }


    uint32 AnimGraphGameControllerSettings::GetActivePresetIndex() const
    {
        if (m_activePresetIndex < m_presets.size())
        {
            return m_activePresetIndex;
        }

        return MCORE_INVALIDINDEX32;
    }


    AnimGraphGameControllerSettings::Preset* AnimGraphGameControllerSettings::GetActivePreset() const
    {
        if (m_activePresetIndex < m_presets.size())
        {
            return m_presets[m_activePresetIndex];
        }

        return nullptr;
    }


    void AnimGraphGameControllerSettings::OnParameterNameChange(const char* oldName, const char* newName)
    {
        // check if we have any preset to save
        const size_t numPresets = m_presets.size();
        if (numPresets == 0)
        {
            return;
        }

        for (Preset* preset : m_presets)
        {
            const size_t parameterInfoCount = preset->GetNumParamInfos();
            for (size_t j = 0; j < parameterInfoCount; ++j)
            {
                ParameterInfo* parameterInfo = preset->GetParamInfo(j);
                if (parameterInfo == nullptr)
                {
                    continue;
                }

                // compare the names and replace them in case they are equal
                if (parameterInfo->m_parameterName == oldName)
                {
                    parameterInfo->m_parameterName = newName;
                }
            }
        }
    }


    AnimGraphGameControllerSettings::Preset* AnimGraphGameControllerSettings::GetPreset(size_t index) const
    {
        return m_presets[index];
    }


    size_t AnimGraphGameControllerSettings::GetNumPresets() const
    {
        return m_presets.size();
    }


    void AnimGraphGameControllerSettings::ParameterInfo::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphGameControllerSettings::ParameterInfo>()
            ->Version(1)
            ->Field("parameterName", &AnimGraphGameControllerSettings::ParameterInfo::m_parameterName)
            ->Field("mode", &AnimGraphGameControllerSettings::ParameterInfo::m_mode)
            ->Field("invert", &AnimGraphGameControllerSettings::ParameterInfo::m_invert)
            ->Field("enabled", &AnimGraphGameControllerSettings::ParameterInfo::m_enabled)
            ->Field("axis", &AnimGraphGameControllerSettings::ParameterInfo::m_axis)
            ;
    }


    void AnimGraphGameControllerSettings::ButtonInfo::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphGameControllerSettings::ButtonInfo>()
            ->Version(1)
            ->Field("buttonIndex", &AnimGraphGameControllerSettings::ButtonInfo::m_buttonIndex)
            ->Field("mode", &AnimGraphGameControllerSettings::ButtonInfo::m_mode)
            ->Field("string", &AnimGraphGameControllerSettings::ButtonInfo::m_string)
            ->Field("enabled", &AnimGraphGameControllerSettings::ButtonInfo::m_enabled)
            ;
    }


    void AnimGraphGameControllerSettings::Preset::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphGameControllerSettings::Preset>()
            ->Version(1)
            ->Field("name", &AnimGraphGameControllerSettings::Preset::m_name)
            ->Field("parameterInfos", &AnimGraphGameControllerSettings::Preset::m_parameterInfos)
            ->Field("buttonInfos", &AnimGraphGameControllerSettings::Preset::m_buttonInfos)
            ;
    }


    void AnimGraphGameControllerSettings::Reflect(AZ::ReflectContext* context)
    {
        ParameterInfo::Reflect(context);
        ButtonInfo::Reflect(context);
        Preset::Reflect(context);


        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphGameControllerSettings>()
            ->Version(1)
            ->Field("activePresetIndex", &AnimGraphGameControllerSettings::m_activePresetIndex)
            ->Field("presets", &AnimGraphGameControllerSettings::m_presets)
            ;
    }
} // namespace EMotionFX