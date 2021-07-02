/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"
#include "Annotations.hxx"
#include <Source/Driller/Workspaces/Workspace.h>
#include <AzCore/std/sort.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/ObjectStream.h>

#include <QColor>
#include <QRgb>

namespace Driller
{
    // stores the settings that are saved into the file and transported from user to user to accompany drill files.
    // as always, this is just a dumb container and does not need encapsulation
    class AnnotationWorkspaceSettings
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(AnnotationWorkspaceSettings, "{431EFFCF-C3C5-4BB3-8246-E452E11D4FF8}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(AnnotationWorkspaceSettings,  AZ::SystemAllocator, 0);

        ChannelContainer m_ActiveAnnotationChannels;
        ChannelCRCContainer m_ActiveAnnotationChannelCRCs;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AnnotationWorkspaceSettings>()
                    ->Version(2)
                    ->Field("m_ActiveAnnotationChannels", &AnnotationWorkspaceSettings::m_ActiveAnnotationChannels)
                    ->Field("m_ActiveAnnotationChannelCRCs", &AnnotationWorkspaceSettings::m_ActiveAnnotationChannelCRCs);
            }
        }
    };

    // stores the data that goes with the user preferences, even without a workspace file
    // mainly gui stuff...
    // as always, this is just a dumb container and does not need encapsulation
    class AnnotationUserSettings
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(AnnotationUserSettings, "{D3584846-0574-4B63-9693-4F3265CDE16D}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(AnnotationUserSettings,  AZ::SystemAllocator, 0);

        ChannelContainer m_KnownAnnotationChannels; // keeps track of all annotation channels ever seen
        AZStd::unordered_map<AZ::u32, AZ::u32> m_customizedColors;

        AZ::u32 GetRGBAColorForChannel(AZ::u32 channelNameCRC)
        {
            auto found = m_customizedColors.find(channelNameCRC);
            if (found != m_customizedColors.end())
            {
                return found->second;
            }

            AZ::u32 num_different_colors = 7;
            QColor col;
            float sat = .9f;
            float val = .9f;
            col.setHsvF((float)(channelNameCRC % num_different_colors) / (float(num_different_colors)), sat, val);
            QRgb rgbColor = col.rgba();
            return rgbColor;
        }

        void SetRGBAColorForChannel(AZ::u32 channelNameCRC, AZ::u32 rgbaColor)
        {
            m_customizedColors[channelNameCRC] = rgbaColor;
        }

        void ResetColorForChannel(AZ::u32 channelNameCRC)
        {
            m_customizedColors.erase(channelNameCRC);
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AnnotationUserSettings>()
                    ->Version(1)
                    ->Field("m_KnownAnnotationChannels", &AnnotationUserSettings::m_KnownAnnotationChannels)
                    ->Field("m_customizedColors", &AnnotationUserSettings::m_customizedColors);
            }
        }
    };

    void AnnotationsProvider::Reflect(AZ::ReflectContext* context)
    {
        AnnotationWorkspaceSettings::Reflect(context);
        AnnotationUserSettings::Reflect(context);
    }

    Annotation::Annotation()
    {
        m_ChannelCRC = 0;
        m_eventIndex = 0;
        m_frameIndex = 0;
    }

    Annotation::Annotation(AZ::s64 eventID, FrameNumberType frame, const char* text, const char* channel)
    {
        m_eventIndex = eventID;
        m_frameIndex = frame;
        m_text = text;
        m_channel = channel;
        m_ChannelCRC = AZ::Crc32(m_channel.c_str());
    }

    Annotation::Annotation(const Annotation& other)
    {
        *this = other;
    }

    Annotation::Annotation(Annotation&& other)
    {
        *this = AZStd::move(other);
    }

    Annotation& Annotation::operator=(Annotation&& other)
    {
        if (this != &other)
        {
            m_eventIndex = other.m_eventIndex;
            m_frameIndex = other.m_frameIndex;
            m_text = AZStd::move(other.m_text);
            m_channel = AZStd::move(other.m_channel);
            m_ChannelCRC = other.m_ChannelCRC;
        }
        return *this;
    }

    Annotation& Annotation::operator=(const Annotation& other)
    {
        if (this != &other)
        {
            m_eventIndex = other.m_eventIndex;
            m_frameIndex = other.m_frameIndex;
            m_text = other.m_text;
            m_channel = other.m_channel;
            m_ChannelCRC = other.m_ChannelCRC;
        }
        return *this;
    }

    AnnotationsProvider::AnnotationsProvider(QObject* pParent)
        : QObject(pParent)
    {
        m_bVectorDirty = false;

        /// load user settings and workspace settings from usersettings component to persist state

        m_ptrUserSettings = AZ::UserSettings::CreateFind<AnnotationUserSettings>(AZ_CRC("ANNOT_USERSETTINGS", 0x3ddaa4f1), AZ::UserSettings::CT_GLOBAL);
        m_ptrWorkspaceSettings = AZ::UserSettings::CreateFind<AnnotationWorkspaceSettings>(AZ_CRC("ANNOT_WORKSPACESETTINGS", 0xf7ca8dd3), AZ::UserSettings::CT_GLOBAL);
    }

    AnnotationsProvider::~AnnotationsProvider()
    {
        AZ::UserSettings::Release(m_ptrUserSettings);
        AZ::UserSettings::Release(m_ptrWorkspaceSettings);
    }


    void AnnotationsProvider::LoadSettingsFromWorkspace(WorkspaceSettingsProvider* ptrProvider)
    {
        AnnotationWorkspaceSettings* rawPtr = ptrProvider->FindSetting<AnnotationWorkspaceSettings>(AZ_CRC("ANNOTATIONWORKSPACE", 0x28319f66));
        if (rawPtr)
        {
            m_ptrWorkspaceSettings->m_ActiveAnnotationChannels = rawPtr->m_ActiveAnnotationChannels;
            m_ptrWorkspaceSettings->m_ActiveAnnotationChannelCRCs = rawPtr->m_ActiveAnnotationChannelCRCs;

            // aggregate missing channels:
            for (auto it = m_ptrWorkspaceSettings->m_ActiveAnnotationChannels.begin(); it != m_ptrWorkspaceSettings->m_ActiveAnnotationChannels.end(); ++it)
            {
                NotifyOfChannelExistence(it->c_str());
            }
        }
    }

    void AnnotationsProvider::SaveSettingsToWorkspace(WorkspaceSettingsProvider* ptrProvider)
    {
        AnnotationWorkspaceSettings* rawPtr = ptrProvider->CreateSetting<AnnotationWorkspaceSettings>(AZ_CRC("ANNOTATIONWORKSPACE", 0x28319f66));
        rawPtr->m_ActiveAnnotationChannels = m_ptrWorkspaceSettings->m_ActiveAnnotationChannels;
        rawPtr->m_ActiveAnnotationChannelCRCs = m_ptrWorkspaceSettings->m_ActiveAnnotationChannelCRCs;
    }

    // returns the end of the vector (ie, the invalid iterator)
    AnnotationsProvider::ConstAnnotationIterator AnnotationsProvider::GetEnd() const
    {
        AZ_Assert(!m_bVectorDirty, "You may not interrogate the annotations provider before it is finalized");

        return m_currentAnnotations.end();
    }

    // returns iterator to the first annotation thats on the frame given (or the end iterator)
    AnnotationsProvider::ConstAnnotationIterator AnnotationsProvider::GetFirstAnnotationForFrame(FrameNumberType frameIndex) const
    {
        AZ_Assert(!m_bVectorDirty, "You may not interrogate the annotations provider before it is finalized");

        // do we have annotations for that framE?
        FrameIndexToCurrentMap::const_iterator it = m_frameToIndex.find(frameIndex);
        if (it == m_frameToIndex.end())
        {
            return GetEnd();
        }

        return m_currentAnnotations.begin() + it->second;
    }

    // returns iterator to the first annotation that is foer that event # (or the end iterator)
    AnnotationsProvider::ConstAnnotationIterator AnnotationsProvider::GetAnnotationForEvent(EventNumberType eventIndex) const
    {
        AZ_Assert(!m_bVectorDirty, "You may not interrogate the annotations provider before it is finalized");

        // do we have annotations for that event index?
        EventIndexToCurrentMap::const_iterator it = m_eventToIndex.find(eventIndex);
        if (it == m_eventToIndex.end())
        {
            return GetEnd();
        }

        return m_currentAnnotations.begin() + it->second;
    }

    // note:: claims ownership of the data in annotation.
    void AnnotationsProvider::AddAnnotation(Annotation&& target)
    {
        if (m_eventToIndex.find(target.GetEventIndex()) != m_eventToIndex.end())
        {
            return;
        }

        // can we do this quickly and easily?
        if (
            (!m_bVectorDirty) &&  // if we're not already going to need to sort, and...
            (
                (m_currentAnnotations.empty()) ||    // we either have no annotations or...
                (m_currentAnnotations.back().GetEventIndex() < target.GetEventIndex())     // the fresh annotation belongs at the end anyway and we will not need to re-sort.
            )
            )
        {
            // we're adding annotations onto the end, so we will not have to sort, we can just accumulate the data.

            m_eventToIndex[target.GetEventIndex()] = m_currentAnnotations.size();

            // if its the first annotation for this frame, we can also add it:
            if (m_frameToIndex.find(target.GetFrameIndex()) == m_frameToIndex.end())
            {
                m_frameToIndex[target.GetFrameIndex()] = m_currentAnnotations.size();
            }

            m_currentAnnotations.push_back(target);
        }
        else
        {
            // we're adding annotations out of order, we have to re-sort:
            m_currentAnnotations.push_back(target);
            m_bVectorDirty = true;
        }
    }


    // called by the main controller to sort and build the map cache.
    void AnnotationsProvider::Finalize()
    {
        /*
        // temp: add some fake annots
        AddAnnotation(Annotation(5, 10, "Test annotation", "tracker"));
        AddAnnotation(Annotation(15, 110, "Test annotation2", "tracker1"));
        AddAnnotation(Annotation(25, 210, "Test annotatio3n", "tracker2"));
        AddAnnotation(Annotation(35, 310, "Test annotation4", "tracker3"));
        */

        if (!m_bVectorDirty)
        {
            emit AnnotationDataInvalidated();

            return;
        }

        m_frameToIndex.clear();
        m_eventToIndex.clear();

        // sort them so they are in order from beginning to end:
        AZStd::sort(
            m_currentAnnotations.begin(),
            m_currentAnnotations.end(),
            [](const Annotation& a, const Annotation& b) -> bool
            {
                return a.GetEventIndex() < b.GetEventIndex();
            }
            );

        // now build the lookup tables:
        FrameNumberType lastFrameIndex = -1;
        for (AZStd::size_t idx = 0, endIdx = m_currentAnnotations.size(); idx < endIdx; ++idx)
        {
            const Annotation& current = m_currentAnnotations[idx];
            m_eventToIndex[current.GetEventIndex()] = idx;
            if (lastFrameIndex != current.GetFrameIndex())
            {
                m_frameToIndex[current.GetFrameIndex()] = idx;
                lastFrameIndex = current.GetFrameIndex();
            }
        }

        emit AnnotationDataInvalidated();

        m_bVectorDirty = false;
    }

    const ChannelContainer& AnnotationsProvider::GetAllKnownChannels() const
    {
        return m_ptrUserSettings->m_KnownAnnotationChannels;
    }

    void AnnotationsProvider::GetCurrentlyEnabledChannelCRCs(ChannelCRCContainer& target) const
    {
        target.insert(m_ptrWorkspaceSettings->m_ActiveAnnotationChannelCRCs.begin(), m_ptrWorkspaceSettings->m_ActiveAnnotationChannelCRCs.end());
    }

    // let us know that a channel exists:
    void AnnotationsProvider::NotifyOfChannelExistence(const char* name)
    {
        if (m_ptrUserSettings->m_KnownAnnotationChannels.insert(name).second)
        {
            emit KnownAnnotationsChanged();
        }
    }

    void AnnotationsProvider::SetChannelEnabled(const char* channelName, bool enabled)
    {
        if (enabled)
        {
            if (m_ptrWorkspaceSettings->m_ActiveAnnotationChannels.insert(channelName).second)
            {
                m_ptrWorkspaceSettings->m_ActiveAnnotationChannelCRCs.insert(AZ::Crc32(channelName));
                NotifyOfChannelExistence(channelName);
                emit SelectedAnnotationsChanged();
            }
        }
        else
        {
            AZ::u32 channelCRC = AZ::Crc32(channelName);
            if (IsChannelEnabled(channelCRC))
            {
                m_ptrWorkspaceSettings->m_ActiveAnnotationChannels.erase(channelName);
                m_ptrWorkspaceSettings->m_ActiveAnnotationChannelCRCs.erase(channelCRC);
                emit SelectedAnnotationsChanged();
            }
        }
    }

    QColor AnnotationsProvider::GetColorForChannel(AZ::u32 channelNameCRC) const
    {
        QRgb rgbaValue = m_ptrUserSettings->GetRGBAColorForChannel(channelNameCRC);
        return QColor(rgbaValue);
    }

    void AnnotationsProvider::SetColorForChannel(AZ::u32 channelNameCRC, QColor newColor)
    {
        QRgb rgbaValue = newColor.rgba();
        m_ptrUserSettings->SetRGBAColorForChannel(channelNameCRC, rgbaValue);

        if (IsChannelEnabled(channelNameCRC))
        {
            // update displays
            emit SelectedAnnotationsChanged();
        }
    }

    void AnnotationsProvider::ResetColorForChannel(AZ::u32 channelNameCRC)
    {
        m_ptrUserSettings->ResetColorForChannel(channelNameCRC);
    }

    bool AnnotationsProvider::IsChannelEnabled(AZ::u32 channelNameCRC) const
    {
        return (m_ptrWorkspaceSettings->m_ActiveAnnotationChannelCRCs.find(channelNameCRC) != m_ptrWorkspaceSettings->m_ActiveAnnotationChannelCRCs.end());
    }

    void AnnotationsProvider::Clear()
    {
        m_frameToIndex.clear();
        m_eventToIndex.clear();
        m_currentAnnotations.clear();
        m_bVectorDirty = false;
    }
}

#include <Source/Driller/Annotations/moc_Annotations.cpp>
