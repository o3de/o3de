/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Driller/DrillerConsoleAPI.h>

#include <AzCore/Driller/Driller.h>
#include <AzCore/Driller/DefaultStringPool.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/parallel/condition_variable.h>

//#define ENABLE_COMPRESSION_FOR_REMOTE_DRILLER

namespace AZ
{
    struct ClassDataReflection;
}

namespace AzFramework
{
    /**
     * Runs on the machine being drilled and is responsible for communications
     * with the DrillerNetworkConsole running on the tool side as well as
     * creating DrillerNetSessionStreams for each driller session being started.
     */
    class DrillToFileComponent
        : public AZ::Component
        , public AZ::Debug::DrillerOutputStream
        , public DrillerConsoleCommandBus::Handler
    {
    public:
        AZ_COMPONENT(DrillToFileComponent, "{42BAA25D-7CEB-4A37-8BD4-4A1FE2253894}")

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        static void Reflect(AZ::ReflectContext* context);
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // DrillerOutputStream
        void WriteBinary(const void* data, unsigned int dataSize) override;
        void OnEndOfFrame() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // DrillerConsoleCommandBus
        void EnumerateAvailableDrillers() override;
        void StartDrillerSession(const AZ::Debug::DrillerManager::DrillerListType& requestedDrillers, AZ::u64 sessionId) override;
        void StopDrillerSession(AZ::u64 sessionId) override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        void AsyncWritePump();

        AZ::u64 m_sessionId;
        AZ::Debug::DrillerSession* m_drillerSession;
        AZ::Debug::DrillerDefaultStringPool m_stringPool;
        AZStd::vector<AZ::u8, AZ::OSStdAllocator> m_frameBuffer;
        AZStd::deque<AZStd::vector<AZ::u8, AZ::OSStdAllocator>, AZ::OSStdAllocator> m_writeQueue;
        AZStd::mutex m_writerMutex;
        AZStd::condition_variable m_signal;
        AZStd::thread m_writerThread;
        bool m_isWriterEnabled;
    };
}   // namespace AzFramework
