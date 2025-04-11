/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/std/any.h>
#include <AzCore/std/parallel/atomic.h>
#include <ScriptCanvas/Data/Data.h>

#include <ScriptCanvas/Data/BehaviorContextObjectPtr.h>

namespace AZ
{
    template<typename ValueType, typename>
    struct AnyTypeInfoConcept;

    class ReflectContext;
    class BehaviorContextObjectSerializer;
}

namespace AZ::Serialize
{
    template<class T, bool U, bool A>
    struct InstanceFactory;
}

namespace ScriptCanvas
{
    class BehaviorContextObject final
    {
        friend struct AZStd::IntrusivePtrCountPolicy<BehaviorContextObject>;
        friend class Datum;
        friend class AZ::BehaviorContextObjectSerializer;

    public:
        AZ_TYPE_INFO(BehaviorContextObject, "{B735214D-5182-4536-B748-61EC83C1F007}");
        AZ_CLASS_ALLOCATOR(BehaviorContextObject, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflection);

        static BehaviorContextObjectPtr Create(const AZ::BehaviorClass& behaviorClass, const void* value = nullptr);

        template<typename t_Value>
        AZ_INLINE static BehaviorContextObjectPtr Create(const t_Value& value, const AZ::BehaviorClass& behaviorClass);

        static BehaviorContextObjectPtr CreateReference(const AZ::Uuid& typeID, void* reference = nullptr);

        template<typename t_Value>
        AZ_INLINE t_Value* Cast();

        template<typename t_Value>
        AZ_INLINE const t_Value* CastConst() const;

        BehaviorContextObjectPtr CloneObject(const AZ::BehaviorClass& behaviorClass);

        AZ_FORCE_INLINE const void* Get() const;

        AZ_FORCE_INLINE void* Mod();

        AZ_INLINE const AZStd::any& ToAny() const;

    private:
        using AnyTypeHandlerFunction = AZStd::any::type_info::HandleFnT;
        using AnyTypeInfo = AZStd::any::type_info;

        enum Flags
        {
            Const = 1 << 0,
            Owned = 1 << 1,
            Pointer = 1 << 2,
            Reference = 1 << 3,
        };

        template<typename... Args>
        static AZ::BehaviorObject InvokeConstructor(const AZ::BehaviorClass& behaviorClass, void* resultPtr, Args&&... args);

        AZ_INLINE static BehaviorContextObject* CreateCopy(const AZ::BehaviorClass& behaviorClass, const void* value);

        AZ_INLINE static BehaviorContextObject* CreateDefault(const AZ::BehaviorClass& behaviorClass);

        AZ_INLINE static BehaviorContextObject* CreateDefaultBuffer(const AZ::BehaviorClass& behaviorClass);

        AZ_INLINE static BehaviorContextObject* CreateDefaultHeap(const AZ::BehaviorClass& behaviorClass);

        static void Deserialize(BehaviorContextObject& target, const AZ::BehaviorClass& behaviorClass, AZStd::any& source);

        // use the SSO optimization on behavior class size ALIGNED with a placement new of behavior class create
        AZ_FORCE_INLINE static AnyTypeInfo GetAnyTypeInfoObject(const AZ::BehaviorClass& behaviorClass);

        AZ_FORCE_INLINE static AnyTypeInfo GetAnyTypeInfoReference(const AZ::Uuid& typeID);

        AZ_FORCE_INLINE static AnyTypeHandlerFunction GetHandlerObject(const AZ::BehaviorClass& behaviorClass, bool useHeap);

        AZ_FORCE_INLINE static AnyTypeHandlerFunction GetHandlerObjectBuffer(const AZ::BehaviorClass& behaviorClass);

        AZ_FORCE_INLINE static AnyTypeHandlerFunction GetHandlerObjectHeap(const AZ::BehaviorClass& behaviorClass);

        AZ_FORCE_INLINE static AnyTypeHandlerFunction GetHandlerReference();

        AZStd::atomic_int m_referenceCount{0};
        AZ::u32 m_flags{ 0 };
        AZStd::any m_object;

        // it is very important to track these from the moment they are created...
        friend struct AZ::Serialize::InstanceFactory<BehaviorContextObject, true, false>;
        friend struct AZ::AnyTypeInfoConcept<BehaviorContextObject, void>;

        //...so don't use the ctors, use the Create functions...
        //...the friend declarations are here for compatibility with the serialization system only
        AZ_FORCE_INLINE BehaviorContextObject() = default;

        BehaviorContextObject& operator=(const BehaviorContextObject&) = delete;

        BehaviorContextObject(const BehaviorContextObject&) = delete;

        // copy ctor
        AZ_FORCE_INLINE BehaviorContextObject(const void* source, const AnyTypeInfo& typeInfo, AZ::u32 flags);

        // reference or transfer ownership ctor
        AZ_FORCE_INLINE BehaviorContextObject(void* source, const AnyTypeInfo& typeInfo, AZ::u32 flags);

        AZ_FORCE_INLINE void Clear();

        AZ_FORCE_INLINE bool IsOwned() const;

        AZ_FORCE_INLINE void add_ref();

        void release();
    };

    AZ_FORCE_INLINE BehaviorContextObject::BehaviorContextObject(const void* value, const AnyTypeInfo& typeInfo, AZ::u32 flags)
        : m_referenceCount(0)
        , m_flags(flags)
        , m_object(value, typeInfo)
    {}

    AZ_FORCE_INLINE BehaviorContextObject::BehaviorContextObject(void* value, const AnyTypeInfo& typeInfo, AZ::u32 flags)
        : m_referenceCount(0)
        , m_flags(flags)
        , m_object(AZStd::s_transfer_ownership, value, typeInfo)
    {
        AZ_Assert((flags & Owned) || AZStd::any_cast<void>(&m_object) == value, "Failed to store the reference in the any class");
    }

    template<typename t_Value>
    AZ_INLINE t_Value* BehaviorContextObject::Cast()
    {
        return AZStd::any_cast<t_Value>(&m_object);
    }

    template<typename t_Value>
    AZ_INLINE const t_Value* BehaviorContextObject::CastConst() const
    {
        return AZStd::any_cast<const t_Value>(&m_object);
    }

    AZ_FORCE_INLINE void BehaviorContextObject::Clear()
    {
        m_object.clear();
        m_flags = 0;
    }

    template<AZStd::size_t Capacity>
    bool CompareSignature(AZ::BehaviorMethod* method, const AZStd::array<AZ::Uuid, Capacity>& typeIds)
    {
        bool signatureMatch = method->GetNumArguments() == typeIds.size();
        for (size_t index = 0; index < typeIds.size() && signatureMatch; ++index)
        {
            signatureMatch = method->GetArgument(index)->m_typeId == typeIds[index];
        }

        return signatureMatch;
    }

    template<size_t startIndex, typename ArrayType, typename... Args, size_t... Indices>
    void UnpackParameterAtIndex(ArrayType& outArray, AZStd::index_sequence<Indices...>, Args&&... args)
    {
        using dummy = int[];
        (void)dummy{0, ((outArray[startIndex + Indices] = AZStd::forward<Args>(args)), 0)... };
    }

    template<typename... Args>
    AZ::BehaviorObject BehaviorContextObject::InvokeConstructor(const AZ::BehaviorClass& behaviorClass, void* resultPtr, Args&&... args)
    {
        // The constructor result would be stored in the first argument
        AZStd::array<AZ::Uuid, 1 + sizeof...(Args)> typeIds{ {behaviorClass.m_typeId} };
        UnpackParameterAtIndex<1>(typeIds, AZStd::make_index_sequence<sizeof...(Args)>(), AZ::AzTypeInfo<Args>::Uuid()...);
        AZ::BehaviorMethod* invokableMethod{};

        AZ::Attribute* genericConstructorAttr = FindAttribute(AZ::Script::Attributes::GenericConstructorOverride, behaviorClass.m_attributes);
        if (genericConstructorAttr)
        {
            if (auto genericConstructorMethod = reinterpret_cast<AZ::BehaviorMethod*>(genericConstructorAttr->GetContextData()))
            {
                if (CompareSignature(genericConstructorMethod, typeIds))
                {
                    invokableMethod = genericConstructorMethod;
                }
            }
        }


        // Check all constructors and see if they match all the parameters
        if (!invokableMethod)
        {
            for (AZ::BehaviorMethod* method : behaviorClass.m_constructors)
            {
                if (CompareSignature(method, typeIds))
                {
                    invokableMethod = method;
                    break;
                }
            }
        }

        AZ::BehaviorObject resultObj{ resultPtr, behaviorClass.m_typeId };
        if (invokableMethod)
        {
            if (!resultObj.IsValid())
            {
                resultObj.m_address = behaviorClass.Allocate();
            }
            AZStd::array<AZ::BehaviorArgument, 1 + sizeof...(Args)> params{ {&resultObj} };
            UnpackParameterAtIndex<1>(params, AZStd::make_index_sequence<sizeof...(Args)>(), AZStd::forward<Args>(args)...);
            invokableMethod->Call(params.data(), static_cast<AZ::u32>(params.size()));
            return resultObj;
        }
        else if (behaviorClass.m_defaultConstructor)
        {
            // Otherwise use the default constructor
            if (!resultObj.IsValid())
            {
                resultObj.m_address = behaviorClass.Allocate();
            }
            behaviorClass.m_defaultConstructor(resultObj.m_address, nullptr);
        }

        return resultObj;
    }

    template<typename t_Value>
    AZ_INLINE BehaviorContextObjectPtr BehaviorContextObject::Create(const t_Value& value, const AZ::BehaviorClass& behaviorClass)
    {
        AZ_Assert(azrtti_typeid<t_Value>() == behaviorClass.m_typeId, "bad call to Create, mismatch with azrttti on value and behavior class");
        return Create(behaviorClass, reinterpret_cast<const void*>(&value));
    }

    AZ_INLINE BehaviorContextObject* BehaviorContextObject::CreateCopy(const AZ::BehaviorClass& behaviorClass, const void* value)
    {
        AZ_Assert(value, "invalid copy source object");
        return aznew BehaviorContextObject(value, GetAnyTypeInfoObject(behaviorClass), Owned);
    }

    AZ_INLINE BehaviorContextObject* BehaviorContextObject::CreateDefault(const AZ::BehaviorClass& behaviorClass)
    {
        const bool useHeap = AZStd::GetMax(behaviorClass.m_size, behaviorClass.m_alignment) > AZStd::Internal::ANY_SBO_BUF_SIZE;
        return useHeap ? CreateDefaultHeap(behaviorClass) : CreateDefaultBuffer(behaviorClass);
    }

    AZ_INLINE BehaviorContextObject* BehaviorContextObject::CreateDefaultBuffer(const AZ::BehaviorClass& behaviorClass)
    {
        alignas(32) char buffer[AZStd::Internal::ANY_SBO_BUF_SIZE];
        AZ::BehaviorObject object = InvokeConstructor(behaviorClass, AZStd::addressof(buffer));
        auto bco = aznew BehaviorContextObject(object.m_address, GetAnyTypeInfoObject(behaviorClass), Owned);
        behaviorClass.m_destructor(object.m_address, behaviorClass.m_userData);
        return bco;
    }

    AZ_INLINE BehaviorContextObject* BehaviorContextObject::CreateDefaultHeap(const AZ::BehaviorClass& behaviorClass)
    {
        AZ::BehaviorObject object = InvokeConstructor(behaviorClass, nullptr);
        auto bco = aznew BehaviorContextObject(object.m_address, GetAnyTypeInfoObject(behaviorClass), Owned);
        return bco;
    }

    AZ_FORCE_INLINE const void* BehaviorContextObject::Get() const
    {
        return const_cast<BehaviorContextObject*>(this)->Mod();
    }

    AZ_FORCE_INLINE BehaviorContextObject::AnyTypeInfo BehaviorContextObject::GetAnyTypeInfoObject(const AZ::BehaviorClass& behaviorClass)
    {
        const bool useHeap = AZStd::GetMax(behaviorClass.m_size, behaviorClass.m_alignment) > AZStd::Internal::ANY_SBO_BUF_SIZE;

        AZStd::any::type_info typeInfo;
        typeInfo.m_id = behaviorClass.m_typeId;
        typeInfo.m_useHeap = useHeap;
        typeInfo.m_handler = GetHandlerObject(behaviorClass, useHeap);
        return typeInfo;
    }

    AZ_FORCE_INLINE BehaviorContextObject::AnyTypeInfo BehaviorContextObject::GetAnyTypeInfoReference(const AZ::Uuid& typeID)
    {
        AZStd::any::type_info typeInfo;
        typeInfo.m_id = typeID;
        typeInfo.m_useHeap = true; // always true for references, regardless of size
        typeInfo.m_handler = GetHandlerReference();
        return typeInfo;
    }

    AZ_FORCE_INLINE BehaviorContextObject::AnyTypeHandlerFunction BehaviorContextObject::GetHandlerObject(const AZ::BehaviorClass& behaviorClass, bool useHeap)
    {
        return useHeap ? GetHandlerObjectHeap(behaviorClass) : GetHandlerObjectBuffer(behaviorClass);
    }

    AZ_FORCE_INLINE BehaviorContextObject::AnyTypeHandlerFunction BehaviorContextObject::GetHandlerObjectBuffer(const AZ::BehaviorClass& behaviorClass)
    {
        return [&behaviorClass](AZStd::any::Action action, AZStd::any* dest, const AZStd::any* source)
        {
            switch (action)
            {
            case AZStd::any::Action::Reserve:
            {
                break;
            }
            case AZStd::any::Action::Copy:
            {
                AZ_Assert(dest->get_type_info().m_id == behaviorClass.m_typeId, "invalid any destination");
                behaviorClass.m_cloner(AZStd::any_cast<void>(dest), AZStd::any_cast<void>(source), behaviorClass.m_userData);
                break;
            }
            case AZStd::any::Action::Move:
            {
                AZ_Assert(dest->get_type_info().m_id == behaviorClass.m_typeId, "invalid any destination");
                behaviorClass.m_mover(AZStd::any_cast<void>(dest), AZStd::any_cast<void>(const_cast<AZStd::any*>(source)), behaviorClass.m_userData);
                break;
            }
            case AZStd::any::Action::Destroy:
            {
                AZ_Assert(dest->get_type_info().m_id == behaviorClass.m_typeId, "invalid any destination");
                behaviorClass.m_destructor(AZStd::any_cast<void>(dest), behaviorClass.m_userData);
                break;
            }
            }
        };
    }

    AZ_FORCE_INLINE BehaviorContextObject::AnyTypeHandlerFunction BehaviorContextObject::GetHandlerObjectHeap(const AZ::BehaviorClass& behaviorClass)
    {
        // If it's a value type, copy/move it around, technically, this will only happen one time on construction
        // if we add on extension to the any class, we could just assert on all these operations (except copy)
        return [&behaviorClass](AZStd::any::Action action, AZStd::any* dest, const AZStd::any* source)
        {
            switch (action)
            {
            case AZStd::any::Action::Reserve:
            {
                AZ_Assert(dest->get_type_info().m_id == behaviorClass.m_typeId, "invalid any destination");
                AZ_Assert(dest->get_type_info().m_useHeap == true, "invalid heap target");
                *reinterpret_cast<void**>(dest) = behaviorClass.Allocate();
                break;
            }
            case AZStd::any::Action::Copy:
            {
                AZ_Assert(dest->get_type_info().m_id == behaviorClass.m_typeId, "invalid any destination");
                AZ_Assert(dest->get_type_info().m_useHeap == true, "invalid heap target");
                behaviorClass.m_cloner(AZStd::any_cast<void>(dest), AZStd::any_cast<void>(source), behaviorClass.m_userData);
                break;
            }
            case AZStd::any::Action::Move:
            {
                AZ_Assert(dest->get_type_info().m_id == behaviorClass.m_typeId, "invalid any destination");
                AZ_Assert(dest->get_type_info().m_useHeap == true, "invalid heap target");
                behaviorClass.m_mover(AZStd::any_cast<void>(dest), AZStd::any_cast<void>(const_cast<AZStd::any*>(source)), behaviorClass.m_userData);
                break;
            }
            case AZStd::any::Action::Destroy:
            {
                AZ_Assert(dest->get_type_info().m_id == behaviorClass.m_typeId, "invalid any destination");
                AZ_Assert(dest->get_type_info().m_useHeap == true, "invalid heap target");
                behaviorClass.Destroy(AZ::BehaviorObject(AZStd::any_cast<void>(dest), behaviorClass.m_typeId));
                break;
            }
            }
        };
    }

    AZ_FORCE_INLINE BehaviorContextObject::AnyTypeHandlerFunction BehaviorContextObject::GetHandlerReference()
    {
        // reference type, just move the pointer around
        return [](AZStd::any::Action action, AZStd::any* dest, const AZStd::any* source)
        {
            switch (action)
            {
            case AZStd::any::Action::Reserve:
            {
                // No-op
                break;
            }
            case AZStd::any::Action::Copy:
            case AZStd::any::Action::Move:
            {
                *reinterpret_cast<void**>(dest) = AZStd::any_cast<void>(const_cast<AZStd::any*>(source));
                break;
            }
            case AZStd::any::Action::Destroy:
            {
                *reinterpret_cast<void**>(dest) = nullptr;
                break;
            }
            }
        };
    }

    AZ_FORCE_INLINE bool BehaviorContextObject::IsOwned() const
    {
        return (m_flags & Flags::Owned) != 0;
    }

    AZ_FORCE_INLINE void* BehaviorContextObject::Mod()
    {
        return AZStd::any_cast<void>(&m_object);
    }

    AZ_INLINE const AZStd::any& BehaviorContextObject::ToAny() const
    {
        return m_object;
    }

    AZ_FORCE_INLINE void BehaviorContextObject::add_ref()
    {
        ++m_referenceCount;
    }

}
