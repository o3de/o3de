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

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/Array.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXConfig.h>


namespace EMotionFX
{
    class EMFX_API AnimGraphGameControllerSettings
    {
    public:
        AZ_RTTI(AnimGraphGameControllerSettings, "{05DF1B3B-2073-4E6D-B5B6-7B87F46CCCB7}");
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphGameControllerSettings();
        virtual ~AnimGraphGameControllerSettings();

        enum ParameterMode : AZ::u8
        {
            PARAMMODE_STANDARD                          = 0,
            PARAMMODE_ZEROTOONE                         = 1,
            PARAMMODE_PARAMRANGE                        = 2,
            PARAMMODE_POSITIVETOPARAMRANGE              = 3,
            PARAMMODE_NEGATIVETOPARAMRANGE              = 4,
            PARAMMODE_ROTATE_CHARACTER                  = 5
        };

        enum ButtonMode : AZ::u8
        {
            BUTTONMODE_NONE                             = 0,
            BUTTONMODE_SWITCHSTATE                      = 1,
            BUTTONMODE_TOGGLEBOOLEANPARAMETER           = 2,
            BUTTONMODE_ENABLEBOOLWHILEPRESSED           = 3,
            BUTTONMODE_DISABLEBOOLWHILEPRESSED          = 4,
            BUTTONMODE_ENABLEBOOLFORONLYONEFRAMEONLY    = 5
        };

        struct EMFX_API ParameterInfo final
        {
            AZ_RTTI(AnimGraphGameControllerSettings::ParameterInfo, "{C3220DB3-54FA-4719-80F0-CEAE5859C641}");
            AZ_CLASS_ALLOCATOR_DECL

            ParameterInfo();
            ParameterInfo(const char* parameterName);
            virtual ~ParameterInfo() = default;

            static void Reflect(AZ::ReflectContext* context);

            AZStd::string           m_parameterName;
            ParameterMode           m_mode;
            bool                    m_invert;
            bool                    m_enabled;
            uint8                   m_axis;
        };

        struct EMFX_API ButtonInfo final
        {
            AZ_RTTI(AnimGraphGameControllerSettings::ButtonInfo, "{94027445-C44F-4310-9DF2-1A2F39518578}");
            AZ_CLASS_ALLOCATOR_DECL

            ButtonInfo();
            ButtonInfo(AZ::u32 buttonIndex);
            virtual ~ButtonInfo() = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::u32                 m_buttonIndex;
            ButtonMode              m_mode;
            AZStd::string           m_string;                /**< Mostly used to store the attribute or parameter name to which this button belongs to. */
            bool                    m_oldIsPressed;
            bool                    m_enabled;
        };

        class EMFX_API Preset
        {
        public:
            AZ_RTTI(AnimGraphGameControllerSettings::Preset, "{51F08C40-B249-4F6D-BE82-D16721816A60}");
            AZ_CLASS_ALLOCATOR_DECL

            Preset();
            Preset(const char* name);
            virtual ~Preset();

            // try to find the parameter or button info and create a new one for the given name or index in case no existing one has been found
            ParameterInfo* FindParameterInfo(const char* parameterName);
            ButtonInfo* FindButtonInfo(AZ::u32 buttonIndex);

            /**
             * Check if the parameter with the given name is being controlled by the gamepad.
             * This assumes that the mString member from the ButtonInfo contains the parameter name.
             * @param[in] stringName The name to compare against the mString member of the button infos.
             * @result True in case a button info with the given string name doesn't have BUTTONMODE_NONE assigned, false in the other case.
             */
            bool CheckIfIsParameterButtonControlled(const char* stringName);

            /**
             * Check if any of the button infos that are linked to the given string name is enabled.
             * This assumes that the mString member from the ButtonInfo contains the parameter name.
             * @param[in] stringName The name to compare against the mString member of the button infos.
             * @result True in case any of the button infos with the given string name is enabled.
             */
            bool CheckIfIsButtonEnabled(const char* stringName);

            /**
             * Set all button infos that are linked to the given string name to the enabled flag.
             * This assumes that the mString member from the ButtonInfo contains the parameter name.
             * @param[in] stringName The name to compare against the mString member of the button infos.
             * @param[in] isEnabled True in case the button infos shall be enabled, false if they shall become disabled.
             */
            void SetButtonEnabled(const char* stringName, bool isEnabled);

            void Clear();

            void SetName(const char* name)                              { m_name = name; }
            const char* GetName() const                                 { return m_name.c_str(); }
            const AZStd::string& GetNameString() const                  { return m_name; }

            void SetNumParamInfos(size_t numParamInfos)                 { m_parameterInfos.resize(numParamInfos); }
            void SetParamInfo(size_t index, ParameterInfo* paramInfo)   { m_parameterInfos[index] = paramInfo; }
            size_t GetNumParamInfos() const                             { return m_parameterInfos.size(); }
            ParameterInfo* GetParamInfo(size_t index) const             { return m_parameterInfos[index]; }

            void SetNumButtonInfos(size_t numButtonInfos)               { m_buttonInfos.resize(numButtonInfos); }
            void SetButtonInfo(size_t index, ButtonInfo* buttonInfo)    { m_buttonInfos[index] = buttonInfo; }
            size_t GetNumButtonInfos() const                            { return m_buttonInfos.size(); }
            ButtonInfo* GetButtonInfo(size_t index) const               { return m_buttonInfos[index]; }

            static void Reflect(AZ::ReflectContext* context);

        private:
            AZStd::vector<ParameterInfo*>    m_parameterInfos;
            AZStd::vector<ButtonInfo*>       m_buttonInfos;
            AZStd::string                    m_name;
        };

        void AddPreset(Preset* preset);
        void RemovePreset(size_t index);
        void SetNumPresets(size_t numPresets);
        void SetPreset(size_t index, Preset* preset);
        void Clear();

        void OnParameterNameChange(const char* oldName, const char* newName);

        Preset* GetPreset(size_t index) const;
        size_t GetNumPresets() const;

        uint32 GetActivePresetIndex() const;
        Preset* GetActivePreset() const;
        void SetActivePreset(Preset* preset);

        size_t FindPresetIndexByName(const char* presetName) const;
        size_t FindPresetIndex(Preset* preset) const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<Preset*>   m_presets;
        AZ::u32                  m_activePresetIndex;
    };
} // namespace EMotionFX
