/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include "EMStudioConfig.h"

#include <EMotionFX/Source/Event.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#endif


namespace EMStudio
{
    class EMSTUDIO_API MotionEventPreset
    {
    public:
        AZ_RTTI(EMStudio::MotionEventPreset, "{EDE6662A-32C4-4DE1-9EC5-19C9F506ACAE}")
        AZ_CLASS_ALLOCATOR(MotionEventPreset, EMStudio::UIAllocator)

        MotionEventPreset() = default;
        MotionEventPreset(const AZStd::string& name, EMotionFX::EventDataSet&& eventDatas, AZ::Color color, const AZStd::string& comment);

        virtual ~MotionEventPreset() = default;

        static void Reflect(AZ::ReflectContext* context);

        const EMotionFX::EventDataSet& GetEventDatas() const { return m_eventDatas; }
        EMotionFX::EventDataSet& GetEventDatas() { return m_eventDatas; }
        const AZStd::string& GetName() const { return m_name; }
        const AZ::Color& GetEventColor() const { return m_color; }
        bool GetIsDefault() const { return m_isDefault; }

        void SetName(const AZStd::string& name) { m_name = name; }
        void SetEventColor(AZ::u32 color) { m_color.FromU32(color); }
        void SetIsDefault(bool isDefault) { m_isDefault = isDefault; }

    private:
        EMotionFX::EventDataSet m_eventDatas;
        AZStd::string m_name;
        AZStd::string m_comment;
        AZ::Color m_color = AZ::Color::CreateOne();
        bool m_isDefault = false;
    };


    class EMSTUDIO_API MotionEventPresetManager
    {
    public:
        AZ_TYPE_INFO(EMStudio::MotionEventPresetManager, "{EEDD56F6-DDBC-40E7-A280-F2FBA09A63D4}")

        MotionEventPresetManager();
        ~MotionEventPresetManager();

        static void Reflect(AZ::ReflectContext* context);

        size_t GetNumPresets() const;
        bool IsEmpty() const;
        void AddPreset(MotionEventPreset* preset);
        void RemovePreset(size_t index);
        MotionEventPreset* GetPreset(size_t index) const;
        void Clear();

        void Load(const AZStd::string& filename);
        void Load()                                                             { Load(m_fileName); }
        void LoadFromSettings();
        void SaveAs(const AZStd::string& filename, bool showNotification=true);
        void Save(bool showNotification=true)                                   { SaveAs(m_fileName, showNotification); }

        bool GetIsDirty() const                                                 { return m_dirtyFlag; }
        void SetDirtyFlag(bool isDirty)                                         { m_dirtyFlag = isDirty; }
        const char* GetFileName() const                                         { return m_fileName.c_str(); }
        const AZStd::string& GetFileNameString() const                          { return m_fileName; }
        void SetFileName(const char* filename)                                  { m_fileName = filename; }

        AZ::Color GetEventColor(const EMotionFX::EventDataSet& eventDatas) const;

    private:
        bool LoadLYSerializedFormat();
        bool LoadLegacyQSettingsFormat();

        AZStd::vector<MotionEventPreset*>           m_eventPresets;
        AZStd::string                               m_fileName;
        bool                                        m_dirtyFlag;
        static const AZ::Color                        s_unknownEventColor;

        void SaveToSettings();
        void CreateDefaultPresets();
    };
} // namespace EMStudio
