/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Module/Environment.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/OSAllocator.h> // Needed for Context allocation from the OS
#include <AzCore/RTTI/RTTI.h> // Context dynamic casting
#include <AzCore/std/containers/vector.h> // Environment context array
#include <AzCore/std/allocator_stateless.h>

namespace AZ
{
    template<class Context>
    struct EBusEnvironmentStoragePolicy;

    class EBusEnvironment;

    namespace Internal
    {
        typedef EBusEnvironment* (*EBusEnvironmentGetterType)();
        typedef void(*EBusEnvironmentSetterType)(EBusEnvironment*);

        /**
        * Base for EBus<T>::Context. We use it to support multiple EBusEnvironments (have collection of contexts and manage state).
        */
        class ContextBase
        {
            template<class Context>
            friend struct AZ::EBusEnvironmentStoragePolicy;
            friend class AZ::EBusEnvironment;

        public:
            ContextBase();
            ContextBase(EBusEnvironment*);

            virtual ~ContextBase() {}

        private:

            int m_ebusEnvironmentTLSIndex;
            EBusEnvironmentGetterType m_ebusEnvironmentGetter;
        };

        /**
        * Objects that will shared in the environment to access all thread local storage
        * current environment. We use that objects so we can register in the Environment 
        * and share the data into that first module. Keep in mind that if the module that 
        * created that class is unload the code will crash as we store the function pointer
        * to make we read the TSL from only one module, otherwise each module has it's own TSL blocks.
        * If this happens make sure you create this structure in the environment from the main executable
        * or a module that will be loaded before and unloaded before any EBuses are used.
        */
        struct EBusEnvironmentTLSAccessors
        {
            EBusEnvironmentTLSAccessors();

            static u32 GetId();

            EBusEnvironmentGetterType m_getter;
            EBusEnvironmentSetterType m_setter;

            static EBusEnvironment* GetTLSEnvironment();
            static void SetTLSEnvironment(EBusEnvironment* environment);

            AZStd::atomic_int m_numUniqueEBuses; ///< Used to provide unique index for the TLS table

            static AZ_THREAD_LOCAL EBusEnvironment* s_tlsCurrentEnvironment; ///< Pointer to the current environment for the current thread.
        };

        using EBusEnvironmentAllocator = AZStd::stateless_allocator;
    }

    /**
     * EBusEnvironment defines a separate EBus execution context. All EBuses will have unique instances in each environment, unless specially configured to not do that (not supported yet as it's tricky and will likely create contention and edge cases).
     * If you want EBusEnvinronments to communicate you should use a combination of listeners/routers and event queuing to implement that. This is by design as the whole purpose of having a separate environment is to cut any possible
     * sharing by default, think of it as a separate VM. When communication is needed it should be explicit and considering the requirements you had in first place when you created separate environment. Otherwise you 
     * will start having contention issues or even worse execute events (handlers) when the environment is not active.
     * EBusEnvironment is very similar to the way OpenGL contexts operate. You can manage their livecycle from any thread at anytime by calling EBusEnvironment::Create/Destroy. You can activate/deactivate an environment by calling
     * ActivateOnCurrentThread/DeactivateOnCurrentThread. Every EBusEnvironment can be activated to only one thread at a time.
     */
    class EBusEnvironment
    {
        template<class Context>
        friend struct EBusEnvironmentStoragePolicy;

    public:
        AZ_CLASS_ALLOCATOR(EBusEnvironment, AZ::OSAllocator);

        EBusEnvironment();

        ~EBusEnvironment();

        void ActivateOnCurrentThread();

        void DeactivateOnCurrentThread();

        /**
         * Create and Destroy are provided for consistency and writing code with explicitly 
         * set Create/Destroy location. You can also just use it as any other class:
         * EBusEnvironment* myEBusEnvironmnet = aznew EBusEnvironment;
         * and later delete myEBusEnvironment.
         */
        static EBusEnvironment* Create();
        
        static void Destroy(EBusEnvironment* environment);
        
        
        template<class Context>
        Context* GetBusContext(int tlsKey);

        /**
         * Finds an EBus \ref Internal::ContextBase in the current environment.
         * @returns Pointer to the context base class, or null if context for this doesn't exists.
         */ 
        AZ::Internal::ContextBase* FindContext(int tlsKey);
        
        /**
         * Inserts an existing context for a specific bus id. When using this function make sure the provided context is alive while this environment is operational.
         * @param busId is of the bus
         * @param context context to insert
         * @param isTakeOwnership true if want this environment to delete the context when destroyed, or false is this context is owned somewhere else
         * @return true if the insert was successful, false if there is already a context for this id and the insert did not happen.
         */
        bool InsertContext(int tlsKey, AZ::Internal::ContextBase* context, bool isTakeOwnership);

        /**
        * This is a helper function that will insert that global bus context Bus::GetContext and use it for this environment.
        * Currently this function requires that the Bus uses \ref EBusEnvironmentStoragePolicy, otherwise you will get a compile error.
        * Once we have a way to have a BusId independent of the storage policy, we can remove that restriction.
        * @return true if the insert/redirect was successful, false if there is already a context for this bus and the insert did not happen.
        */
        template<class Bus>
        bool RedirectToGlobalContext();

    protected:

        EnvironmentVariable<Internal::EBusEnvironmentTLSAccessors> m_tlsAccessor;

        EBusEnvironment*  m_stackPrevEnvironment;   ///< Pointer to the previous environment on the TLS stack. This is valid only when the context is in use. Each BusEnvironment can only be active on a single thread at a time.

        AZStd::vector<AZStd::pair<Internal::ContextBase*,bool>, OSStdAllocator> m_busContexts; ///< Array with all EBus<T>::Context for this environment
    };

    //////////////////////////////////////////////////////////////////////////
    template<class Context>
    inline Context* EBusEnvironment::GetBusContext(int tlsKey)
    {
        AZ::Internal::ContextBase* context = FindContext(tlsKey);
        if (!context)
        {
            context = new(AZ_OS_MALLOC(sizeof(Context), AZStd::alignment_of<Context>::value)) Context(this);
            InsertContext(tlsKey, context, true);
        }
        return static_cast<Context*>(context);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Bus>
    bool EBusEnvironment::RedirectToGlobalContext()
    {
        EBusEnvironment* currentEnvironment = m_tlsAccessor->m_getter(); // store current environment
        m_tlsAccessor->m_setter(nullptr); // set the environment to null to make sure that we can global context when we call Get()
        typename Bus::Context& globalContext = Bus::StoragePolicy::GetOrCreate();
        bool isInserted = InsertContext(globalContext.m_ebusEnvironmentTLSIndex, &globalContext, false);
        m_tlsAccessor->m_setter(currentEnvironment);
        return isInserted;
    }

    /**
    * A choice of AZ::EBusTraits::StoragePolicy that specifies
    * that EBus data is stored in the AZ::Environment and it also support multiple \ref EBusEnvironment
    * With this policy, a single EBus instance is shared across all
    * modules (DLLs) that attach to the AZ::Environment.
    * @tparam Context A class that contains EBus data.
    *
    * \note Using separate EBusEnvironment allows you to manage fully independent EBus communication environments,
    * most frequently this is use to remove/reduce contention when we choose to process in parallel unrelated 
    * systems.
    */
    template<class Context>
    struct EBusEnvironmentStoragePolicy
    {
        /**
        * Returns the EBus data if it's already created, else nullptr.
        * @return A pointer to the EBus data.
        */
        static Context* Get();

        /**
        * Returns the EBus data. Creates it if not already created.
        * @return A reference to the EBus data.
        */
        static Context& GetOrCreate();

        /**
        * Environment variable that contains a pointer to the EBus data.
        */
        static EnvironmentVariable<Context> s_defaultGlobalContext;

        // EBus traits should provide a valid unique name, so that handlers can 
        // connect to the EBus across modules.
        // This can fail on some compilers. If it fails, make sure that you give 
        // each bus a unique name.
        static u32 GetVariableId();
    };

    template<class Context>
    EnvironmentVariable<Context> EBusEnvironmentStoragePolicy<Context>::s_defaultGlobalContext;

    //////////////////////////////////////////////////////////////////////////
    template<class Context>
    Context* EBusEnvironmentStoragePolicy<Context>::Get()
    {
        if (!s_defaultGlobalContext)
        {
            s_defaultGlobalContext = Environment::FindVariable<Context>(GetVariableId());
        }

        if (s_defaultGlobalContext)
        {
            if (s_defaultGlobalContext.IsConstructed())
            {
                Context* globalContext = &s_defaultGlobalContext.Get();

                if (EBusEnvironment* tlsEnvironment = globalContext->m_ebusEnvironmentGetter())
                {
                    return tlsEnvironment->GetBusContext<Context>(globalContext->m_ebusEnvironmentTLSIndex);
                }
                return globalContext;
            }
        }

        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Context>
    Context& EBusEnvironmentStoragePolicy<Context>::GetOrCreate()
    {
        if (!s_defaultGlobalContext)
        {
            s_defaultGlobalContext = Environment::CreateVariable<Context>(GetVariableId());
        }

        Context& globalContext = *s_defaultGlobalContext;

        if (EBusEnvironment* tlsEnvironment = globalContext.m_ebusEnvironmentGetter())
        {
            return *tlsEnvironment->GetBusContext<Context>(globalContext.m_ebusEnvironmentTLSIndex);
        }

        return globalContext;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Context>
    u32 EBusEnvironmentStoragePolicy<Context>::GetVariableId()
    {
        static constexpr u32 NameCrc = Crc32(AZ_FUNCTION_SIGNATURE);
        return NameCrc;
    }
} // namespace AZ
