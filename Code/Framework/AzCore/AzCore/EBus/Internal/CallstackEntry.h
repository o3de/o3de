/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/Internal/Debug.h>

#include <AzCore/std/parallel/lock.h>

namespace AZ
{
    template <typename Interface, typename Traits>
    class EBus;

    namespace Internal
    {
        // Common implementation between single and multithreaded versions
        template <typename Interface, typename Traits>
        struct CallstackEntryBase
        {
            using BusType = EBus<Interface, Traits>;

            CallstackEntryBase(const typename Traits::BusIdType* id)
                : m_busId(id)
            { }

            virtual ~CallstackEntryBase() = default;

            virtual void OnRemoveHandler(Interface* handler)
            {
                if (m_prev)
                {
                    m_prev->OnRemoveHandler(handler);
                }
            }

            virtual void OnPostRemoveHandler()
            {
                if (m_prev)
                {
                    m_prev->OnPostRemoveHandler();
                }
            }

            virtual void SetRouterProcessingState(typename BusType::RouterProcessingState /*state*/) { }

            virtual bool IsRoutingQueuedEvent() const { return false; }

            virtual bool IsRoutingReverseEvent() const { return false; }

            const typename Traits::BusIdType* m_busId;
            CallstackEntryBase<Interface, Traits>* m_prev = nullptr;
        };

        // Single threaded callstack entry
        template <typename Interface, typename Traits>
        struct CallstackEntry
            : public CallstackEntryBase<Interface, Traits>
        {
            using BusType = EBus<Interface, Traits>;
            using BusContextPtr = typename BusType::Context*;

            CallstackEntry(BusContextPtr context, const typename Traits::BusIdType* busId)
                : CallstackEntryBase<Interface, Traits>(busId)
                , m_threadId(AZStd::this_thread::get_id().m_id)
            {
                EBUS_ASSERT(context, "Internal error: context deleted while execution still in progress.");
                m_context = context;

                this->m_prev = m_context->s_callstack->m_prev;

                // We don't use the AZ_Assert macro here because it places the assert call (unlikely) before the
                // actual work to do (likely) which results in inlining shenanigans and icache misses.
                if (!this->m_prev || static_cast<CallstackEntry*>(this->m_prev)->m_threadId == m_threadId)
                {
                    m_context->s_callstack->m_prev = this;

                    m_context->m_dispatches++;
                }
                else
                {
                    AZ::Debug::Trace::Instance().Assert(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE,
                        "Bus %s has multiple threads in its callstack records. Configure MutexType on the bus, or don't send to it from multiple threads", BusType::GetName());
                }
            }

            CallstackEntry(CallstackEntry&&) = default;

            CallstackEntry(const CallstackEntry&) = delete;
            CallstackEntry& operator=(const CallstackEntry&) = delete;
            CallstackEntry& operator=(CallstackEntry&&) = delete;

            ~CallstackEntry() override
            {
                m_context->m_dispatches--;

                m_context->s_callstack->m_prev = this->m_prev;
            }

            BusContextPtr m_context = nullptr;
            AZStd::native_thread_id_type m_threadId;
        };

        // One of these will be allocated per thread. It acts as the bottom of any callstack during dispatch within
        // that thread. It has to be stored in the context so that it is shared across DLLs. We accelerate this by
        // caching the root into a thread_local pointer (Context::s_callstack) on first access. Since global bus contexts
        // never die, the TLS pointer does not need to be lifetime managed.
        template <typename Interface, typename Traits>
        struct CallstackEntryRoot
            : public CallstackEntryBase<Interface, Traits>
        {
            using BusType = EBus<Interface, Traits>;

            CallstackEntryRoot()
                : CallstackEntryBase<Interface, Traits>(nullptr)
            {}

            void OnRemoveHandler(Interface*) override { AZ_Assert(false, "Callstack root should never attempt to handle the removal of a bus handler"); }
            void OnPostRemoveHandler() override { AZ_Assert(false, "Callstack root should never attempt to handle the removal of a bus handler"); }
            void SetRouterProcessingState(typename BusType::RouterProcessingState) override { AZ_Assert(false, "Callstack root should never attempt to alter router processing state"); }
            bool IsRoutingQueuedEvent() const override { return false; }
            bool IsRoutingReverseEvent() const override { return false; }
        };

        template <class C, bool UseTLS /*= false*/>
        struct EBusCallstackStorage
        {
            C* m_entry = nullptr;

            EBusCallstackStorage() = default;
            ~EBusCallstackStorage() = default;
            EBusCallstackStorage(const EBusCallstackStorage&) = delete;
            EBusCallstackStorage(EBusCallstackStorage&&) = delete;

            C* operator->() const
            {
                return m_entry;
            }

            C& operator*() const
            {
                return *m_entry;
            }

            C* operator=(C* entry)
            {
                m_entry = entry;
                return m_entry;
            }

            operator C*() const
            {
                return m_entry;
            }
        };

        template <class C>
        struct EBusCallstackStorage<C, true>
        {
            EBusCallstackStorage() = default;
            ~EBusCallstackStorage() = default;
            EBusCallstackStorage(const EBusCallstackStorage&) = delete;
            EBusCallstackStorage(EBusCallstackStorage&&) = delete;

            C* operator->() const
            {
                return GetEntry();
            }

            C& operator*() const
            {
                return *GetEntry();
            }

            C* operator=(C* entry)
            {
                GetEntry() = entry;
                return GetEntry();
            }

            operator C*() const
            {
                return GetEntry();
            }

        private:
            C*& GetEntry() const;
        };

        // This functino needs to be defined outside of the class definition such that it can get explicitly instantiated correctly. This is
        // important when using an EBus across module boundaries, as otherwise a callstack cannot traverse the module boundary.
        template <class C>
        C*& EBusCallstackStorage<C, true>::GetEntry() const
        {
            static AZ_THREAD_LOCAL C* s_entry = nullptr;
            return s_entry;
        }
    }
}
