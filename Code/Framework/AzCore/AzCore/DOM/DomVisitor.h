/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/any.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Name/Name.h>

namespace AZ::DOM
{
    //! Specifies the semantics under which a reference type is safe to store.
    enum class StorageSemantics
    {
        //! StoreByReference guarantees that the underlying value shall remain allocated
        //! and immutable for the duration of the consumer's lifetime.
        //! StoreByReference data can be stored as a reference without incurring a copy.
        StoreByReference,
        //! StoreByCopy only guarantees that the data will be safe to read at the time
        //! the reference type is passed, after which it may be deallocated or otherwise mutated.
        //! StoreByCopy data may not be stored as a reference and must be copied if storage is needed.
        StoreByCopy,
    };

    //! Error code specifying the reason a Visitor operation failed.
    enum class VisitorErrorCode
    {
        //! Set when a Visitor doesn't have an implementation for a given attribute type.
        //! A pure-JSON serializer might reject a Node attribute, for example, and serialization visitors
        //! can forbid non-serializable Opaque types.
        UnsupportedOperation,
        //! Set when a Visitor has received malformed or invalid data.
        //! Potential sources include mismatching Begin/End call pairs or invalid attribute or element counts
        //! being sent to End methods.
        InvalidData,
        //! The Visitor failed for some other reason not caused by invalid input.
        //! If returning a custom error with this code, it's preferrable to also provide supplemental info
        //! in the form of an explanatory string.
        InternalError
    };

    //
    // VisitorError class
    // 
    //! Details of the reason for failure within a VisitorInterface operation.
    class VisitorError final
    {
    public:
        explicit VisitorError(VisitorErrorCode code);
        VisitorError(VisitorErrorCode code, AZStd::string_view additionalInfo);

        //! Gets the error code associated with this error.
        VisitorErrorCode GetCode() const;
        //! Gets a supplemental error info string from the error.
        //! Returns an empty string if no additional information was provided to the error.
        const AZStd::string& GetAdditionalInfo() const;
        //! Provides a formatted, human-readable error description that can be used for logging purposes.
        AZStd::string FormatVisitorErrorMessage() const;

        //! Helper method, translates a VisitorErrorCode to a human readable string.
        static const char* CodeToString(VisitorErrorCode code);

    private:
        VisitorErrorCode m_code;
        AZStd::string m_additionalInfo;
    };

    //! A type alias for opaque DOM types that aren't meant to be serializable.
    //! /see VisitorInterface::OpaqueValue
    using OpaqueType = AZStd::any;

    //
    // VisitorInterface class
    // 
    //! An interface for performing operations on elements of a generic DOM (Document Object Model).
    //! A Document Object Model is defined here as a tree structure comprised of one of the following values:
    //! - Primitives: plain data types, including
    //!     - \ref Int64: 64 bit signed integer
    //!     - \ref Uint64: 64 bit unsigned integer
    //!     - \ref Bool: boolean value
    //!     - \ref Double: 64 bit double precision float
    //!     - \ref Null: sentinel "empty" type with no value representation
    //! - \ref String: UTF8 encoded string
    //! - \ref Object: an ordered container of key/value pairs where keys are  AZ::Names and values may be any DOM type
    //!   (including Object)
    //! - \ref Array: an ordered container of values, in which values are any DOM value type (including Array)
    //! - \ref Node: a container 
    //! - \ref OpaqueValue: An arbitrary value stored in an AZStd::any. This is a non-serializable representation of an
    //!   entry useful for in-memory options. This is intended to be used as an intermediate value over the course of DOM
    //!   transformation and as a proxy to pass through types of which the DOM has no knowledge to other systems.
    //! 
    //!   Opaque values are rejected by the default VisitorInterface implementation.
    //!
    //!   Care should be ensured that DOMs representing opaque types are only visited by consumers that understand them.
    class VisitorInterface
    {
    public:
        //! The result of a Visitor operation.
        //! A failure indicates a non-recoverable issue and signals that no further visit calls may be made in the
        //! current state.
        using Result = AZ::Outcome<void, VisitorError>;

        //! Operates on an empty null value.
        virtual Result Null();
        //! Operates on a bool value.
        virtual Result Bool(bool value);
        //! Operates on a signed, 64 bit integer value.
        virtual Result Int64(AZ::s64 value);
        //! Operates on an unsigned, 64 bit integer value.
        virtual Result Uint64(AZ::u64 value);
        //! Operates on a double precision, 64 bit floating point value.
        virtual Result Double(double value);
        //! Operates on a string value. As strings are a reference type.
        //! Storage semantics are provided to indicate where the value may be stored persistently or requires a copy.
        virtual Result String(AZStd::string_view value, StorageSemantics storageSemantics);
        //! Operates on an opaque value. As opaque values are a reference type, storage semantics are provided to
        //! indicate where the value may be stored persistently or requires a copy.
        //! The default implementation of OpaqueValue rejects the operation, as opaque values are meant for special
        //! cases with specific implementations, not generic usage.
        //! Storage semantics are provided to indicate where the value may be stored persistently or requires a copy.
        virtual Result OpaqueValue(const OpaqueType& value, StorageSemantics storageSemantics);

        //! Operates on an Object.
        //! Callers may make any number of Key calls, followed by calls representing a value (including a nested
        //! StartObject call) and then must call EndObject.
        virtual Result StartObject();
        //! Finishes operating on an Object.
        //! Callers must provide the number of attributes that were provided to the object, i.e. the number of key
        //! and value calls made within the direct context of this object (but not any nested objects / nodes).
        virtual Result EndObject(AZ::u64 attributeCount);

        //! Specifies a key for a key/value pair.
        //! Key must be called subsequent to a call to \ref StartObject or \ref StartNode and immediately followed by
        //! calls representing the key's associated value.
        virtual Result Key(AZ::Name);

        //! Operates on an Array.
        //! Callers may make any number of subsequent value calls to represent the elements of the array, and then must
        //! call EndArray.
        virtual Result StartArray();
        //! Finishes operating on an Array.
        //! Callers must provide the number of elements that were provided to the array, i.e. the number of value calls
        //! made within the direct context of this array (but not any nested arrays / nodes).
        virtual Result EndArray(AZ::u64 elementCount);

        //! Operates on a Node.
        //! Callers may make any number of Key calls followed by value calls or value calls not prefixed with a Key
        //! call, and then must call EndNode. See \ref StartObject and \ref StartArray as Node types combine the
        //! functionality of both structures into a named Node structure.
        virtual Result StartNode(AZ::Name name);
        //! Finishes operating on a Node.
        //! Callers must provide both the number of attributes the were provided and the number of elements that were
        //! provided to the node, attributes being values prefaced by a call to Key.
        virtual Result EndNode(AZ::u64 attributeCount, AZ::u64 elementCount);

    protected:
        VisitorInterface() = default;

        //! Helper method, constructs a failure \ref Result with the specified code.
        static Result VisitorFailure(VisitorErrorCode code);
        //! Helper method, constructs a failure \ref Result with the specified code and supplemental info.
        static Result VisitorFailure(VisitorErrorCode code, AZStd::string_view additionalInfo);
        //! Helper method, constructs a failure \ref Result with the specified error.
        static Result VisitorFailure(VisitorError error);
        //! Helper method, constructs a success \ref Result.
        static Result VisitorSuccess();
    };
} // namespace AZ::DOM
