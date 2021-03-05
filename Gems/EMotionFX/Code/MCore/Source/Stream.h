/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include "StandardHeaders.h"
#include <AzCore/std/string/string.h>

namespace MCore
{
    /**
     * The stream abstract base class.
     * A stream is a source from which can be read and to which can be written.
     * This could be a file, a TCP/IP stream, or anything else you can imagine.
     */
    class MCORE_API Stream
    {
        MCORE_MEMORYOBJECTCATEGORY(Stream, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_STREAM)

    public:
        /**
         * The constructor.
         */
        Stream() {}

        /**
         * The destructor.
         */
        virtual ~Stream() {}

        /**
         * Get the unique type ID.
         * @result The type identification number.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Read a given amount of data from the stream.
         * @param data The pointer where to store the read data.
         * @param length The size in bytes of the data to read.
         * @result Returns the number of bytes read.
         */
        virtual size_t Read(void* data, size_t length)              { MCORE_UNUSED(data); MCORE_UNUSED(length); MCORE_ASSERT(false); return true; } // should never be called, since this method should be overloaded somewhere else

        /**
         * Writes a given amount of data to the stream.
         * @param data The pointer to the data to write.
         * @param length The size in bytes of the data to write.
         * @result Returns the number of written bytes.
         */
        virtual size_t Write(const void* data, size_t length)       { MCORE_UNUSED(data); MCORE_UNUSED(length); MCORE_ASSERT(false); return true; } // should never be called, since this method should be overloaded somewhere else

        // write operators
        virtual Stream& operator<<(bool b)                          { Write(&b,      sizeof(bool)); return *this; }
        virtual Stream& operator<<(char ch)                         { Write(&ch,     sizeof(char)); return *this; }
        virtual Stream& operator<<(uint8 ch)                        { Write(&ch,     sizeof(uint8)); return *this; }
        virtual Stream& operator<<(int16 number)                    { Write(&number, sizeof(int16)); return *this; }
        virtual Stream& operator<<(uint16 number)                   { Write(&number, sizeof(uint16)); return *this; }
        virtual Stream& operator<<(int32 number)                    { Write(&number, sizeof(int32)); return *this; }
        virtual Stream& operator<<(uint32 number)                   { Write(&number, sizeof(uint32)); return *this; }
        virtual Stream& operator<<(uint64 number)                   { Write(&number, sizeof(uint64)); return *this; }
        virtual Stream& operator<<(int64 number)                    { Write(&number, sizeof(int64)); return *this; }
        virtual Stream& operator<<(float number)                    { Write(&number, sizeof(float)); return *this; }
        virtual Stream& operator<<(double number)                   { Write(&number, sizeof(double)); return *this; }
        virtual Stream& operator<<(AZStd::string& text)              { Write((void*)text.c_str(), text.size() + 1); return *this; }  // +1 to include the '\0'
        virtual Stream& operator<<(const char* text)                { Write((void*)text, (int32)strlen(text)); char c = '\0'; Write(&c, 1); return *this; }

        // read operators
        virtual Stream& operator>>(bool& b)                         { Read(&b, sizeof(bool));  return *this; }
        virtual Stream& operator>>(char& ch)                        { Read(&ch, sizeof(char)); return *this; }
        virtual Stream& operator>>(uint8& ch)                       { Read(&ch, sizeof(uint8)); return *this; }
        virtual Stream& operator>>(int16& number)                   { Read(&number, sizeof(int16)); return *this; }
        virtual Stream& operator>>(uint16& number)                  { Read(&number, sizeof(uint16)); return *this; }
        virtual Stream& operator>>(int32& number)                   { Read(&number, sizeof(int32)); return *this; }
        virtual Stream& operator>>(uint32& number)                  { Read(&number, sizeof(uint32)); return *this; }
        virtual Stream& operator>>(int64& number)                   { Read(&number, sizeof(int64)); return *this; }
        virtual Stream& operator>>(uint64& number)                  { Read(&number, sizeof(uint64)); return *this; }
        virtual Stream& operator>>(float& number)                   { Read(&number, sizeof(float)); return *this; }
        virtual Stream& operator>>(double& number)                  { Read(&number, sizeof(double)); return *this; }
        virtual Stream& operator>>(AZStd::string& text)
        {
            char c;
            for (;; )
            {
                // check if we can read the character
                if (!Read(&c, 1))
                {
                    return *this;
                }

                // add the character or quit
                if (c != '\0')
                {
                    text += c;
                }
                else
                {
                    break;
                }
            }

            return *this;
        }
    };
} // namespace MCore
