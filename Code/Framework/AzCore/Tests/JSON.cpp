/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>     // rapidjson's DOM-style API
#include <AzCore/JSON/prettywriter.h> // for stringify JSON
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;
using namespace rapidjson;

namespace UnitTest
{
    /**
     * Rapid XML parser
     */
    class RapidJSON
        : public LeakDetectionFixture
    {
    public:
        RapidJSON() {}

        void run()
        {
            // rapidjson is already tested (has unittests), for now just run the tututorial
            // otherwise we can include all of it's test
            const char json[] = " { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ";

            Document document;  // Default template parameter uses UTF8 and MemoryPoolAllocator.

            // In-situ parsing, decode strings directly in the source string. Source must be string.
            char buffer[sizeof(json)];
            memcpy(buffer, json, sizeof(json));
            AZ_TEST_ASSERT(document.ParseInsitu(buffer).HasParseError() == false);

            ////////////////////////////////////////////////////////////////////////////
            // 2. Access values in document.

            AZ_TEST_ASSERT(document.IsObject());    // Document is a JSON value represents the root of DOM. Root can be either an object or array.

            AZ_TEST_ASSERT(document.HasMember("hello"));
            AZ_TEST_ASSERT(document["hello"].IsString());

            // Since version 0.2, you can use single lookup to check the existing of member and its value:
            Value::MemberIterator hello = document.FindMember("hello");
            AZ_TEST_ASSERT(hello != document.MemberEnd());
            AZ_TEST_ASSERT(hello->value.IsString());
            AZ_TEST_ASSERT(strcmp("world", hello->value.GetString()) == 0);
            (void)hello;

            AZ_TEST_ASSERT(document["t"].IsBool());     // JSON true/false are bool. Can also uses more specific function IsTrue().

            AZ_TEST_ASSERT(document["f"].IsBool());

            AZ_TEST_ASSERT(document["i"].IsNumber());   // Number is a JSON type, but C++ needs more specific type.
            AZ_TEST_ASSERT(document["i"].IsInt());      // In this case, IsUint()/IsInt64()/IsUInt64() also return true.

            AZ_TEST_ASSERT(document["pi"].IsNumber());
            AZ_TEST_ASSERT(document["pi"].IsDouble());

            {
                const Value& a = document["a"]; // Using a reference for consecutive access is handy and faster.
                AZ_TEST_ASSERT(a.IsArray());

                int y = a[0].GetInt();
                (void)y;
            }

            // Iterating object members
            //static const char* kTypeNames[] = { "Null", "False", "True", "Object", "Array", "String", "Number" };
            for (Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
            {
                (void)itr;
                //printf("Type of member %s is %s\n", itr->name.GetString(), kTypeNames[itr->value.GetType()]);
            }

            ////////////////////////////////////////////////////////////////////////////
            // 3. Modify values in document.

            // Change i to a bigger number
            {
                uint64_t f20 = 1;   // compute factorial of 20
                for (uint64_t j = 1; j <= 20; j++)
                {
                    f20 *= j;
                }
                document["i"] = f20;    // Alternate form: document["i"].SetUint64(f20)
                AZ_TEST_ASSERT(!document["i"].IsInt()); // No longer can be cast as int or uint.
            }

            // Adding values to array.
            {
                Value& a = document["a"];   // This time we uses non-const reference.
                Document::AllocatorType& allocator = document.GetAllocator();
                for (int i = 5; i <= 10; i++)
                {
                    a.PushBack(i, allocator);   // May look a bit strange, allocator is needed for potentially realloc. We normally uses the document's.
                }
                // Fluent API
                a.PushBack("Lua", allocator).PushBack("Mio", allocator);
            }

            // Making string values.

            // This version of SetString() just store the pointer to the string.
            // So it is for literal and string that exists within value's life-cycle.
            {
                document["hello"] = "rapidjson";    // This will invoke strlen()
                                                    // Faster version:
                                                    // document["hello"].SetString("rapidjson", 9);
            }

            // This version of SetString() needs an allocator, which means it will allocate a new buffer and copy the the string into the buffer.
            Value author;
            {
                char lbuffer[10];
                int len = azsnprintf(lbuffer, AZ_ARRAY_SIZE(lbuffer), "%s %s", "Milo", "Yip");  // synthetic example of dynamically created string.

                author.SetString(lbuffer, static_cast<rapidjson_ly::SizeType>(len), document.GetAllocator());
                // Shorter but slower version:
                // document["hello"].SetString(buffer, document.GetAllocator());

                // Constructor version:
                // Value author(lbuffer, len, document.GetAllocator());
                // Value author(lbuffer, document.GetAllocator());
                memset(lbuffer, 0, sizeof(lbuffer)); // For demonstration purpose.
            }
            // Variable 'buffer' is unusable now but 'author' has already made a copy.
            document.AddMember("author", author, document.GetAllocator());

            AZ_TEST_ASSERT(author.IsNull());        // Move semantic for assignment. After this variable is assigned as a member, the variable becomes null.

            ////////////////////////////////////////////////////////////////////////////
            // 4. Stringify JSON

            StringBuffer sb;
            PrettyWriter<StringBuffer> writer(sb);
            document.Accept(writer);    // Accept() traverses the DOM and generates Handler events.

            // Test error handling
            {
                Document badDoc;
                badDoc.Parse("{ dflakjdflkajdlfkja");
                AZ_TEST_ASSERT(badDoc.HasParseError());
                AZ_TEST_ASSERT(badDoc.GetParseError() != kParseErrorNone);
                AZ_TEST_ASSERT(badDoc.GetErrorOffset() != 0);
            }
        }
    };

    TEST_F(RapidJSON, Test)
    {
        run();
    }
}
