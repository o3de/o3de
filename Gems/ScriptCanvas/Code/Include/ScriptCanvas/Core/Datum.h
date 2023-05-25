/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/string/string_view.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/SerializationListener.h>
#include <ScriptCanvas/Data/BehaviorContextObject.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Data/DataTrait.h>

namespace AZ
{
    class ReflectContext;
    class DatumSerializer;
}

namespace ScriptCanvas
{
    struct BehaviorContextResultTag { explicit BehaviorContextResultTag() = default; };
    const BehaviorContextResultTag s_behaviorContextResultTag{};

    using ComparisonOutcome = AZ::Outcome<bool, AZStd::string>;

    /// A Datum is used to provide generic storage for all data types in ScriptCanvas, and provide a common interface to accessing, modifying, and displaying them 
    /// in the editor, regardless of their actual ScriptCanvas or BehaviorContext type.
    class Datum final
        : public SerializationListener
    {
        friend class AZ::DatumSerializer;

    public:
        AZ_RTTI(Datum, "{8B836FC0-98A8-4A81-8651-35C7CA125451}", SerializationListener);
        AZ_CLASS_ALLOCATOR(Datum, AZ::SystemAllocator);

        enum class eOriginality : int
        {
            Original,
            Copy
        };

        // calls a function and converts the result to a ScriptCanvas type, if necessary
        static AZ::Outcome<Datum, AZStd::string> CallBehaviorContextMethodResult(const AZ::BehaviorMethod* method, const AZ::BehaviorParameter* resultType, AZ::BehaviorArgument* params, unsigned int numExpectedArgs, const AZStd::string_view context);
        static AZ::Outcome<void, AZStd::string> CallBehaviorContextMethod(const AZ::BehaviorMethod* method, AZ::BehaviorArgument* params, unsigned int numExpectedArgs);

        static bool IsValidDatum(const Datum* datum);

        static void Reflect(AZ::ReflectContext* reflectContext);

        Datum();
        Datum(const Datum& object);
        Datum(Datum&& object);
        Datum(const Data::Type& type, eOriginality originality);
        Datum(const Data::Type& type, eOriginality originality, const void* source, const AZ::Uuid& sourceTypeID);
        Datum(const AZ::BehaviorParameter& parameterDesc, eOriginality originality, const void* source);
        Datum(BehaviorContextResultTag, const AZ::BehaviorParameter& resultType);
        Datum(const AZStd::string& behaviorClassName, eOriginality originality);
        Datum(const AZ::BehaviorArgument& value);

        void ReconfigureDatumTo(Datum&& object);
        void ReconfigureDatumTo(const Datum& object);

        void CopyDatumTypeAndValue(const Datum& object);
        void DeepCopyDatum(const Datum& object);

        const AZStd::any& ToAny() const;

        /// If t_Value is a ScriptCanvas value type, regardless of pointer/reference, this will create datum with a copy of that
        /// value. That is, Datum<AZ::Vector3>(source), Datum<AZ::Vector3&>(source), Datum<AZ::Vector3*>(&source), will all produce
        /// a copy of source. If t_Value is a ScriptCanvas reference type, passing in a pointer or reference will created a datum
        /// that REFERS to the source, and passing in a value will create a datum with a new, Script-owned, copy from the source.
        //
        // Also needs to bypass when it conflicts with the other constructors available.
        template<typename t_Value, typename = AZStd::enable_if_t<!AZStd::is_same<AZStd::decay_t<t_Value>, Datum>::value && !AZStd::is_same<AZStd::decay_t<t_Value>, AZ::BehaviorArgument>::value> >
        explicit Datum(t_Value&& value);

        AZ_INLINE bool Empty() const;

        //! use RARELY, this is dangerous. Use ONLY to read the value contained by this Datum, never to modify
        template<typename t_Value>
        const t_Value* GetAs() const;

        /// \note Directly modifying the data circumvents all of the runtime (and even the edit time) handling of script canvas data.
        /// This is dangerous in that one may not be properly handling values, like converting numeric types to the supported numeric type, 
        /// or modifying objects without notifying systems whose correctness depends on changes in those values.
        /// Use with extreme caution. This will made clearer and easier in a future API change.
        AZ_INLINE const void* GetAsDanger() const;

        const Data::Type& GetType() const { return m_type; }
        enum class TypeChange
        {
            Forced,
            Requested,
        };
        void SetType(const Data::Type& dataType, TypeChange typeChange = TypeChange::Requested);

        template<typename T>
        void SetAZType()
        {
            SetType(ScriptCanvas::Data::FromAZType(azrtti_typeid<T>()));
        }

        // checks this datum can be converted to the specified type, including checking for required storage
        AZ_INLINE bool IsConvertibleFrom(const AZ::Uuid& typeID) const;
        AZ_INLINE bool IsConvertibleFrom(const Data::Type& type) const;

        // checks if the type of this datum can be converted to the specified type, regardless of storage 
        AZ_INLINE bool IsConvertibleTo(const AZ::Uuid& typeID) const;
        AZ_INLINE bool IsConvertibleTo(const Data::Type& type) const;
        bool IsConvertibleTo(const AZ::BehaviorParameter& parameterDesc) const;

        bool IsDefaultValue() const;

        // returns true if this type IS_A t_Value type
        template<typename t_Value>
        bool IS_A() const;

        AZ_INLINE bool IS_A(const Data::Type& type) const;

        //! use RARELY, this is dangerous as it circumvents ScriptCanvas execution. Use to initialize values more simply in unit testing, or assist debugging.
        template<typename t_Value>
        t_Value* ModAs();

        AZ_INLINE void* ModAsDanger();

        // can cause de-allocation of original datum, to which other data can point!
        Datum& operator=(const Datum& other);
        Datum& operator=(Datum&& other);

        ComparisonOutcome operator==(const Datum& other) const;
        ComparisonOutcome operator!=(const Datum& other) const;

        ComparisonOutcome operator<(const Datum& other) const;
        ComparisonOutcome operator<=(const Datum& other) const;
        ComparisonOutcome operator>(const Datum& other) const;
        ComparisonOutcome operator>=(const Datum& other) const;

        // use RARELY, this is dangerous
        template<typename t_Value>
        AZ_INLINE bool Set(const t_Value& value);

        void SetToDefaultValueOfType();

        void SetNotificationsTarget(AZ::EntityId notificationId);

        // pushes this datum to the void* address in destination
        bool ToBehaviorContext(AZ::BehaviorArgument& destination) const;

        // returns BVP with void* to the datum address
        AZ::BehaviorArgument ToBehaviorContext(AZ::BehaviorClass*& behaviorClass);

        // creates an AZ::BehaviorArgument with a void* that points to this datum, depending on what the parameter needs
        // this is called when the AZ::BehaviorArgument needs this value as input to another function
        // so it is appropriate for the value output to be nullptr
        AZ::Outcome<AZ::BehaviorArgument, AZStd::string> ToBehaviorValueParameter(const AZ::BehaviorParameter& description) const;

        AZ_INLINE AZStd::string ToString() const;

        bool ToString(Data::StringType& result) const;

        void SetLabel(AZStd::string_view name);
        AZStd::string GetLabel() const;

        void SetVisibility(AZ::Crc32 visibility);
        AZ::Crc32 GetVisibility() const;

        // Remaps references to the SelfReference Entity Id to the Entity Id of the ScriptCanvas Graph component owner
        // This should be called at ScriptCanvas compile time when the runtime entity is available
        void ResolveSelfEntityReferences(const AZ::EntityId& graphOwnerId);

        // creates an AZ::BehaviorArgument with a void* that points to this datum, depending on what the parameter needs
        // this is called when the AZ::BehaviorArgument needs this value as output from another function
        // so it is NOT appropriate for the value output to be nullptr, if the description is for a pointer to an object
        // there needs to be valid memory to write that pointer
        AZ::Outcome<AZ::BehaviorArgument, AZStd::string> ToBehaviorValueParameterResult(const AZ::BehaviorParameter& description, const AZStd::string_view className, const AZStd::string_view methodName);

        // This is used as the destination for a Behavior Context function call; after the call the result must be converted.
        void ConvertBehaviorContextMethodResult(const AZ::BehaviorParameter& resultType);

    private:

        void CopyDatumStorage(const Datum& object);
        AZ::Crc32 GetDatumVisibility() const;

        template<typename t_Value, bool isReference>
        struct InitializerHelper
        {
            static void Help(const t_Value& value, Datum& datum)
            {
                const bool isValue = Data::Traits<t_Value>::s_isNative || !isReference;
                datum.Initialize(Data::FromAZType(Data::Traits<t_Value>::GetAZType()), isValue ? Datum::eOriginality::Original : Datum::eOriginality::Copy, reinterpret_cast<const void*>(&value), azrtti_typeid<t_Value>());
            }
        };

        template<typename t_Value, bool isReference>
        struct InitializerHelper<t_Value*, isReference>
        {
            static void Help(const t_Value* value, Datum& datum)
            {
                datum.Initialize(Data::FromAZType(Data::Traits<t_Value>::GetAZType()), Datum::eOriginality::Copy, reinterpret_cast<const void*>(value), azrtti_typeid<t_Value>());
            }
        };

        template<typename t_Value>
        struct GetAsHelper
        {
            static const t_Value* Help(Datum& datum)
            {
                static_assert(!AZStd::is_pointer<t_Value>::value, "no pointer types in the Datum::GetAsHelper<t_Value, false>");

                if (datum.m_storage.value.empty())
                {
                    // rare, but can be caused by removals or problems with reflection to BehaviorContext, so must be checked
                    return nullptr;
                }
                else if (datum.m_type.GetType() == Data::eType::BehaviorContextObject)
                {
                    return (*AZStd::any_cast<BehaviorContextObjectPtr>(&datum.m_storage.value))->CastConst<t_Value>();
                }
                else
                {
                    return AZStd::any_cast<const t_Value>(&datum.m_storage.value);
                }
            }
        };

        template<typename t_Value>
        struct GetAsHelper<t_Value*>
        {
            static const t_Value** Help(Datum& datum)
            {
                datum.m_pointer = const_cast<void*>(static_cast<const void*>(GetAsHelper<AZStd::decay_t<t_Value>>::Help(datum)));
                return (const t_Value**)(&datum.m_pointer);
            }
        };

#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
        class SerializeContextEventHandler : public AZ::SerializeContext::IEventHandler
        {
        public:
            /// Called after we are done writing to the instance pointed by classPtr.
            void OnWriteEnd(void* classPtr) override
            {
                Datum* datum = reinterpret_cast<Datum*>(classPtr);
                datum->OnWriteEnd();
            }
        };
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)

        friend class SerializeContextEventHandler;

        static ComparisonOutcome CallComparisonOperator(AZ::Script::Attributes::OperatorType operatorType, const AZ::BehaviorClass* behaviorClass, const Datum& lhs, const Datum& rhs);

        // is this storage for nodes that are overloaded, e.g. Log, which takes in any data type
        const bool m_isOverloadedStorage = false;

        bool m_isDefaultConstructed = false;

        // eOriginality records the graph source of the object
        eOriginality m_originality = eOriginality::Copy;
        // storage for the datum, regardless of ScriptCanvas::Data::Type
        RuntimeVariable m_storage;

        // This contains the editor label for m_storage.value.
        AZStd::string m_datumLabel;

        // This contains the editor visibility for m_storage.value.
        AZ::Crc32 m_visibility{ AZ::Edit::PropertyVisibility::ShowChildrenOnly };
        // storage for implicit conversions, when needed
        AZStd::any m_conversionStorage;
        // the least derived class this datum can accept
        const AZ::BehaviorClass* m_class = nullptr;
        // storage for pointer, if necessary
        mutable void* m_pointer = nullptr;
        // the ScriptCanvas type of the object
        Data::Type m_type;

        // The notificationId to send change notifications to.
        AZ::EntityId m_notificationId;

        // Destroys the datum, and the type information
        void Clear();

        bool FromBehaviorContext(const void* source);

        bool FromBehaviorContext(const void* source, const AZ::Uuid& typeID);

        bool FromBehaviorContextNumber(const void* source, const AZ::Uuid& typeID);

        bool FromBehaviorContextObject(const AZ::BehaviorClass* behaviorClass, const void* source);

        const void* GetValueAddress() const;

        bool Initialize(const Data::Type& type, eOriginality originality, const void* source, const AZ::Uuid& sourceTypeID);

        bool InitializeBehaviorContextParameter(const AZ::BehaviorParameter& parameterDesc, eOriginality originality, const void* source);

        bool InitializeAABB(const void* source);

        bool InitializeAssetId(const void* source);

        bool InitializeBehaviorContextObject(eOriginality originality, const void* source);

        bool InitializeBehaviorContextMethodResult(const AZ::BehaviorParameter& resultType);

        bool InitializeBool(const void* source);

        bool InitializeColor(const void* source);

        bool InitializeCRC(const void* source);

        bool InitializeEntityID(const void* source);

        bool InitializeNamedEntityID(const void* source);

        bool InitializeMatrix3x3(const void* source);

        bool InitializeMatrix4x4(const void* source);

        bool InitializeNumber(const void* source, const AZ::Uuid& sourceTypeID);

        bool InitializeOBB(const void* source);

        bool InitializePlane(const void* source);

        bool InitializeQuaternion(const void* source);

        bool InitializeString(const void* source, const AZ::Uuid& sourceTypeID);

        bool InitializeTransform(const void* source);

        AZ_INLINE bool InitializeOverloadedStorage(const Data::Type& type, eOriginality originality);

        bool InitializeVector2(const void* source, const AZ::Uuid& sourceTypeID);

        bool InitializeVector3(const void* source, const AZ::Uuid& sourceTypeID);

        bool InitializeVector4(const void* source, const AZ::Uuid& sourceTypeID);

        void* ModResultAddress();

        void* ModValueAddress() const;

        void OnDatumEdited();

        void OnDeserialize() override;
               
#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
        void OnWriteEnd();
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)

        AZ_INLINE bool SatisfiesTraits(AZ::u8 behaviorValueTraits) const;

        bool ToBehaviorContextNumber(void* target, const AZ::Uuid& typeID) const;

        AZ::BehaviorArgument ToBehaviorValueParameterNumber(const AZ::BehaviorParameter& description);

        AZ::Outcome<AZ::BehaviorArgument, AZStd::string> ToBehaviorValueParameterString(const AZ::BehaviorParameter& description);

        AZStd::string ToStringAABB(const Data::AABBType& source) const;

        AZStd::string ToStringColor(const Data::ColorType& source) const;

        AZStd::string ToStringCRC(const Data::CRCType& source) const;

        bool ToStringBehaviorClassObject(Data::StringType& result) const;

        AZStd::string ToStringMatrix3x3(const AZ::Matrix3x3& source) const;

        AZStd::string ToStringMatrix4x4(const AZ::Matrix4x4& source) const;

        AZStd::string ToStringOBB(const Data::OBBType& source) const;

        AZStd::string ToStringPlane(const Data::PlaneType& source) const;

        AZStd::string ToStringQuaternion(const Data::QuaternionType& source) const;

        AZStd::string ToStringTransform(const Data::TransformType& source) const;

        AZStd::string ToStringVector2(const AZ::Vector2& source) const;

        AZStd::string ToStringVector3(const AZ::Vector3& source) const;

        AZStd::string ToStringVector4(const AZ::Vector4& source) const;
    };

    template<typename t_Value, typename>
    Datum::Datum(t_Value&& value)
    {
        InitializerHelper<AZStd::remove_reference_t<t_Value>, AZStd::is_reference<t_Value>::value>::Help(value, *this);
    }

    bool Datum::Empty() const
    {
        return m_storage.value.empty() || GetValueAddress() == nullptr;
    }

    template<typename t_Value>
    const t_Value* Datum::GetAs() const
    {
        return GetAsHelper<t_Value>::Help(*const_cast<Datum*>(this));
    }

#define DATUM_GET_NUMBER_SPECIALIZE(NUMERIC_TYPE)\
        template<>\
        struct Datum::GetAsHelper<NUMERIC_TYPE>\
        {\
            AZ_FORCE_INLINE static const NUMERIC_TYPE* Help(Datum& datum)\
            {\
                static_assert(!AZStd::is_pointer<NUMERIC_TYPE>::value, "no pointer types in the Datum::GetAsHelper<" #NUMERIC_TYPE ">");\
                void* numberStorage(const_cast<void*>(reinterpret_cast<const void*>(&datum.m_conversionStorage)));\
                return datum.IS_A(Data::Type::Number()) && datum.ToBehaviorContextNumber(numberStorage, AZ::AzTypeInfo<NUMERIC_TYPE>::Uuid())\
                    ? reinterpret_cast<const NUMERIC_TYPE*>(numberStorage)\
                    : nullptr;\
            }\
        };

    DATUM_GET_NUMBER_SPECIALIZE(char);
    DATUM_GET_NUMBER_SPECIALIZE(short);
    DATUM_GET_NUMBER_SPECIALIZE(int);
    DATUM_GET_NUMBER_SPECIALIZE(long);
    DATUM_GET_NUMBER_SPECIALIZE(AZ::s8);
    DATUM_GET_NUMBER_SPECIALIZE(AZ::s64);
    DATUM_GET_NUMBER_SPECIALIZE(unsigned char);
    DATUM_GET_NUMBER_SPECIALIZE(unsigned int);
    DATUM_GET_NUMBER_SPECIALIZE(unsigned long);
    DATUM_GET_NUMBER_SPECIALIZE(unsigned short);
    DATUM_GET_NUMBER_SPECIALIZE(AZ::u64);
    DATUM_GET_NUMBER_SPECIALIZE(float);
    // only requred if the ScriptCanvas::NumberType changes from double, see set specialization below
    //DATUM_GET_NUMBER_SPECIALIZE(double); 

    const void* Datum::GetAsDanger() const
    {
        return GetValueAddress();
    }

    template<typename t_Value>
    bool Datum::IS_A() const
    {
        static_assert(!AZStd::is_pointer<t_Value>::value, "no pointer types in the Datum::Is, please");
        return m_type.IS_A(Data::FromAZType(azrtti_typeid<t_Value>()));
    }

    bool Datum::IS_A(const Data::Type& type) const
    {
        return Data::IS_A(m_type, type);
    }

    bool Datum::IsConvertibleFrom(const AZ::Uuid& typeID) const
    {
        return m_type.IsConvertibleFrom(typeID);
    }

    bool Datum::IsConvertibleFrom(const Data::Type& type) const
    {
        return m_type.IsConvertibleTo(type);
    }

    bool Datum::IsConvertibleTo(const AZ::Uuid& typeID) const
    {
        return m_type.IsConvertibleTo(typeID);
    }

    bool Datum::IsConvertibleTo(const Data::Type& type) const
    {
        return m_type.IsConvertibleTo(type);
    }

    bool Datum::InitializeOverloadedStorage(const Data::Type& type, eOriginality originality)
    {
        return m_isOverloadedStorage && type.IsValid() && (m_type.IS_EXACTLY_A(type) || Initialize(type, originality, nullptr, AZ::Uuid::CreateNull()));
    }

    template<typename t_Value>
    t_Value* Datum::ModAs()
    {
        return const_cast<t_Value*>(GetAs<t_Value>());
    }

    void* Datum::ModAsDanger()
    {
        return ModValueAddress();
    }

    bool Datum::SatisfiesTraits(AZ::u8 behaviorValueTraits) const
    {
        AZ_Assert(!(behaviorValueTraits & AZ::BehaviorParameter::TR_POINTER && behaviorValueTraits & AZ::BehaviorParameter::TR_REFERENCE), "invalid traits on behavior parameter");
        return GetValueAddress() || (!(behaviorValueTraits & AZ::BehaviorParameter::TR_THIS_PTR) && (behaviorValueTraits & AZ::BehaviorParameter::TR_POINTER));
    }

    template<typename t_Value>
    bool Datum::Set(const t_Value& value)
    {
        static_assert(!AZStd::is_pointer<t_Value>::value, "no pointer types in the Datum::Set, please");
        InitializeOverloadedStorage(Data::FromAZType(azrtti_typeid<t_Value>()), eOriginality::Copy);
        AZ_Error("Script Canvas", !IS_A(Data::Type::Number()) || azrtti_typeid<t_Value>() == azrtti_typeid<Data::NumberType>(), "Set on number types must be specialized!");

        if (IS_A<t_Value>())
        {
            if (Data::IsValueType(m_type))
            {
                m_storage.value = value;
                return true;
            }
            else
            {
                return FromBehaviorContext(static_cast<const void*>(&value));
            }
        }

        return false;
    }

#define DATUM_SET_NUMBER_SPECIALIZE(NUMERIC_TYPE)\
    template<>\
    AZ_INLINE bool Datum::Set(const NUMERIC_TYPE& value)\
    {\
        return FromBehaviorContextNumber(&value, azrtti_typeid<NUMERIC_TYPE>());\
    }

    DATUM_SET_NUMBER_SPECIALIZE(char);
    DATUM_SET_NUMBER_SPECIALIZE(short);
    DATUM_SET_NUMBER_SPECIALIZE(int);
    DATUM_SET_NUMBER_SPECIALIZE(long);
    DATUM_SET_NUMBER_SPECIALIZE(AZ::s8);
    DATUM_SET_NUMBER_SPECIALIZE(AZ::s64);
    DATUM_SET_NUMBER_SPECIALIZE(unsigned char);
    DATUM_SET_NUMBER_SPECIALIZE(unsigned int);
    DATUM_SET_NUMBER_SPECIALIZE(unsigned long);
    DATUM_SET_NUMBER_SPECIALIZE(unsigned short);
    DATUM_SET_NUMBER_SPECIALIZE(AZ::u64);
    DATUM_SET_NUMBER_SPECIALIZE(float);
    // only requried if the ScriptCanvas::NumberType changes from double, see get specialization above
    //DATUM_SET_NUMBER_SPECIALIZE(double);

    // vectors are the most convertible objects, so more get/set specialization is necessary
#define DATUM_SET_VECTOR_SPECIALIZE(VECTOR_TYPE)\
    template<>\
    AZ_INLINE bool Datum::Set(const VECTOR_TYPE& value)\
    {\
        if (FromBehaviorContext(&value, azrtti_typeid<VECTOR_TYPE>()))\
        {\
            return true;\
        }\
        return false;\
    }

    DATUM_SET_VECTOR_SPECIALIZE(AZ::Vector2);
    DATUM_SET_VECTOR_SPECIALIZE(AZ::Vector3);
    DATUM_SET_VECTOR_SPECIALIZE(AZ::Vector4);

    AZStd::string Datum::ToString() const
    {
        AZStd::string result;
        ToString(result);
        return result;
    }

}
