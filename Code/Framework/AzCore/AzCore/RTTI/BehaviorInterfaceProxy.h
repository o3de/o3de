/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/typetraits/function_traits.h>

namespace AZ
{
    /**
     * Utility class for reflecting an AZ::Interface through the BehaviorContext
     *
     * Example:
     *
     * class MyInterface
     * {
     * public:
     *     AZ_RTTI(MyInterface, "{BADDF000D-CDCD-CDCD-CDCD-BAAAADF0000D}");
     *     virtual ~MyInterface() = default;
     *
     *     virtual AZStd::string Foo() = 0;
     *     virtual void Bar(float x, float y) = 0;
     * };
     *
     * class MySystemProxy
     *     : public BehaviorInterfaceProxy<MyInterface>
     * {
     * public:
     *     AZ_RTTI(MySystemProxy, "{CDCDCDCD-BAAD-BADD-F00D-CDCDCDCDCDCD}", BehaviorInterfaceProxy<MyInterface>);
     * };
     *
     * void Reflect(AZ::ReflectContext* context)
     * {
     *     if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
     *     {
     *         behaviorContext->ConstantProperty("g_MySystem", MySystemProxy::GetInstance)
     *             ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
     *             ->Attribute(AZ::Script::Attributes::Module, "MyModule");
     *
     *         behaviorContext->Class<MySystemProxy>("MySystemInterface")
     *             ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
     *             ->Attribute(AZ::Script::Attributes::Module, "MyModule")
     *
     *             ->Method("Foo", MySystemProxy::WrapMethod<&MyInterface::Foo>())
     *             ->Method("Bar", MySystemProxy::WrapMethod<&MyInterface::Bar>());
     *     }
     * }
     */
    template<typename T>
    class BehaviorInterfaceProxy
    {
    public:
        AZ_CLASS_ALLOCATOR(BehaviorInterfaceProxy, AZ::SystemAllocator, 0);
        AZ_RTTI(BehaviorInterfaceProxy<T>, "{E7CC8D27-4499-454E-A7DF-3F72FBECD30D}");

        //! Accessor for use with ConstantProperty
        static T* GetInstance()
        {
            T* interfacePtr = AZ::Interface<T>::Get();
            AZ_Warning("BehaviorInterfaceProxy", interfacePtr,
                "There is currently no global %s registered with an AZ Interface<T>",
                AzTypeInfo<T>::Name()
            );
            // Don't delete the global instance, it is not owned by the behavior context
            return interfacePtr;
        }

        //! Helper for attaching interface function via ClassBuilder::Method
        template<auto Method>
        static auto WrapMethod()
        {
            using FuncTraits = AZStd::function_traits<AZStd::remove_cvref_t<decltype(Method)>>;
            return FuncTraits::template expand_args<MethodWrapper>::template WrapMethod<Method>();
        }

        BehaviorInterfaceProxy() = default;
        virtual ~BehaviorInterfaceProxy() = default;

        //! Stores the instance which will use the provided shared_ptr deleter when the reference count hits zero
        BehaviorInterfaceProxy(AZStd::shared_ptr<T> sharedInstance)
            : m_instance(AZStd::move(sharedInstance))
        {
        }

        //! Stores the instance which will perform a no-op deleter when the reference count hits zero
        BehaviorInterfaceProxy(T* rawIntance)
            : m_instance(rawIntance, [](T*) {})
        {
        }

        //! Returns if the m_instance shared pointer is non-nullptr
        bool IsValid() const { return m_instance; }

    private:
        template <typename... Args>
        struct MethodWrapper
        {
            template<auto Method>
            static auto WrapMethod()
            {
                using return_type = AZStd::function_traits_get_result_t<AZStd::remove_cvref_t<decltype(Method)>>;
                return [](BehaviorInterfaceProxy<T>* proxy, Args... params) -> return_type
                {
                    if (proxy && proxy->IsValid())
                    {
                        return AZStd::invoke(Method, proxy->m_instance, AZStd::forward<Args>(params)...);
                    }
                    return return_type();
                };
            }
        };

        AZStd::shared_ptr<T> m_instance;
    };
} // namespace AZ
