/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_PROFILER_DATAAGGREGATOR_TESTS_H
#define DRILLER_PROFILER_DATAAGGREGATOR_TESTS_H

#if !defined(Q_MOC_RUN)
#include "Source/Driller/DrillerAggregator.hxx"
#include "Source/Driller/DrillerAggregatorOptions.hxx"
#include "AzCore/std/string/string.h"
#include "AzCore/std/containers/map.h"
#include "AzCore/RTTI/RTTI.h"
#include "AzCore/Memory/SystemAllocator.h"

#include "ProfilerDataParser.h"
#endif

namespace AZ
{
    class SerializeContext;
}

namespace Driller
{
    class ProfilerDrillerNewRegisterEvent;
    class ProfilerDrillerEnterThreadEvent;
    class ProfilerDrillerRegisterSystemEvent;
    class ProfilerDataAggregatorSavedState;

    /**
     * Profiler data drilling aggregator.
     */
    class ProfilerDataAggregator : public Aggregator
    {
        friend class ProfilerDataView;
        ProfilerDataAggregator(const ProfilerDataAggregator&) = delete;
        Q_OBJECT;
    public:
        AZ_RTTI(ProfilerDataAggregator, "{0DDEB1EA-0D49-4A5E-866A-885F51231FDA}");
        AZ_CLASS_ALLOCATOR(ProfilerDataAggregator,AZ::SystemAllocator,0);

        ProfilerDataAggregator(int identity = 0);
        virtual ~ProfilerDataAggregator();

        static AZ::u32 DrillerId()                                            { return ProfilerDrillerHandlerParser::GetDrillerId(); }
        virtual AZ::u32 GetDrillerId() const                                { return DrillerId(); }

        static const char* ChannelName() { return "Timing"; }
        virtual AZ::Crc32 GetChannelId() const override { return AZ::Crc32(ChannelName()); }

        virtual AZ::Debug::DrillerHandlerParser* GetDrillerDataParser()     { return &m_parser; }

        virtual void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*);
        virtual void ActivateWorkspaceSettings(WorkspaceSettingsProvider*);
        virtual void SaveSettingsToWorkspace(WorkspaceSettingsProvider*);

        /// Called after an event has been loaded
        void OnEventLoaded(DrillerEvent* event);

        void KillAllViews();

        //////////////////////////////////////////////////////////////////////////
        // Aggregator
        
        virtual void Reset();
    
    public slots:
        float ValueAtFrame( FrameNumberType frame ) override;
        QColor GetColor() const override;        
        QString GetName() const override;
        QString GetChannelName() const override;
        QString GetDescription() const override;
        QString GetToolTip() const override;
        AZ::Uuid GetID() const override;
        QWidget* DrillDownRequest(FrameNumberType frame) override;
        QWidget* DrillDownRequest(FrameNumberType frame, int viewType);
        virtual void OptionsRequest();
        void OnDataViewDestroyed(QObject*);
    
    //protected:
    public:
        typedef AZStd::unordered_map<AZ::u64,ProfilerDrillerNewRegisterEvent*> RegisterMapType;
        typedef AZStd::unordered_map<AZ::u64,ProfilerDrillerEnterThreadEvent*> ThreadMapType;
        typedef AZStd::multimap<AZ::u64,ProfilerDrillerEnterThreadEvent*> ThreadMultiMapType;
        typedef AZStd::unordered_map<AZ::u32,ProfilerDrillerRegisterSystemEvent*> SystemMapType;        

        /**
         * Map with all systems in use (system is a logical group of registers, 
         * which we can enable/disable sampling in order to improve performance and data granularity
         */
        SystemMapType                    m_systems;        
        ThreadMapType                    m_threads;                ///< Map with all the threads which are currently running.
        /**
         * Map with all the threads we have ever encountered. 
         * IMPORTANT: Thread which were NOT reported to AZStd::ThreadEventBus
         * will still be in the map, but the ProfilerDrillerEnterThreadEvent* pointer will be null.
         * make sure your code accounts for that.
         */
        ThreadMultiMapType                m_lifeTimeThreads;        
        RegisterMapType                    m_registers;

        AZStd::vector<ProfilerDrillerNewRegisterEvent*> m_allRegistersOfInterestInData;
        AZStd::vector<AZ::u64> m_allCorrespondingIdsForRegistersOfInterestInData;
        AZ::u64 m_currentDisplayRegister;

        QObject*                        m_dataView;
        ProfilerDrillerHandlerParser    m_parser;                ///< Parser for this aggregator

        AZStd::intrusive_ptr<ProfilerDataAggregatorSavedState> m_persistentState;

        static void Reflect(AZ::ReflectContext* context);
    };

}

#endif
