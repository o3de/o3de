/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/JSON/error/error.h>

#include <Framework/JsonObjectHandler.h>


namespace AWSCore
{

    class JsonReaderHandler
        : public JsonReader
    {

    public:
        virtual ~JsonReaderHandler() = default;

        using Ch = char;
        using SizeType = rapidjson::SizeType;

        bool StartObject()
        {
            if (!CallArrayHandler())
            {
                return false;
            }
            if (m_expecting != Expecting::OBJECT)
            {
                return UnexpectedContent(Expecting::OBJECT);
            }
            else
            {
                m_jsonKeyHandlerStack.push_back(m_targetKeyHandler);
                m_jsonArrayHandlerStack.push_back(nullptr);
                return true;
            }
        }

        bool EndObject(rapidjson::SizeType memberCount)
        {
            AZ_UNUSED(memberCount);

            m_jsonKeyHandlerStack.pop_back();
            m_jsonArrayHandlerStack.pop_back();
            return true;
        }

        bool StartArray()
        {
            if (!CallArrayHandler())
            {
                return false;
            }
            if (m_expecting != Expecting::ARRAY)
            {
                return UnexpectedContent(Expecting::ARRAY);
            }
            else
            {
                m_jsonArrayHandlerStack.push_back(m_targetArrayHandler);
                m_jsonKeyHandlerStack.push_back(nullptr);
                return true;
            }
        }

        bool EndArray(rapidjson::SizeType elementCount)
        {
            AZ_UNUSED(elementCount);

            m_jsonKeyHandlerStack.pop_back();
            m_jsonArrayHandlerStack.pop_back();
            return true;
        }

        bool Key(const Ch* str, rapidjson::SizeType length, bool copy)
        {
            AZ_UNUSED(str);
            AZ_UNUSED(length);
            AZ_UNUSED(copy);

            if (m_jsonKeyHandlerStack.empty() || !m_jsonKeyHandlerStack.back())
            {
                return UnexpectedContent("key");
            }
            else
            {
                m_expecting = Expecting::NOTHING; // default to ignoring 
                bool ok = m_jsonKeyHandlerStack.back()(str, *this);
                if (!ok)
                {
                    return UnexpectedObjectKey(str);
                }
                return true;
            }
        }

        inline bool CallArrayHandler()
        {
            if (!m_jsonArrayHandlerStack.empty() && m_jsonArrayHandlerStack.back())
            {
                bool ok = m_jsonArrayHandlerStack.back()(*this);
                if (!ok)
                {
                    return UnexpectedArrayElement();
                }
            }
            return true;
        }

        bool String(const Ch* str, rapidjson::SizeType length, bool copy)
        {
            AZ_UNUSED(str);
            AZ_UNUSED(length);
            AZ_UNUSED(copy);

            if (!CallArrayHandler())
            {
                return false;
            }
            if (m_expecting != Expecting::STRING)
            {
                return UnexpectedContent(Expecting::STRING);
            }
            else
            {
                // Doesn't support embedded \0, which rapidjson allows.
                *m_targetString = AZStd::string::format("%.*s", aznumeric_cast<int>(length), str);
                return true;
            }

        }

        bool RawNumber(const Ch* str, rapidjson::SizeType length, bool copy)
        {
            return String(str, length, copy);
        }

        bool Null()
        {
            if (!CallArrayHandler())
            {
                return false;
            }
            return UnexpectedContent("null");
        }

        bool Bool(bool b)
        {
            if (!CallArrayHandler())
            {
                return false;
            }
            if (m_expecting != Expecting::BOOL)
            {
                return UnexpectedContent(Expecting::BOOL);
            }
            else
            {
                *m_targetBool = b;
                return true;
            }
        }

        bool Int(int i)
        {
            if (!CallArrayHandler())
            {
                return false;
            }
            if (m_expecting == Expecting::INT)
            {
                *m_targetInt = i;
                return true;
            }
            if (m_expecting == Expecting::INT64)
            {
                *m_targetInt64 = i;
                return true;
            }
            if (m_expecting == Expecting::DOUBLE)
            {
                *m_targetDouble = i;
                return true;
            }
            return UnexpectedContent(Expecting::INT);
        }

        bool Uint(unsigned i)
        {
            if (!CallArrayHandler())
            {
                return false;
            }
            if (m_expecting == Expecting::INT && i <= INT_MAX)
            {
                *m_targetInt = i;
                return true;
            }
            if (m_expecting == Expecting::UINT)
            {
                *m_targetUInt = i;
                return true;
            }
            if (m_expecting == Expecting::INT64)
            {
                *m_targetInt64 = i;
                return true;
            }
            if (m_expecting == Expecting::UINT64)
            {
                *m_targetUInt64 = i;
                return true;
            }
            if (m_expecting == Expecting::DOUBLE)
            {
                *m_targetDouble = i;
                return true;
            }
            return UnexpectedContent("unsigned");
        }

        bool Int64(int64_t i)
        {
            if (!CallArrayHandler())
            {
                return false;
            }
            if (m_expecting == Expecting::INT64)
            {
                *m_targetInt64 = i;
                return true;
            }
            if (m_expecting == Expecting::DOUBLE)
            {
                *m_targetDouble = aznumeric_caster(i);
                return true;
            }
            return UnexpectedContent(Expecting::INT64);
        }

        bool Uint64(uint64_t i)
        {
            if (!CallArrayHandler())
            {
                return false;
            }
            if (m_expecting == Expecting::INT64 && i < INT64_MAX)
            {
                *m_targetInt64 = i;
                return true;
            }
            if (m_expecting == Expecting::UINT64)
            {
                *m_targetUInt64 = i;
                return true;
            }
            if (m_expecting == Expecting::DOUBLE)
            {
                *m_targetDouble = aznumeric_caster(i);
                return true;
            }
            return UnexpectedContent(Expecting::UINT64);
        }

        bool Double(double d)
        {
            if (!CallArrayHandler())
            {
                return false;
            }
            if (m_expecting == Expecting::DOUBLE)
            {
                *m_targetDouble = d;
                return true;
            }
            return UnexpectedContent(Expecting::DOUBLE);
        }

        bool Ignore() override
        {
            m_expecting = JsonReaderHandler::Expecting::NOTHING;
            return true;
        }

        /// Tell the JsonReaderHandler that a boolean value is expected and provide 
        /// a location where the value can be stored.
        bool Accept(bool& target) override
        {
            m_targetBool = &target;
            m_expecting = JsonReaderHandler::Expecting::BOOL;
            return true;
        }

        /// Tell the JsonReaderHandler that a string value is expected and provide 
        /// a location where the value can be stored.
        bool Accept(AZStd::string& target) override
        {
            m_targetString = &target;
            m_expecting = JsonReaderHandler::Expecting::STRING;
            return true;
        }

        /// Tell the JsonReaderHandler that an int value is expected and provide 
        /// a location where the value can be stored.
        bool Accept(int& target) override
        {
            m_targetInt = &target;
            m_expecting = JsonReaderHandler::Expecting::INT;
            return true;
        }

        /// Tell the JsonReaderHandler that an unsigned value is expected and provide 
        /// a location where the value can be stored.
        bool Accept(unsigned& target) override
        {
            m_targetUInt = &target;
            m_expecting = JsonReaderHandler::Expecting::UINT;
            return true;
        }

        /// Tell the JsonReaderHandler that a int64_t value is expected and provide 
        /// a location where the value can be stored.
        bool Accept(int64_t& target) override
        {
            m_targetInt64 = &target;
            m_expecting = JsonReaderHandler::Expecting::INT64;
            return true;
        }

        /// Tell the JsonReaderHandler that a uint64_t value is expected and provide 
        /// a location where the value can be stored.
        bool Accept(uint64_t& target) override
        {
            m_targetUInt64 = &target;
            m_expecting = JsonReaderHandler::Expecting::UINT64;
            return true;
        }

        /// Tell the JsonReaderHandler that a double value is expected and provide 
        /// a location where the value can be stored.
        bool Accept(double& target) override
        {
            m_targetDouble = &target;
            m_expecting = JsonReaderHandler::Expecting::DOUBLE;
            return true;
        }

        /// Tell the JsonReaderHandler that an object is expected and provide
        /// a JsonKeyHandler function for that object.
        bool Accept(JsonKeyHandler keyHandler) override
        {
            m_targetKeyHandler = keyHandler;
            m_expecting = JsonReaderHandler::Expecting::OBJECT;
            return true;
        }

        bool Accept(JsonArrayHandler arrayHandler) override
        {
            m_expecting = JsonReaderHandler::Expecting::ARRAY;
            m_targetArrayHandler = arrayHandler;
            return true;
        }

        AZStd::string GetParseErrorMessage(const rapidjson::ParseResult& result, JsonInputStream& stream)
        {

            AZStd::string msg;

            switch (result.Code())
            {

            case rapidjson::kParseErrorNone:
                msg = "No error";
                break;
            case rapidjson::kParseErrorDocumentEmpty:
                msg = "The document is empty";
                break;
            case rapidjson::kParseErrorDocumentRootNotSingular:
                msg = "The document root must not follow by other values";
                break;
            case rapidjson::kParseErrorValueInvalid:
                msg = "Invalid value";
                break;
            case rapidjson::kParseErrorObjectMissName:
                msg = "Missing a name for object member";
                break;
            case rapidjson::kParseErrorObjectMissColon:
                msg = "Missing a colon after a name of object member";
                break;
            case rapidjson::kParseErrorObjectMissCommaOrCurlyBracket:
                msg = "Missing a comma or '}' after an object member";
                break;
            case rapidjson::kParseErrorArrayMissCommaOrSquareBracket:
                msg = "Missing a comma or ']' after an array element";
                break;
            case rapidjson::kParseErrorStringUnicodeEscapeInvalidHex:
                msg = "Incorrect hex digit after \\u escape in string";
                break;
            case rapidjson::kParseErrorStringUnicodeSurrogateInvalid:
                msg = "The surrogate pair in string is invalid";
                break;
            case rapidjson::kParseErrorStringEscapeInvalid:
                msg = "Invalid escape character in string";
                break;
            case rapidjson::kParseErrorStringMissQuotationMark:
                msg = "Missing a closing quotation mark in string";
                break;
            case rapidjson::kParseErrorStringInvalidEncoding:
                msg = "Invalid encoding in string";
                break;
            case rapidjson::kParseErrorNumberTooBig:
                msg = "Number too big to be stored in double";
                break;
            case rapidjson::kParseErrorNumberMissFraction:
                msg = "Miss fraction part in number";
                break;
            case rapidjson::kParseErrorNumberMissExponent:
                msg = "Miss exponent in number";
                break;
            case rapidjson::kParseErrorTermination:
                if (m_errorMessage.empty())
                {
                    msg = "Parsing terminated";
                }
                else
                {
                    msg = m_errorMessage;
                }
                break;
            case rapidjson::kParseErrorUnspecificSyntaxError:
                msg = "Unspecific syntax error";
                break;
            default:
                msg = AZStd::string::format("Unexpected error code %i", result.Code());
                break;
            }

            msg += AZStd::string::format(" at character %zu: ", result.Offset());

            const int snippet_size = 40;
            int start = static_cast<int>(result.Offset() - snippet_size / 2);
            int length = snippet_size;
            int offset = snippet_size / 2;
            if (start < 0) {
                length -= -start;
                offset -= -start;
                start = 0;
            }
            AZStd::string snippet = stream.GetContent().substr(start, length);
            if (offset >= 0 && offset <= snippet.size())
            {
                snippet.insert(offset, " <--- ");
            }

            msg += snippet;

            return msg;

        }

    protected:

        friend class JsonReader;

        enum class Expecting
        {
            ARRAY,
            BOOL,
            DOUBLE,
            INT,
            INT64,
            NOTHING,
            OBJECT,
            STRING,
            UINT,
            UINT64
        };

        Expecting m_expecting{ Expecting::NOTHING };

        bool* m_targetBool{ nullptr };
        AZStd::string* m_targetString{ nullptr };
        int* m_targetInt{ nullptr };
        unsigned* m_targetUInt{ nullptr };
        int64_t* m_targetInt64{ nullptr };
        uint64_t* m_targetUInt64{ nullptr };
        double* m_targetDouble{ nullptr };
        JsonKeyHandler m_targetKeyHandler{};
        JsonArrayHandler m_targetArrayHandler{};

        AZStd::vector<JsonKeyHandler> m_jsonKeyHandlerStack;
        AZStd::vector<JsonArrayHandler> m_jsonArrayHandlerStack;

        AZStd::string m_errorMessage;

        bool UnexpectedObjectKey(const char* key)
        {
            m_errorMessage = AZStd::string::format("Found unexpected object key %s",
                key
            );
            return false;
        }

        bool UnexpectedArrayElement()
        {
            m_errorMessage = "Found unexpected array element";
            return false;
        }

        bool UnexpectedContent(Expecting actual)
        {
            return UnexpectedContent(ExpectingToString(actual));
        }

        bool UnexpectedContent(const char* actual)
        {
            bool result = false;

            if (m_expecting == Expecting::NOTHING)
            {
                result = true;
            }
            else if (m_expecting == Expecting::STRING && !strcmp(actual, "null"))
            {
                // We are allowing null values to parse as empty strings as a workaround for optional fields not always being handled correctly.
                result = true;
            }
            else
            {
                m_errorMessage = AZStd::string::format("Found %s when expecting %s",
                    actual,
                    ExpectingToString(m_expecting)
                );
            }

            return result;
        }

        static const char* ExpectingToString(Expecting expecting)
        {
            switch (expecting)
            {
            case Expecting::ARRAY:   return "an array";
            case Expecting::BOOL:    return "a boolean";
            case Expecting::DOUBLE:  return "a double";
            case Expecting::INT:     return "an int";
            case Expecting::INT64:   return "an int64";
            case Expecting::NOTHING: return "nothing";
            case Expecting::OBJECT:  return "an object";
            case Expecting::STRING:  return "a string";
            case Expecting::UINT:    return "an unsigned";
            case Expecting::UINT64:  return "an uint64";
            default: return "unknown";
            }
        }

    };


    bool JsonReader::ReadObject(JsonInputStream& stream, JsonKeyHandler keyHandler, AZStd::string& errorMessage)
    {
        JsonReaderHandler handler;
        handler.Accept(keyHandler);
        rapidjson::Reader reader;
        rapidjson::ParseResult result = reader.Parse(stream, handler);
        if (result.IsError())
        {
            errorMessage = handler.GetParseErrorMessage(result, stream);
            return false;
        }
        else
        {
            return true;
        }
    }

} // namespace AWSCore
