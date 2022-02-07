/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/JSON/writer.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>

namespace AWSCore
{

    class JsonOutputStream
    {
    public:
        typedef char Ch;
        JsonOutputStream(std::ostream& os) : m_os(os)
        {
        }

        Ch Peek() const
        {
            AZ_Assert(false, "Not Implemented");
            return '\0';
        }

        Ch Take()
        {
            AZ_Assert(false, "Not Implemented");
            return '\0';
        }

        size_t Tell() const
        {
            AZ_Assert(false, "Not Implemented");
            return 0;
        }

        Ch* PutBegin()
        {
            AZ_Assert(false, "Not Implemented");
            return nullptr;
        }

        void Put(Ch c)
        {
            m_os.put(c);
        }

        void Flush()
        {
            m_os.flush();
        }

        size_t PutEnd(Ch*)
        {
            AZ_Assert(false, "Not Implemented");
            return 0;
        }

    private:
        JsonOutputStream(const JsonOutputStream&) = delete;
        JsonOutputStream& operator=(const JsonOutputStream&) = delete;
        std::ostream& m_os;
    };

    class JsonWriter;

    /// The default GlobalWriteJson implementation. Provide
    /// specializations of this function in the ServiceApi
    /// namespace to implement serialization differently 
    /// for specific types.
    template<class ObjectType>
    bool GlobalWriteJson(JsonWriter& writer, const ObjectType& source)
    {
        return source.WriteJson(writer);
    }

    /// Used to produce request json format content.
    class JsonWriter
        : public rapidjson::Writer<JsonOutputStream>
    {

        using BaseType = rapidjson::Writer<JsonOutputStream>;

    public:

        template<typename ObjectType>
        static bool WriteObject(JsonOutputStream& os, const ObjectType& object)
        {
            JsonWriter writer{ os };
            return GlobalWriteJson(writer, object);
        }

        JsonWriter(JsonOutputStream& os)
            : BaseType{ os }
        {
        }

        bool Write(const char* s)
        {
            return String(s);
        }

        bool Write(const AZStd::string& s)
        {
            return String(s);
        }

        bool Write(int i)
        {
            return Int(i);
        }

        bool Write(unsigned i)
        {
            return Uint(i);
        }

        bool Write(int64_t i)
        {
            return Int64(i);
        }

        bool Write(uint64_t i)
        {
            return Uint64(i);
        }

        bool Write(bool b)
        {
            return Bool(b);
        }

        template<typename ElementType>
        bool Write(const AZStd::vector<ElementType, AZStd::allocator>& v)
        {
            return Array(v);
        }

        template<class ObjectType>
        bool Write(const ObjectType& o)
        {
            return Object(o);
        }

        template<class ValueType>
        bool Write(const char* key, const ValueType& value)
        {
            bool ok = Key(key);
            if (ok)
            {
                ok = Write(value);
            }
            return ok;
        }

        /// Write JSON format content directly to the writer's output stream.
        /// This can be used to efficiently output static content.
        bool WriteJson(const Ch* json)
        {
            if (json)
            {
                while (*json != '\0')
                {
                    os_->Put(*json);
                    ++json;
                }
            }
            return true;
        }

        /// Overload of String that calls c_str for you.
        bool String(const AZStd::string& str)
        {
            return BaseType::String(str.c_str());
        }

        /// Write an object. The object can implement a WriteJson function
        /// or you can provide an GlobalWriteJson template function 
        /// specialization.
        template<class ObjectType>
        bool Object(const ObjectType& obj)
        {
            return GlobalWriteJson(*this, obj);
        }

        template<class ElementType>
        bool Array(const AZStd::vector<ElementType, AZStd::allocator>& v)
        {
            bool ok = StartArray();
            if (ok)
            {
                for (const ElementType& e : v)
                {
                    ok = Write(e);
                    if (!ok) break;
                }
                if (ok)
                {
                    ok = EndArray();
                }
            }
            return ok;
        }

    private:
        // Prohibit copy constructor & assignment operator.
        JsonWriter(const JsonWriter&) = delete;
        JsonWriter& operator=(const JsonWriter&) = delete;
    };

} // namespace AWSCore
