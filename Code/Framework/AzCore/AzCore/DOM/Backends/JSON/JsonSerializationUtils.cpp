/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/JSON/filewritestream.h>
#include <AzCore/JSON/memorystream.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/reader.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/optional.h>

namespace AZ::Dom::Json
{
    //
    // class RapidJsonValueWriter
    //
    RapidJsonValueWriter::RapidJsonValueWriter(rapidjson::Value& outputValue, rapidjson::Value::AllocatorType& allocator)
        : m_result(outputValue)
        , m_allocator(allocator)
    {
    }

    VisitorFlags RapidJsonValueWriter::GetVisitorFlags() const
    {
        return VisitorFlags::SupportsRawKeys | VisitorFlags::SupportsArrays | VisitorFlags::SupportsObjects;
    }

    Visitor::Result RapidJsonValueWriter::Null()
    {
        CurrentValue().SetNull();
        return FinishWrite();
    }

    Visitor::Result RapidJsonValueWriter::Bool(bool value)
    {
        CurrentValue().SetBool(value);
        return FinishWrite();
    }

    Visitor::Result RapidJsonValueWriter::Int64(AZ::s64 value)
    {
        CurrentValue().SetInt64(value);
        return FinishWrite();
    }

    Visitor::Result RapidJsonValueWriter::Uint64(AZ::u64 value)
    {
        CurrentValue().SetUint64(value);
        return FinishWrite();
    }

    Visitor::Result RapidJsonValueWriter::Double(double value)
    {
        CurrentValue().SetDouble(value);
        return FinishWrite();
    }

    Visitor::Result RapidJsonValueWriter::String(AZStd::string_view value, Lifetime lifetime)
    {
        if (lifetime == Lifetime::Temporary)
        {
            CurrentValue().SetString(value.data(), aznumeric_cast<rapidjson::SizeType>(value.length()), m_allocator);
        }
        else
        {
            CurrentValue().SetString(value.data(), aznumeric_cast<rapidjson::SizeType>(value.length()));
        }
        return FinishWrite();
    }

    Visitor::Result RapidJsonValueWriter::StartObject()
    {
        CurrentValue().SetObject();

        const bool isObject = true;
        m_entryStack.emplace_front(isObject, CurrentValue());
        return VisitorSuccess();
    }

    Visitor::Result RapidJsonValueWriter::EndObject(AZ::u64 attributeCount)
    {
        if (m_entryStack.empty())
        {
            return VisitorFailure(VisitorErrorCode::InternalError, "EndObject called without a matching BeginObject call");
        }

        const ValueInfo& frontEntry = m_entryStack.front();
        if (!frontEntry.m_isObject)
        {
            return VisitorFailure(VisitorErrorCode::InternalError, "Expected EndArray and received EndObject instead");
        }

        if (frontEntry.m_entryCount != attributeCount)
        {
            return VisitorFailure(
                VisitorErrorCode::InternalError,
                AZStd::string::format(
                    "EndObject: Expected %llu attributes but received %llu attributes instead", attributeCount,
                    frontEntry.m_entryCount));
        }

        m_entryStack.pop_front();
        return FinishWrite();
    }

    Visitor::Result RapidJsonValueWriter::Key(AZ::Name key)
    {
        return RawKey(key.GetStringView(), Lifetime::Persistent);
    }

    Visitor::Result RapidJsonValueWriter::RawKey(AZStd::string_view key, Lifetime lifetime)
    {
        AZ_Assert(!m_entryStack.empty(), "Attempmted to push a key with no object");
        AZ_Assert(m_entryStack.front().m_isObject, "Attempted to push a key to an array");
        if (lifetime == Lifetime::Persistent)
        {
            m_entryStack.front().m_key.SetString(key.data(), aznumeric_cast<rapidjson::SizeType>(key.size()));
        }
        else
        {
            m_entryStack.front().m_key.SetString(key.data(), aznumeric_cast<rapidjson::SizeType>(key.size()), m_allocator);
        }
        return VisitorSuccess();
    }

    Visitor::Result RapidJsonValueWriter::StartArray()
    {
        CurrentValue().SetArray();

        const bool isObject = false;
        m_entryStack.emplace_front(isObject, CurrentValue());
        return VisitorSuccess();
    }

    Visitor::Result RapidJsonValueWriter::EndArray(AZ::u64 elementCount)
    {
        if (m_entryStack.empty())
        {
            return VisitorFailure(VisitorErrorCode::InternalError, "EndArray called without a matching BeginArray call");
        }

        const ValueInfo& frontEntry = m_entryStack.front();
        if (frontEntry.m_isObject)
        {
            return VisitorFailure(VisitorErrorCode::InternalError, "Expected EndObject and received EndArray instead");
        }

        if (frontEntry.m_entryCount != elementCount)
        {
            return VisitorFailure(
                VisitorErrorCode::InternalError,
                AZStd::string::format(
                    "EndArray: Expected %llu elements but received %llu elements instead", elementCount, frontEntry.m_entryCount));
        }

        m_entryStack.pop_front();
        return FinishWrite();
    }

    Visitor::Result RapidJsonValueWriter::FinishWrite()
    {
        if (m_entryStack.empty())
        {
            return VisitorSuccess();
        }

        // Retrieve the top value of the stack and replace it with a null value
        rapidjson::Value value;
        m_entryStack.front().m_value.Swap(value);
        ValueInfo& newEntry = m_entryStack.front();
        ++newEntry.m_entryCount;

        if (newEntry.m_key.IsString())
        {
            newEntry.m_container.AddMember(m_entryStack.front().m_key.Move(), AZStd::move(value), m_allocator);
            newEntry.m_key.SetNull();
        }
        else
        {
            newEntry.m_container.PushBack(AZStd::move(value), m_allocator);
        }

        return VisitorSuccess();
    }

    rapidjson::Value& RapidJsonValueWriter::CurrentValue()
    {
        if (m_entryStack.empty())
        {
            return m_result;
        }
        return m_entryStack.front().m_value;
    }

    RapidJsonValueWriter::ValueInfo::ValueInfo(bool isObject, rapidjson::Value& container)
        : m_isObject(isObject)
        , m_container(container)
    {
    }

    //
    // class StreamWriter
    //
    // Visitor that writes to a rapidjson::Writer
    template<class Writer>
    class StreamWriter : public Visitor
    {
    public:
        StreamWriter(AZ::IO::GenericStream* stream)
            : m_streamWriter(stream)
            , m_writer(Writer(m_streamWriter))
        {
        }

        VisitorFlags GetVisitorFlags() const override
        {
            return VisitorFlags::SupportsRawKeys | VisitorFlags::SupportsArrays | VisitorFlags::SupportsObjects;
        }

        Result Null() override
        {
            return CheckWrite(m_writer.Null());
        }

        Result Bool(bool value) override
        {
            return CheckWrite(m_writer.Bool(value));
        }

        Result Int64(AZ::s64 value) override
        {
            return CheckWrite(m_writer.Int64(value));
        }

        Result Uint64(AZ::u64 value) override
        {
            return CheckWrite(m_writer.Uint64(value));
        }

        Result Double(double value) override
        {
            return CheckWrite(m_writer.Double(value));
        }

        Result String(AZStd::string_view value, Lifetime lifetime) override
        {
            const bool shouldCopy = lifetime == Lifetime::Temporary;
            return CheckWrite(m_writer.String(value.data(), aznumeric_cast<rapidjson::SizeType>(value.size()), shouldCopy));
        }

        Result StartObject() override
        {
            return CheckWrite(m_writer.StartObject());
        }

        Result EndObject(AZ::u64 attributeCount) override
        {
            return CheckWrite(m_writer.EndObject(aznumeric_cast<rapidjson::SizeType>(attributeCount)));
        }

        Result Key(AZ::Name key) override
        {
            return RawKey(key.GetStringView(), Lifetime::Persistent);
        }

        Result RawKey(AZStd::string_view key, Lifetime lifetime) override
        {
            const bool shouldCopy = lifetime == Lifetime::Temporary;
            return CheckWrite(m_writer.Key(key.data(), aznumeric_cast<rapidjson::SizeType>(key.size()), shouldCopy));
        }

        Result StartArray() override
        {
            return CheckWrite(m_writer.StartArray());
        }

        Result EndArray(AZ::u64 elementCount) override
        {
            return CheckWrite(m_writer.EndArray(aznumeric_cast<rapidjson::SizeType>(elementCount)));
        }

    private:
        Result CheckWrite(bool writeSucceeded)
        {
            if (writeSucceeded)
            {
                return VisitorSuccess();
            }
            else
            {
                return VisitorFailure(VisitorErrorCode::InternalError, "Failed to write JSON");
            }
        }

        AZ::IO::RapidJSONStreamWriter m_streamWriter;
        Writer m_writer;
    };

    //
    // struct JsonReadHandler
    //
    RapidJsonReadHandler::RapidJsonReadHandler(Visitor* visitor, Lifetime stringLifetime)
        : m_visitor(visitor)
        , m_stringLifetime(stringLifetime)
        , m_outcome(AZ::Success())
    {
    }

    bool RapidJsonReadHandler::Null()
    {
        return CheckResult(m_visitor->Null());
    }

    bool RapidJsonReadHandler::Bool(bool b)
    {
        return CheckResult(m_visitor->Bool(b));
    }

    bool RapidJsonReadHandler::Int(int i)
    {
        return CheckResult(m_visitor->Int64(aznumeric_cast<AZ::s64>(i)));
    }

    bool RapidJsonReadHandler::Uint(unsigned i)
    {
        return CheckResult(m_visitor->Uint64(aznumeric_cast<AZ::u64>(i)));
    }

    bool RapidJsonReadHandler::Int64(int64_t i)
    {
        return CheckResult(m_visitor->Int64(i));
    }

    bool RapidJsonReadHandler::Uint64(uint64_t i)
    {
        return CheckResult(m_visitor->Uint64(i));
    }

    bool RapidJsonReadHandler::Double(double d)
    {
        return CheckResult(m_visitor->Double(d));
    }

    bool RapidJsonReadHandler::RawNumber(
        [[maybe_unused]] const char* str, [[maybe_unused]] rapidjson::SizeType length, [[maybe_unused]] bool copy)
    {
        AZ_Assert(false, "Raw numbers are unsupported in the rapidjson DOM backend");
        return false;
    }

    bool RapidJsonReadHandler::String(const char* str, rapidjson::SizeType length, bool copy)
    {
        const Lifetime lifetime = copy ? m_stringLifetime : Lifetime::Temporary;
        return CheckResult(m_visitor->String(AZStd::string_view(str, length), lifetime));
    }

    bool RapidJsonReadHandler::StartObject()
    {
        return CheckResult(m_visitor->StartObject());
    }

    bool RapidJsonReadHandler::Key(const char* str, rapidjson::SizeType length, [[maybe_unused]] bool copy)
    {
        AZStd::string_view key = AZStd::string_view(str, length);
        if (!m_visitor->SupportsRawKeys())
        {
            m_visitor->Key(AZ::Name(key));
        }
        const Lifetime lifetime = copy ? m_stringLifetime : Lifetime::Temporary;
        return CheckResult(m_visitor->RawKey(key, lifetime));
    }

    bool RapidJsonReadHandler::EndObject([[maybe_unused]] rapidjson::SizeType memberCount)
    {
        return CheckResult(m_visitor->EndObject(memberCount));
    }

    bool RapidJsonReadHandler::StartArray()
    {
        return CheckResult(m_visitor->StartArray());
    }

    bool RapidJsonReadHandler::EndArray([[maybe_unused]] rapidjson::SizeType elementCount)
    {
        return CheckResult(m_visitor->EndArray(elementCount));
    }

    Visitor::Result&& RapidJsonReadHandler::TakeOutcome()
    {
        return AZStd::move(m_outcome);
    }

    bool RapidJsonReadHandler::CheckResult(Visitor::Result result)
    {
        if (result.IsSuccess())
        {
            return true;
        }
        else
        {
            m_outcome = AZStd::move(result);
            return false;
        }
    }

    //
    // Serialized JSON util functions
    //
    AZStd::unique_ptr<Visitor> CreateJsonStreamWriter(AZ::IO::GenericStream& stream, OutputFormatting format)
    {
        if (format == OutputFormatting::MinifiedJson)
        {
            using WriterType = rapidjson::Writer<AZ::IO::RapidJSONStreamWriter>;
            return AZStd::make_unique<StreamWriter<WriterType>>(&stream);
        }
        else
        {
            using WriterType = rapidjson::PrettyWriter<AZ::IO::RapidJSONStreamWriter>;
            return AZStd::make_unique<StreamWriter<WriterType>>(&stream);
        }
    }

    //
    // In-memory rapidjson util functions
    //
    AZ::Outcome<rapidjson::Document, AZStd::string> WriteToRapidJsonDocument(Backend::WriteCallback writeCallback)
    {
        rapidjson::Document document;
        RapidJsonValueWriter writer(document, document.GetAllocator());
        auto result = writeCallback(writer);
        if (!result.IsSuccess())
        {
            return AZ::Failure(result.TakeError().FormatVisitorErrorMessage());
        }
        return AZ::Success(AZStd::move(document));
    }

    Visitor::Result WriteToRapidJsonValue(
        rapidjson::Value& value, rapidjson::Value::AllocatorType& allocator, Backend::WriteCallback writeCallback)
    {
        RapidJsonValueWriter writer(value, allocator);
        return writeCallback(writer);
    }

    Visitor::Result VisitRapidJsonValue(const rapidjson::Value& value, Visitor& visitor, Lifetime lifetime)
    {
        struct EndArrayMarker
        {
        };
        struct EndObjectMarker
        {
        };

        // Processing stack consists of values comprised of one of a:
        // - rapidjson::Value to process
        // - EndArrayMarker or EndObjectMarker denoting the end of an array or object
        // - string denoting a key at the beginning of a key/value pair
        using Entry = AZStd::variant<const rapidjson::Value*, EndArrayMarker, EndObjectMarker, AZStd::string_view>;
        AZStd::stack<Entry> entryStack;
        AZStd::stack<u64> entryCountStack;
        entryStack.push(&value);

        while (!entryStack.empty())
        {
            const Entry currentEntry = entryStack.top();
            entryStack.pop();

            Visitor::Result result = AZ::Success();

            AZStd::visit(
                [&visitor, &entryStack, &entryCountStack, &result, lifetime](auto&& arg)
                {
                    using Alternative = AZStd::decay_t<decltype(arg)>;
                    if constexpr (AZStd::is_same_v<Alternative, const rapidjson::Value*>)
                    {
                        const rapidjson::Value& currentValue = *arg;
                        if (!entryCountStack.empty())
                        {
                            ++entryCountStack.top();
                        }

                        switch (currentValue.GetType())
                        {
                        case rapidjson::kNullType:
                            result = visitor.Null();
                            break;
                        case rapidjson::kFalseType:
                            result = visitor.Bool(false);
                            break;
                        case rapidjson::kTrueType:
                            result = visitor.Bool(true);
                            break;
                        case rapidjson::kObjectType:
                            entryStack.push(EndObjectMarker{});
                            entryCountStack.push(0);
                            result = visitor.StartObject();
                            for (auto it = currentValue.MemberEnd(); it != currentValue.MemberBegin(); --it)
                            {
                                auto entry = (it - 1);
                                const AZStd::string_view key(
                                    entry->name.GetString(), aznumeric_cast<size_t>(entry->name.GetStringLength()));
                                entryStack.push(&entry->value);
                                entryStack.push(key);
                            }
                            break;
                        case rapidjson::kArrayType:
                            entryStack.push(EndArrayMarker{});
                            entryCountStack.push(0);
                            result = visitor.StartArray();
                            for (auto it = currentValue.End(); it != currentValue.Begin(); --it)
                            {
                                auto entry = (it - 1);
                                entryStack.push(entry);
                            }
                            break;
                        case rapidjson::kStringType:
                            result = visitor.String(
                                AZStd::string_view(currentValue.GetString(), aznumeric_cast<size_t>(currentValue.GetStringLength())),
                                lifetime);
                            break;
                        case rapidjson::kNumberType:
                            if (currentValue.IsFloat() || currentValue.IsDouble())
                            {
                                result = visitor.Double(currentValue.GetDouble());
                            }
                            else if (currentValue.IsInt64() || currentValue.IsInt())
                            {
                                result = visitor.Int64(currentValue.GetInt64());
                            }
                            else
                            {
                                result = visitor.Uint64(currentValue.GetUint64());
                            }
                            break;
                        default:
                            result = AZ::Failure(VisitorError(VisitorErrorCode::InvalidData, "Value with invalid type specified"));
                        }
                    }
                    else if constexpr (AZStd::is_same_v<Alternative, EndArrayMarker>)
                    {
                        result = visitor.EndArray(entryCountStack.top());
                        entryCountStack.pop();
                    }
                    else if constexpr (AZStd::is_same_v<Alternative, EndObjectMarker>)
                    {
                        result = visitor.EndObject(entryCountStack.top());
                        entryCountStack.pop();
                    }
                    else if constexpr (AZStd::is_same_v<Alternative, AZStd::string_view>)
                    {
                        if (visitor.SupportsRawKeys())
                        {
                            visitor.RawKey(arg, lifetime);
                        }
                        else
                        {
                            visitor.Key(AZ::Name(arg));
                        }
                    }
                },
                currentEntry);

            if (!result.IsSuccess())
            {
                return result;
            }
        }

        return AZ::Success();
    }
} // namespace AZ::Dom::Json
