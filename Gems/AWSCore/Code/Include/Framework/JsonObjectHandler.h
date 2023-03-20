/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/JSON/reader.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>

namespace AWSCore
{

    class JsonInputStream
    {
    public:
        typedef char Ch;

        JsonInputStream(std::istream& is) : m_is(is)
        {
        }

        Ch Peek() const
        {
            int c = m_is.peek();
            return c == std::char_traits<char>::eof() ? '\0' : static_cast<Ch>(c);
        }

        Ch Take()
        {
            int c = m_is.get();
            return c == std::char_traits<char>::eof() ? '\0' : static_cast<Ch>(c);
        }

        size_t Tell() const
        {
            return static_cast<size_t>(m_is.tellg());
        }

        Ch* PutBegin()
        {
            AZ_Assert(false, "Not Implemented");
            return nullptr;
        }

        void Put(Ch)
        {
            AZ_Assert(false, "Not Implemented");
        }

        void Flush()
        {
            AZ_Assert(false, "Not Implemented");
        }

        size_t PutEnd(Ch*)
        {
            AZ_Assert(false, "Not Implemented");
            return 0;
        }

        AZStd::string GetContent()
        {
            m_is.seekg(0);
            std::istreambuf_iterator<AZStd::string::value_type> eos;
            AZStd::string content{ std::istreambuf_iterator<AZStd::string::value_type>(m_is),eos };
            return content;
        }

    private:
        JsonInputStream(const JsonInputStream&) = delete;
        JsonInputStream& operator=(const JsonInputStream&) = delete;

        std::istream& m_is;

    };

    class JsonReader;

    /// Type of function called to update a JsonReaderHandler's state when reading 
    /// a JSON object.
    typedef AZStd::function<bool(const char* key, JsonReader& reader)> JsonKeyHandler;

    /// Type of function called to update a JsonReaderHandler's state when reading 
    /// a JSON array.
    typedef AZStd::function<bool(JsonReader& reader)> JsonArrayHandler;

    /// Default GlobalGetJsonKeyHandler template function implementation. Returns a 
    /// lambda that calls the following function on the object:
    ///
    ///     bool OnJsonKey(const char* key, ServiceApi::JsonReader& reader)
    ///     {
    ///         if(strcmp(key, "foo") == 0) return handler.ExpectObject(m_foo)
    ///         if(strcmp(key, "bar") == 0) return handler.ExpectString(m_bar)
    ///         return true; // ignore other keys, return false to fail
    ///     }
    ///
    template<class ObjectType>
    JsonKeyHandler GlobalGetJsonKeyHandler(ObjectType& object)
    {
        return [&object](const char* key, JsonReader& reader) {
            return object.OnJsonKey(key, reader);
        };
    }

    /// Handles the reading of JSON data.
    class JsonReader
    {
    public:
        template<class ObjectType>
        static JsonKeyHandler GetJsonKeyHandler(ObjectType& object)
        {
            return GlobalGetJsonKeyHandler(object);
        }

        /// Read a JSON format object from a stream into an object. ObjectType should
        /// implement the following function:
        ///
        ///     static void OnJsonKey(const char* key, JsonReader& reader)
        ///
        /// This function will be called for each of the object's properties. It should 
        /// call one of the Expect methods on the state object to identify the expected
        /// property type and provide a location where the property value can be stored.
        template<class ObjectType>
        static bool ReadObject(JsonInputStream& stream, ObjectType& object, AZStd::string& errorMessage)
        {
            return ReadObject(stream, GetJsonKeyHandler(object), errorMessage);
        }

        /// Read a JSON format object from a stream. The specified JsonKeyHandler will
        /// be called for each of the object's properties. It should call one of the Expect
        /// methods on the state object to identify the expected property type and provide
        /// a location where the property value can be stored.
        static bool ReadObject(JsonInputStream& stream, JsonKeyHandler keyHandler, AZStd::string&);

        virtual bool Ignore() = 0;

        /// Tell the JsonReaderHandler that a boolean value is expected and provide 
        /// a location where the value can be stored.
        virtual bool Accept(bool& target) = 0;

        /// Tell the JsonReaderHandler that a string value is expected and provide 
        /// a location where the value can be stored.
        virtual bool Accept(AZStd::string& target) = 0;

        /// Tell the JsonReaderHandler that an int value is expected and provide 
        /// a location where the value can be stored.
        virtual bool Accept(int& target) = 0;

        /// Tell the JsonReaderHandler that an unsigned value is expected and provide 
        /// a location where the value can be stored.
        virtual bool Accept(unsigned& target) = 0;

        /// Tell the JsonReaderHandler that a int64_t value is expected and provide 
        /// a location where the value can be stored.
        virtual bool Accept(int64_t& target) = 0;

        /// Tell the JsonReaderHandler that a uint64_t value is expected and provide 
        /// a location where the value can be stored.
        virtual bool Accept(uint64_t& target) = 0;

        /// Tell the JsonReaderHandler that a double value is expected and provide 
        /// a location where the value can be stored.
        virtual bool Accept(double& target) = 0;

        /// Tell the JsonReaderHandler that an object is expected and provide
        /// a JsonKeyHandler function for that object.
        virtual bool Accept(JsonKeyHandler keyHandler) = 0;

        /// Tell the JsonReaderHandler that an object is expected and provide 
        /// a location where the value can be stored. ObjectType should
        /// implement the following function:
        ///
        ///     static void OnJsonKey(const char* key, JsonReader& reader)
        ///
        /// This function will be called for each of the object's properties. It should 
        /// call one of the Expect methods on the state object to identify the expected
        /// property type and provide a location where the property value can be stored.
        template<class ObjectType>
        bool Accept(ObjectType& object)
        {
            return Accept(GlobalGetJsonKeyHandler(object));
        }

        virtual bool Accept(JsonArrayHandler arrayHandler) = 0;

        template<class ElementType>
        bool Accept(AZStd::vector<ElementType>& target)
        {

            target.clear();

            JsonArrayHandler arrayHandler =
                [&target](JsonReader& reader)
            {
                target.resize(target.size() + 1);
                return reader.Accept(target[target.size() - 1]);
            };

            return Accept(arrayHandler);

        }

    };

} // namespace AWSCore
