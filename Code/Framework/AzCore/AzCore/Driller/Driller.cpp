/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Driller/Driller.h>
#include <AzCore/Driller/DrillerBus.h>

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/Math/Crc.h>


namespace AZ
{
    namespace Debug
    {
        class DrillerManagerImpl
            : public DrillerManager
        {
        public:
            AZ_CLASS_ALLOCATOR(DrillerManagerImpl, OSAllocator, 0);

            typedef forward_list<DrillerSession>::type SessionListType;
            SessionListType m_sessions;
            typedef vector<Driller*>::type DrillerArrayType;
            DrillerArrayType m_drillers;

            ~DrillerManagerImpl() override;

            void Register(Driller* factory) override;
            void Unregister(Driller* factory) override;

            void FrameUpdate() override;

            DrillerSession*     Start(DrillerOutputStream& output, const DrillerListType& drillerList, int numFrames = -1) override;
            void                Stop(DrillerSession* session) override;

            int                 GetNumDrillers() const override  { return static_cast<int>(m_drillers.size()); }
            Driller*            GetDriller(int index) override   { return m_drillers[index]; }
        };

        //////////////////////////////////////////////////////////////////////////
        // Driller

        //=========================================================================
        // Register
        // [3/17/2011]
        //=========================================================================
        AZ::u32 Driller::GetId() const
        {
            return AZ::Crc32(GetName());
        }

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Driller Manager

        //=========================================================================
        // Register
        // [3/17/2011]
        //=========================================================================
        DrillerManager* DrillerManager::Create(/*const Descriptor& desc*/)
        {
            const bool createAllocator = !AZ::AllocatorInstance<OSAllocator>::IsReady();
            if (createAllocator)
            {
                AZ::AllocatorInstance<OSAllocator>::Create();
            }

            DrillerManagerImpl* impl = aznew DrillerManagerImpl;
            impl->m_ownsOSAllocator = createAllocator;
            return impl;
        }

        //=========================================================================
        // Register
        // [3/17/2011]
        //=========================================================================
        void DrillerManager::Destroy(DrillerManager* manager)
        {
            const bool allocatorCreated = manager->m_ownsOSAllocator;
            delete manager;
            if (allocatorCreated)
            {
                AZ::AllocatorInstance<OSAllocator>::Destroy();
            }
        }

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // DrillerManagerImpl

        //=========================================================================
        // ~DrillerManagerImpl
        // [3/17/2011]
        //=========================================================================
        DrillerManagerImpl::~DrillerManagerImpl()
        {
            while (!m_sessions.empty())
            {
                Stop(&m_sessions.front());
            }

            while (!m_drillers.empty())
            {
                Driller* driller = m_drillers[0];
                Unregister(driller);
                delete driller;
            }
        }

        //=========================================================================
        // Register
        // [3/17/2011]
        //=========================================================================
        void
        DrillerManagerImpl::Register(Driller* driller)
        {
            AZ_Assert(driller, "You must provide a valid factory!");
            for (size_t i = 0; i < m_drillers.size(); ++i)
            {
                if (m_drillers[i]->GetId() == driller->GetId())
                {
                    AZ_Error("Debug", false, "Driller with id %08x has already been registered! You can't have two factory instances for the same driller type", driller->GetId());
                    return;
                }
            }
            m_drillers.push_back(driller);
        }

        //=========================================================================
        // Unregister
        // [3/17/2011]
        //=========================================================================
        void
        DrillerManagerImpl::Unregister(Driller* driller)
        {
            AZ_Assert(driller, "You must provide a valid factory!");
            for (DrillerArrayType::iterator iter = m_drillers.begin(); iter != m_drillers.end(); ++iter)
            {
                if ((*iter)->GetId() == driller->GetId())
                {
                    m_drillers.erase(iter);
                    return;
                }
            }

            AZ_Error("Debug", false, "Failed to find driller factory with id %08x", driller->GetId());
        }

        //=========================================================================
        // FrameUpdate
        // [3/17/2011]
        //=========================================================================
        void
        DrillerManagerImpl::FrameUpdate()
        {
            if (m_sessions.empty())
            {
                return;
            }

            AZStd::lock_guard<DrillerEBusMutex::MutexType> lock(DrillerEBusMutex::GetMutex());  ///< Make sure no driller is writing to the stream
            for (SessionListType::iterator sessionIter = m_sessions.begin(); sessionIter != m_sessions.end(); )
            {
                DrillerSession& s = *sessionIter;

                // tick the drillers directly if they care.
                for (size_t i = 0; i < s.drillers.size(); ++i)
                {
                    s.drillers[i]->Update();
                }

                s.output->EndTag(AZ_CRC("Frame", 0xb5f83ccd));

                s.output->OnEndOfFrame();

                s.curFrame++;

                if (s.numFrames != -1)
                {
                    if (s.curFrame == s.numFrames)
                    {
                        Stop(&s);
                        continue;
                    }
                }

                s.output->BeginTag(AZ_CRC("Frame", 0xb5f83ccd));
                s.output->Write(AZ_CRC("FrameNum", 0x85a1a919), s.curFrame);

                ++sessionIter;
            }
        }

        //=========================================================================
        // Start
        // [3/17/2011]
        //=========================================================================
        DrillerSession*
        DrillerManagerImpl::Start(DrillerOutputStream& output, const DrillerListType& drillerList, int numFrames)
        {
            if (drillerList.empty())
            {
                return nullptr;
            }

            m_sessions.push_back();
            DrillerSession& s = m_sessions.back();
            s.curFrame = 0;
            s.numFrames = numFrames;
            s.output = &output;

            s.output->WriteHeader(); // first write the header in the stream

            s.output->BeginTag(AZ_CRC("StartData", 0xecf3f53f));
            s.output->Write(AZ_CRC("Platform", 0x3952d0cb), (unsigned int)g_currentPlatform);
            for (DrillerListType::const_iterator iDriller = drillerList.begin(); iDriller != drillerList.end(); ++iDriller)
            {
                const DrillerInfo& di = *iDriller;
                s.output->BeginTag(AZ_CRC("Driller", 0xa6e1fb73));
                s.output->Write(AZ_CRC("Name", 0x5e237e06), di.id);
                for (int iParam = 0; iParam < (int)di.params.size(); ++iParam)
                {
                    s.output->BeginTag(AZ_CRC("Param", 0xa4fa7c89));
                    s.output->Write(AZ_CRC("Name", 0x5e237e06), di.params[iParam].name);
                    s.output->Write(AZ_CRC("Description", 0x6de44026), di.params[iParam].desc);
                    s.output->Write(AZ_CRC("Type", 0x8cde5729), di.params[iParam].type);
                    s.output->Write(AZ_CRC("Value", 0x1d775834), di.params[iParam].value);
                    s.output->EndTag(AZ_CRC("Param", 0xa4fa7c89));
                }
                s.output->EndTag(AZ_CRC("Driller", 0xa6e1fb73));
            }
            s.output->EndTag(AZ_CRC("StartData", 0xecf3f53f));

            s.output->BeginTag(AZ_CRC("Frame", 0xb5f83ccd));
            s.output->Write(AZ_CRC("FrameNum", 0x85a1a919), s.curFrame);

            {
                AZStd::lock_guard<DrillerEBusMutex::MutexType> lock(DrillerEBusMutex::GetMutex());  ///< Make sure no driller is writing to the stream
                for (DrillerListType::const_iterator iDriller = drillerList.begin(); iDriller != drillerList.end(); ++iDriller)
                {
                    Driller* driller = nullptr;
                    const DrillerInfo& di = *iDriller;
                    for (size_t iDesc = 0; iDesc < m_drillers.size(); ++iDesc)
                    {
                        if (m_drillers[iDesc]->GetId() == di.id)
                        {
                            driller = m_drillers[iDesc];
                            AZ_Assert(driller->m_output == nullptr, "Driller with id %08x is already have an output stream %p (currently we support only 1 at a time)", di.id, driller->m_output);
                            driller->m_output = &output;
                            driller->Start(di.params.data(), static_cast<unsigned int>(di.params.size()));
                            s.drillers.push_back(driller);
                            break;
                        }
                    }
                    AZ_Warning("Driller", driller != nullptr, "We can't start a driller with id %d!", di.id);
                }
            }
            return &s;
        }


        //=========================================================================
        // Stop
        // [3/17/2011]
        //=========================================================================
        void
        DrillerManagerImpl::Stop(DrillerSession* session)
        {
            SessionListType::iterator iter;
            for (iter = m_sessions.begin(); iter != m_sessions.end(); ++iter)
            {
                if (&*iter == session)
                {
                    break;
                }
            }

            AZ_Assert(iter != m_sessions.end(), "We did not find session ID 0x%08x in the list!", session);
            if (iter != m_sessions.end())
            {
                DrillerSession& s = *session;

                {
                    AZStd::lock_guard<DrillerEBusMutex::MutexType> lock(DrillerEBusMutex::GetMutex());
                    for (size_t i = 0; i < s.drillers.size(); ++i)
                    {
                        s.drillers[i]->Stop();
                        s.drillers[i]->m_output = nullptr;
                    }
                }
                s.output->EndTag(AZ_CRC("Frame", 0xb5f83ccd));
                m_sessions.erase(iter);
            }
        }
    } // namespace Debug
} // namespace AZ
