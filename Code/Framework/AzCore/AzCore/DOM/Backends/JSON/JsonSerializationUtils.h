/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomBackend.h>
#include <AzCore/DOM/DomVisitor.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/document.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::Dom::Json
{
    //! Specifies how JSON should be formatted when serialized.
    enum class OutputFormatting
    {
        MinifiedJson, //!< Formats JSON in compact minified form, focusing on minimizing output size.
        PrettyPrintedJson, //!< Formats JSON in a pretty printed form, focusing on legibility to readers.
    };

    //! Specifies parsing behavior when deserializing JSON.
    enum class ParseFlags : int
    {
        Null = 0,
        StopWhenDone = rapidjson::kParseStopWhenDoneFlag,
        FullFloatingPointPrecision = rapidjson::kParseFullPrecisionFlag,
        ParseComments = rapidjson::kParseCommentsFlag,
        ParseNumbersAsStrings = rapidjson::kParseNumbersAsStringsFlag,
        ParseTrailingCommas = rapidjson::kParseTrailingCommasFlag,
        ParseNanAndInfinity = rapidjson::kParseNanAndInfFlag,
        ParseEscapedApostrophies = rapidjson::kParseEscapedApostropheFlag,
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(ParseFlags);

    //! Visitor that feeds into a rapidjson::Value
    class RapidJsonValueWriter final : public Visitor
    {
    public:
        RapidJsonValueWriter(rapidjson::Value& outputValue, rapidjson::Value::AllocatorType& allocator);

        VisitorFlags GetVisitorFlags() const override;
        Result Null() override;
        Result Bool(bool value) override;
        Result Int64(AZ::s64 value) override;
        Result Uint64(AZ::u64 value) override;
        Result Double(double value) override;

        Result String(AZStd::string_view value, Lifetime lifetime) override;
        Result StartObject() override;
        Result EndObject(AZ::u64 attributeCount) override;
        Result Key(AZ::Name key) override;
        Result RawKey(AZStd::string_view key, Lifetime lifetime) override;
        Result StartArray() override;
        Result EndArray(AZ::u64 elementCount) override;

    private:
        Result FinishWrite();
        rapidjson::Value& CurrentValue();

        struct ValueInfo
        {
            ValueInfo(bool isObject, rapidjson::Value& container);

            rapidjson::Value m_key;
            rapidjson::Value m_value;
            rapidjson::Value& m_container;
            AZ::u64 m_entryCount = 0;
            bool m_isObject;
        };

        rapidjson::Value& m_result;
        rapidjson::Value::AllocatorType& m_allocator;
        AZStd::deque<ValueInfo> m_entryStack;
    };

    //! Handler for a rapidjson::Reader that translates reads into an AZ::Dom::Visitor
    struct RapidJsonReadHandler
    {
    public:
        RapidJsonReadHandler(Visitor* visitor, Lifetime stringLifetime);

        bool Null();
        bool Bool(bool b);
        bool Int(int i);
        bool Uint(unsigned i);
        bool Int64(int64_t i);
        bool Uint64(uint64_t i);
        bool Double(double d);
        bool RawNumber(const char* str, rapidjson::SizeType length, bool copy);
        bool String(const char* str, rapidjson::SizeType length, bool copy);
        bool StartObject();
        bool Key(const char* str, rapidjson::SizeType length, bool copy);
        bool EndObject(rapidjson::SizeType memberCount);
        bool StartArray();
        bool EndArray(rapidjson::SizeType elementCount);
        Visitor::Result&& TakeOutcome();

    private:
        bool CheckResult(Visitor::Result result);

        Visitor::Result m_outcome;
        Visitor* m_visitor;
        Lifetime m_stringLifetime;
    };

    //! rapidjson stream wrapper for AZStd::string suitable for in-situ parsing
    //! Faster than rapidjson::MemoryStream for reading from AZStd::string / AZStd::string_view (because it requires a null terminator)
    //! \note This needs to be inlined for performance reasons.
    struct NullDelimitedStringStream
    {
        using Ch = char; //<! Denotes the string character storage type for rapidjson

        AZ_FORCE_INLINE NullDelimitedStringStream(char* buffer)
        {
            m_cursor = buffer;
            m_begin = m_cursor;
        }

        AZ_FORCE_INLINE NullDelimitedStringStream(AZStd::string_view buffer)
        {
            // rapidjson won't actually call PutBegin or Put unless kParseInSituFlag is set, so this is safe
            m_cursor = const_cast<char*>(buffer.data());
            m_begin = m_cursor;
        }

        AZ_FORCE_INLINE char Peek() const
        {
            return *m_cursor;
        }

        AZ_FORCE_INLINE char Take()
        {
            return *m_cursor++;
        }

        AZ_FORCE_INLINE size_t Tell() const
        {
            return static_cast<size_t>(m_cursor - m_begin);
        }

        AZ_FORCE_INLINE char* PutBegin()
        {
            m_write = m_cursor;
            return m_cursor;
        }

        AZ_FORCE_INLINE void Put(char c)
        {
            (*m_write++) = c;
        }

        AZ_FORCE_INLINE void Flush()
        {
        }

        AZ_FORCE_INLINE size_t PutEnd(char* begin)
        {
            return m_write - begin;
        }

        AZ_FORCE_INLINE const char* Peek4() const
        {
            AZ_Assert(false, "Not implemented, encoding is hard-coded to UTF-8");
            return m_cursor;
        }

        char* m_cursor; //!< Current read position.
        char* m_write; //!< Current write position.
        const char* m_begin; //!< Head of string.
    };

    //! Creates a Visitor that will write serialized JSON to the specified stream.
    //! \param stream The stream the visitor will write to.
    //! \param format The format to write in.
    //! \return A Visitor that will write to stream when visited.
    AZStd::unique_ptr<Visitor> CreateJsonStreamWriter(
        AZ::IO::GenericStream& stream, OutputFormatting format = OutputFormatting::PrettyPrintedJson);
    //! Reads serialized JSON from a string and applies it to a visitor.
    //! \param buffer The UTF-8 serialized JSON to read.
    //! \param lifetime Specifies the lifetime of the specified buffer. If the string specified by buffer might be deallocated,
    //! ensure Lifetime::Temporary is specified.
    //! \param visitor The visitor to visit with the JSON buffer's contents.
    //! \param parseFlags (template) Settings for adjusting parser behavior.
    //! \return The aggregate result specifying whether the visitor operations were successful.
    template<ParseFlags parseFlags = ParseFlags::ParseComments>
    Visitor::Result VisitSerializedJson(AZStd::string_view buffer, Lifetime lifetime, Visitor& visitor);

    //! Reads serialized JSON from a string in-place and applies it to a visitor.
    //! \param buffer The UTF-8 serialized JSON to read. This buffer will be modified as part of the deserialization process to
    //! apply null terminators.
    //! \param visitor The visitor to visit with the JSON buffer's contents. The strings provided to the visitor will only
    //! be valid for the lifetime of buffer.
    //! \param parseFlags (template) Settings for adjusting parser behavior.
    //! \return The aggregate result specifying whether the visitor operations were successful.
    template<ParseFlags parseFlags = ParseFlags::ParseComments>
    Visitor::Result VisitSerializedJsonInPlace(char* buffer, Visitor& visitor);

    //! Takes a visitor specified by a callback and produces a rapidjson::Document.
    //! \param writeCallback A callback specifying a visitor to accept to build the resulting document.
    //! \return An outcome with either the rapidjson::Document or an error message.
    AZ::Outcome<rapidjson::Document, AZStd::string> WriteToRapidJsonDocument(Backend::WriteCallback writeCallback);
    //! Takes a visitor specified by a callback and reads them into a rapidjson::Value.
    //! \param value The value to read into, its contents will be overridden.
    //! \param allocator The allocator to use when performing rapidjson allocations (generally provded by the rapidjson::Document).
    //! \param writeCallback A callback specifying a visitor to accept to build the resulting document.
    //! \return An outcome with either the rapidjson::Document or an error message.
    Visitor::Result WriteToRapidJsonValue(
        rapidjson::Value& value, rapidjson::Value::AllocatorType& allocator, Backend::WriteCallback writeCallback);
    //! Accepts a visitor with the contents of a rapidjson::Value.
    //! \param value The rapidjson::Value to apply to visitor.
    //! \param visitor The visitor to receive the contents of value.
    //! \param lifetime The lifetime to specify for visiting strings. If the rapidjson::Value might be destroyed or changed
    //! before the visitor is finished using these values, Lifetime::Temporary should be specified.
    //! \return The aggregate result specifying whether the visitor operations were successful.
    Visitor::Result VisitRapidJsonValue(const rapidjson::Value& value, Visitor& visitor, Lifetime lifetime);

    template<ParseFlags parseFlags>
    Visitor::Result VisitSerializedJson(AZStd::string_view buffer, Lifetime lifetime, Visitor& visitor)
    {
        rapidjson::Reader reader;
        RapidJsonReadHandler handler(&visitor, lifetime);

        // If the string is null terminated, we can use the faster AzStringStream path - otherwise we fall back on rapidjson::MemoryStream
        if (buffer.data()[buffer.size()] == '\0')
        {
            NullDelimitedStringStream stream(buffer);
            reader.Parse<aznumeric_cast<unsigned>(parseFlags)>(stream, handler);
        }
        else
        {
            rapidjson::MemoryStream stream(buffer.data(), buffer.size());
            reader.Parse<aznumeric_cast<unsigned>(parseFlags)>(stream, handler);
        }
        return handler.TakeOutcome();
    }

    template<ParseFlags parseFlags>
    Visitor::Result VisitSerializedJsonInPlace(char* buffer, Visitor& visitor)
    {
        rapidjson::Reader reader;
        NullDelimitedStringStream stream(buffer);
        RapidJsonReadHandler handler(&visitor, Lifetime::Persistent);

        reader.Parse<aznumeric_cast<unsigned>(parseFlags) | rapidjson::kParseInsituFlag>(stream, handler);
        return handler.TakeOutcome();
    }
} // namespace AZ::Dom::Json
