/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_ANNOTATIONS_H
#define DRILLER_ANNOTATIONS_H

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Math/Crc.h>
#include <QObject>

#include <Source/Driller/DrillerDataTypes.h>
#endif

#pragma once

namespace AZ { class ReflectContext; }

namespace Driller
{
    // ------------------------------------------------------------------------------------------------------------
    // Annotation
    // represents one annotation returned or fed into the annotations interface.
    class Annotation
    {
    public:
        AZ_CLASS_ALLOCATOR(Annotation, AZ::SystemAllocator, 0);
        Annotation();
        Annotation(AZ::s64 eventID, FrameNumberType frame, const char* text, const char* channel);
        Annotation(const Annotation& other);
        Annotation(Annotation&& other);
        Annotation& operator=(Annotation&& other);
        Annotation& operator=(const Annotation& other);

        AZ::s64 GetEventIndex() const  { return m_eventIndex; }
        FrameNumberType GetFrameIndex() const { return m_frameIndex; }
        const AZStd::string& GetText() const { return m_text; }
        const AZStd::string& GetChannel() const { return m_channel; }
        const AZ::u32 GetChannelCRC() const { return m_ChannelCRC; }

    private:
        AZ::s64 m_eventIndex;
        FrameNumberType m_frameIndex;
        AZStd::string m_text;
        AZStd::string m_channel;
        AZ::u32 m_ChannelCRC;
    };

    // ------------------------------------------------------------------------------------------------------------
    // AnnotationsProviderInterface
    // a class which provides annotation information to the parts of the system that care about annotations.
    // other parts of the system that want to know what annotations occur where will be given a pointer to this guy
    // and they will ask him what they need to know.
    // PLEASE NOTE:  This is essentially a live cache of what's currently in the view range and is for rendering only.
    // its essentially destroyed and recreated every frame.

    class AnnotationWorkspaceSettings;
    class AnnotationUserSettings;
    class WorkspaceSettingsProvider;

    // contains a set of channel names
    typedef AZStd::unordered_set<AZStd::string> ChannelContainer;
    typedef AZStd::unordered_set<AZ::u32> ChannelCRCContainer;

    class AnnotationsProvider
        : public QObject
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AnnotationsProvider, AZ::SystemAllocator, 0);

        typedef AZStd::vector<Annotation> AnnotationContainer;
        typedef AnnotationContainer::const_iterator ConstAnnotationIterator;

        // graph renderers need to know what color to draw annotations, and what annotations exist given a particular event or frame:
        ConstAnnotationIterator GetEnd() const; // returns the end of the vector (ie, the invalid iterator)
        ConstAnnotationIterator GetFirstAnnotationForFrame(FrameNumberType frameIndex) const; // returns iterator to the first annotation thats on the frame given (or the end iterator)
        ConstAnnotationIterator GetAnnotationForEvent(EventNumberType eventIndex) const; // returns iterator to the first annotation that is for that event # (or the end iterator)

        // claims ownership of the given annotation, via r-value ref.  This is used during populate.
        void AddAnnotation(Annotation&& target);

        // configure a channel
        void ConfigureChannel(const char* channel, QColor color);

        AnnotationsProvider(QObject* pParent = NULL);
        ~AnnotationsProvider();

        // called by the main controller to sort and build the map cache.
        void Finalize();
        void Clear();
        static void Reflect(AZ::ReflectContext* context);

        // --- channel management ---
        const ChannelContainer& GetAllKnownChannels() const;
        void GetCurrentlyEnabledChannelCRCs(ChannelCRCContainer& target) const;
        void NotifyOfChannelExistence(const char* name);
        void SetChannelEnabled(const char* channelName, bool enabled);
        bool IsChannelEnabled(AZ::u32 channelNameCRC) const;
        QColor GetColorForChannel(AZ::u32 channelNameCRC) const; // each channel has a color, this is configured externally.
        void SetColorForChannel(AZ::u32 channelNameCRC, QColor newColor); // each channel has a color, this is configured externally.
        void ResetColorForChannel(AZ::u32 channelNameCRC); // each channel has a color, this is configured externally.

        void LoadSettingsFromWorkspace(WorkspaceSettingsProvider* ptrProvider);
        void SaveSettingsToWorkspace(WorkspaceSettingsProvider* ptrProvider);
    signals:
        void KnownAnnotationsChanged();
        void SelectedAnnotationsChanged();
        void AnnotationDataInvalidated();

    protected:
        AnnotationContainer m_currentAnnotations;
        typedef AZStd::unordered_map<EventNumberType, AZStd::size_t> EventIndexToCurrentMap; // maps from event index to index in our vector.
        typedef AZStd::unordered_map<FrameNumberType, AZStd::size_t> FrameIndexToCurrentMap; // maps from frame index to index in our vector.
        EventIndexToCurrentMap m_eventToIndex;
        FrameIndexToCurrentMap m_frameToIndex;

        // housekeeping
        bool m_bVectorDirty;

        AZStd::intrusive_ptr<AnnotationWorkspaceSettings> m_ptrWorkspaceSettings; // loaded from workspace and user settings. // crc(somethingelse)
        AZStd::intrusive_ptr<AnnotationUserSettings> m_ptrUserSettings; // loaded only from user settings / CRC(whatever)
    };
}

#endif//DRILLER_ANNOTATIONS_H
