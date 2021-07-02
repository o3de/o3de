/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GRIDMATE_MARSHALER_TYPES_H
#define GRIDMATE_MARSHALER_TYPES_H 1

#include <AzCore/base.h>
#include <AzCore/std/typetraits/is_same.h>

namespace GridMate
{
    class WriteBuffer;
    class ReadBuffer;

    /**
        Base marshaler interface. Specialize this to get a default marshaler for your type.
    */
    template<class Type, typename = void>
    class Marshaler
    {
    public:
        AZ_TYPE_INFO_LEGACY(Marshaler, "{D9546741-8ABD-43C8-9790-499FCB1BA1E6}", Type);
        /// Defines the size that is written to the wire. This is only valid for fixed size marshalers, marshalers for dynamic objects don't define it.
        static const AZStd::size_t MarshalSize = 0;

        void Marshal(WriteBuffer& wb, const Type& value) const
        {
            (void) wb;
            (void) value;
            static_assert(sizeof(Type) != sizeof(Type), "A specialized Marshaler implementation is required for all types");
        }
        void Unmarshal(Type& value, ReadBuffer& rb) const
        {
            (void) value;
            (void) rb;
            static_assert(sizeof(Type) != sizeof(Type), "A specialized Marshaler implementation is required for all types");
        }
    };

    /**
        Trait for checking if a Marshaler writes a fixed size to the stream
    */
    template<typename MarshalerType>
    struct IsFixedMarshaler
    {
        static_assert(sizeof(MarshalerType) == sizeof(MarshalerType), "Complete type for Marshaler is required");

        template<typename LocalType>
        struct FindMember
        {
            template<class InnerType>
            static AZStd::false_type Evaluate(...);
            template<class InnerType>
            static AZStd::true_type Evaluate(int,
                decltype(InnerType::MarshalSize) = InnerType::MarshalSize);

            enum
            {
                Value = AZStd::is_same<decltype(Evaluate<LocalType>(0)), AZStd::true_type>::value
            };
        };

        template<typename LocalType, bool>
        struct ExtractValue;

        template<typename LocalType>
        struct ExtractValue<LocalType, false>
        {
            static const AZStd::size_t MarshalSize = 0;
        };

        template<typename LocalType>
        struct ExtractValue<LocalType, true>
        {
            static const AZStd::size_t MarshalSize = LocalType::MarshalSize;
        };

        enum
        {
            Value = ExtractValue<MarshalerType, FindMember<MarshalerType>::Value>::MarshalSize != 0 ?
                AZStd::true_type::value : AZStd::false_type::value
        };
    };

    /**
        Trait for checking if a Marshaler is for a given type
    */
    template <typename Type, typename MarshalerType>
    class IsMarshalerForType
    {
    private:
        template <typename T, T>
        struct Evaluate;

        template <typename, typename>
        static AZStd::false_type EvaluateMarshal(...);
        template <typename C, typename CM>
        static AZStd::true_type EvaluateMarshal(Evaluate<void(CM::*)(WriteBuffer&, const C&), & CM::Marshal>*);
        template <typename C, typename CM>
        static AZStd::true_type EvaluateMarshal(Evaluate<void(CM::*)(WriteBuffer&, const C&) const, & CM::Marshal>*);
        template <typename C, typename CM>
        static AZStd::true_type EvaluateMarshal(Evaluate<void(CM::*)(WriteBuffer&, C), & CM::Marshal>*);
        template <typename C, typename CM>
        static AZStd::true_type EvaluateMarshal(Evaluate<void(CM::*)(WriteBuffer&, C) const, & CM::Marshal>*);

        template <typename, typename>
        static AZStd::false_type EvaluateUnmarshal(...);
        template <typename C, typename CM>
        static AZStd::true_type EvaluateUnmarshal(Evaluate<void(CM::*)(C&, ReadBuffer&), & CM::Unmarshal>*);
        template <typename C, typename CM>
        static AZStd::true_type EvaluateUnmarshal(Evaluate<void(CM::*)(C&, ReadBuffer&) const, & CM::Unmarshal>*);

    public:
        static_assert(sizeof(Type) == sizeof(Type), "Complete type for marshaled type is required");
        static_assert(sizeof(MarshalerType) == sizeof(MarshalerType), "Complete type for Marshaler is required");

        enum
        {
            Value = AZStd::is_same<decltype(EvaluateMarshal<Type, MarshalerType>(0)), AZStd::true_type>::value &&
                AZStd::is_same<decltype(EvaluateUnmarshal<Type, MarshalerType>(0)), AZStd::true_type>::value
        };
    };
}

#endif // GRIDMATE_MARSHALER_TYPES_H
