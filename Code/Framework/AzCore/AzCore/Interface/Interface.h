/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Environment.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/typetraits/is_constructible.h>
#include <AzCore/std/typetraits/is_assignable.h>
#include <AzCore/std/typetraits/has_virtual_destructor.h>


namespace AZ
{
    // Type aliases for return types for register and unregister commands
    using InterfaceErrorString = AZStd::fixed_string<256>;
    using InterfaceRegisterOutcome = AZ::Outcome<void, InterfaceErrorString>;

    /**
     * Interface class for accessing registered singletons across module boundaries. The raw interface
     * pointer is handed to you; the assumption is that the registered system will outlive cached
     * references. A system must register itself with the interface at initialization time by calling
     * @ref Register(); and unregister at shutdown by calling @ref Unregister(). The provided Registrar
     * class will do this for you automatically.  Registration will only succeed after the
     * AZ::Environment has been attached and is ready.
     *
     * Example Usage:
     * @code{.cpp}
     *      class ISystem
     *      {
     *      public:
     *          virtual ~ISystem();
     *
     *          virtual void DoSomething() = 0;
     *      };
     *
     *      class System
     *          : public AZ::Interface<ISystem>::Registrar
     *      {
     *      public:
     *          void DoSomething() override;
     *      };
     *
     *      // In client code.
     *
     *      // Check that the pointer is valid before use.
     *      if (ISystem* system = AZ::Interface<ISystem>::Get())
     *      {
     *          system->DoSomething();
     *      }
     * @endcode
     */
    template <typename T>
    class Interface final
    {
    public:
        using ErrorString = InterfaceErrorString;
        using RegisterOutcome = InterfaceRegisterOutcome;

        // Fix Atom usage and re-enable these static_asserts
        //static_assert(!AZStd::is_move_constructible_v<T>, "Interface type should not be movable, as this could leave a dangling interface");
        //static_assert(!AZStd::is_move_assignable_v<T>, "Interface type should not be movable, as this could leave a dangling interface");

        /**
         * Registers an instance pointer to the interface. Only one instance is allowed to register at a time.
         */
        static RegisterOutcome Register(T* type);

        /**
         * Unregisters from the interface. An unregister must occur in the same module as the original Register
         * call; the pointer must match the original call to Register.
         */
        static RegisterOutcome Unregister(T* type);

        /**
         * Returns the registered interface pointer, if it exists. This method is thread-safe in that you can
         * call @ref Get from multiple threads. Likewise, calls to Register / Unregister are guarded, but it
         * is highly recommended to restrict Register / Unregister to a dedicated serial code path.
         */
        static T* Get();

        /**
         * A helper utility RAII mixin class that will register / unregister within the constructor / destructor, respectively.
         *
         * Example Usage:
         * @code{.cpp}
         *      class System
         *          : public Interface<ISystem>::Registrar
         *      {
         *      };
         * @endcode
         */
        class Registrar
            : public T
        {
        public:
            Registrar();
            virtual ~Registrar();
        };

    private:
        static uint32_t GetVariableName();

        inline static EnvironmentVariable<T*>& GetInstance()
        {
            /**
             * Module-specific static environment variable. This will require a FindVariable<>() operation
             * when invoked for the first time in a new module. There is one of these per module, but they
             * all point to the same internal pointer.
             */
            static EnvironmentVariable<T*> s_instance;
            return s_instance;
        }

        static AZStd::shared_mutex s_mutex;
        static bool s_instanceAssigned;
    };

    template <typename T>
    AZStd::shared_mutex Interface<T>::s_mutex;

    template <typename T>
    bool Interface<T>::s_instanceAssigned = false;

    template <typename T>
    auto Interface<T>::Register(T* type) -> RegisterOutcome
    {
        if (!type)
        {
            return AZ::Failure(ErrorString::format("Interface '%s' registering a null pointer!", AzTypeInfo<T>::Name()));
        }

        if (T* foundType = Get())
        {
            return AZ::Failure(ErrorString::format("Interface '%s' already registered! [Found: %p]", AzTypeInfo<T>::Name(), foundType));
        }

        AZStd::unique_lock<AZStd::shared_mutex> lock(s_mutex);
        GetInstance() = Environment::CreateVariable<T*>(GetVariableName());
        GetInstance().Get() = type;
        s_instanceAssigned = true;
        return AZ::Success();
    }

    template <typename T>
    auto Interface<T>::Unregister(T* type) -> RegisterOutcome
    {
        if (!s_instanceAssigned)
        {
            return AZ::Failure(ErrorString::format("Interface '%s' not registered on this module!", AzTypeInfo<T>::Name()));
        }

        if (GetInstance() && GetInstance().Get() != type)
        {
            return AZ::Failure(ErrorString::format("Interface '%s' is not the same instance that was registered! [Expected '%p', Found '%p']",
                AzTypeInfo<T>::Name(), type, GetInstance().Get()));
        }

        // Assign the internal pointer to null and release the EnvironmentVariable reference.
        AZStd::unique_lock<AZStd::shared_mutex> lock(s_mutex);
        *GetInstance() = nullptr;
        GetInstance().Reset();
        s_instanceAssigned = false;
        return AZ::Success();
    }

    template <typename T>
    T* Interface<T>::Get()
    {
        // First attempt to use the module-static reference; take a read lock to check it.
        // This is the fast path which won't block.
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(s_mutex);
            if (s_instanceAssigned)
            {
                return GetInstance() ? GetInstance().Get() : nullptr;
            }
        }

        // If the instance doesn't exist (which means we could be in a different module),
        // take the full lock and request it.
        AZStd::unique_lock<AZStd::shared_mutex> lock(s_mutex);
        GetInstance() = Environment::FindVariable<T*>(GetVariableName());
        s_instanceAssigned = static_cast<bool>(GetInstance());
        return GetInstance() ? GetInstance().Get() : nullptr;
    }

    template <typename T>
    uint32_t Interface<T>::GetVariableName()
    {
        // Environment variable name is taken from the hash of the Uuid (truncated to 32 bits).
        return static_cast<uint32_t>(AzTypeInfo<T>::Uuid().GetHash());
    }

    template <typename T>
    Interface<T>::Registrar::Registrar()
    {
        Interface<T>::Register(this);
    }

    template <typename T>
    Interface<T>::Registrar::~Registrar()
    {
        Interface<T>::Unregister(this);
    }
}
