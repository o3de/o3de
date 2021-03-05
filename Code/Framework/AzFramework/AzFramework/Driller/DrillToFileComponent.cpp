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

#include <AzFramework/Driller/DrillToFileComponent.h>

#include <AzCore/Driller/Driller.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>

namespace AzFramework
{
    void DrillToFileComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DrillToFileComponent, AZ::Component>()
                ;

            if (serialize->FindClassData(DrillerInfo::RTTI_Type()) == nullptr)
            {
                serialize->Class<DrillerInfo>()
                    ->Field("Id", &DrillerInfo::m_id)
                    ->Field("GroupName", &DrillerInfo::m_groupName)
                    ->Field("Name", &DrillerInfo::m_name)
                    ->Field("Description", &DrillerInfo::m_description);
            }
        }
    }

    void DrillToFileComponent::Activate()
    {
        m_drillerSession = nullptr;
        DrillerConsoleCommandBus::Handler::BusConnect();
    }

    void DrillToFileComponent::Deactivate()
    {
        DrillerConsoleCommandBus::Handler::BusDisconnect();
        StopDrillerSession(reinterpret_cast<AZ::u64>(this));
    }

    void DrillToFileComponent::WriteBinary(const void* data, unsigned int dataSize)
    {
        if (dataSize > 0)
        {
            m_frameBuffer.insert(m_frameBuffer.end(), reinterpret_cast<const AZ::u8*>(data), reinterpret_cast<const AZ::u8*>(data) + dataSize);
        }
    }

    void DrillToFileComponent::OnEndOfFrame()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_writerMutex);
        m_writeQueue.push_back();
        m_writeQueue.back().swap(m_frameBuffer);
        m_signal.notify_all();
    }

    void DrillToFileComponent::EnumerateAvailableDrillers()
    {
        DrillerInfoListType availableDrillers;

        AZ::Debug::DrillerManager* mgr = NULL;
        EBUS_EVENT_RESULT(mgr, AZ::ComponentApplicationBus, GetDrillerManager);
        if (mgr)
        {
            for (int i = 0; i < mgr->GetNumDrillers(); ++i)
            {
                AZ::Debug::Driller* driller = mgr->GetDriller(i);
                AZ_Assert(driller, "DrillerManager returned a NULL driller. This is not legal!");
                availableDrillers.push_back();
                availableDrillers.back().m_id = driller->GetId();
                availableDrillers.back().m_groupName = driller->GroupName();
                availableDrillers.back().m_name = driller->GetName();
                availableDrillers.back().m_description = driller->GetDescription();
            }
        }

        EBUS_EVENT(DrillerConsoleEventBus, OnDrillersEnumerated, availableDrillers);
    }

    void DrillToFileComponent::StartDrillerSession(const AZ::Debug::DrillerManager::DrillerListType& requestedDrillers, AZ::u64 sessionId)
    {
        if (!m_drillerSession)
        {
            AZ_Assert(m_writeQueue.empty(), "write queue is not empty!");

            m_sessionId = sessionId;
            AZ::Debug::DrillerManager* mgr = nullptr;
            EBUS_EVENT_RESULT(mgr, AZ::ComponentApplicationBus, GetDrillerManager);
            if (mgr)
            {
                SetStringPool(&m_stringPool);;
                m_drillerSession = mgr->Start(*this, requestedDrillers);

                AZStd::unique_lock<AZStd::mutex> signalLock(m_writerMutex);
                m_isWriterEnabled = true;
                AZStd::thread_desc td;
                td.m_name = "DrillToFileComponent Writer Thread";
                m_writerThread = AZStd::thread(AZStd::bind(&DrillToFileComponent::AsyncWritePump, this), &td);
                m_signal.wait(signalLock);

                EBUS_EVENT(DrillerConsoleEventBus, OnDrillerSessionStarted, sessionId);
            }
        }
    }

    void DrillToFileComponent::StopDrillerSession(AZ::u64 sessionId)
    {
        if (sessionId == m_sessionId)
        {
            if (m_drillerSession)
            {
                AZ::Debug::DrillerManager* mgr = NULL;
                EBUS_EVENT_RESULT(mgr, AZ::ComponentApplicationBus, GetDrillerManager);
                if (mgr)
                {
                    mgr->Stop(m_drillerSession);
                }
                m_drillerSession = nullptr;
                EBUS_EVENT(DrillerConsoleEventBus, OnDrillerSessionStopped, reinterpret_cast<AZ::u64>(this));
            }

            m_isWriterEnabled = false;
            if (m_writerThread.joinable())
            {
                m_writerMutex.lock();
                m_signal.notify_all();
                m_writerMutex.unlock();
                m_writerThread.join();
            }
            SetStringPool(nullptr);
            m_stringPool.Reset();
            m_frameBuffer.clear(); // there may be pending data but we don't want to write it because it's an incomplete frame.
        }
    }
    void DrillToFileComponent::AsyncWritePump()
    {
        AZStd::unique_lock<AZStd::mutex> signalLock(m_writerMutex);

        AZStd::basic_string<char, AZStd::char_traits<char>, AZ::OSStdAllocator> drillerOutputPath;

        // Try the log path first
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO)
        {
            const char* logLocation = fileIO->GetAlias("@log@");
            if (logLocation)
            {
                drillerOutputPath = logLocation;
                drillerOutputPath.append("/");
            }
        }

        // Try the executable path
        if (drillerOutputPath.empty())
        {
            EBUS_EVENT_RESULT(drillerOutputPath, AZ::ComponentApplicationBus, GetExecutableFolder);
            drillerOutputPath.append("/");
        }

        drillerOutputPath.append("drillerdata.drl");
        AZ::IO::SystemFile output;
        output.Open(drillerOutputPath.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
        AZ_Assert(output.IsOpen(), "Failed to open driller output file!");

        m_signal.notify_all();

        while (true)
        {
            while (!m_writeQueue.empty())
            {
                AZStd::vector<AZ::u8, AZ::OSStdAllocator> outBuffer;
                outBuffer.swap(m_writeQueue.front());
                m_writeQueue.pop_front();
                signalLock.unlock();

                output.Write(outBuffer.data(), outBuffer.size());
                output.Flush();

                signalLock.lock();
            }
            if (!m_isWriterEnabled)
            {
                break;
            }
            m_signal.wait(signalLock);
        }

        output.Close();
    }
}   // namespace AzFramework
