/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "StringIdPool.h"
#include "LogManager.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace MCore
{
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(StringIdPoolIndex, "StringIdPoolIndex", "{C374F051-8323-49DB-A1BD-C6B6CF0333C0}");

    StringIdPool::StringIdPool()
    {
        Reserve(10000);
        GenerateIdForString("");
    }


    StringIdPool::~StringIdPool()
    {
        Clear();
    }


    void StringIdPool::Clear()
    {
        Lock();

        for (AZStd::basic_string<char>*& string : m_strings)
        {
            delete string;
        }
        m_strings.clear();

        m_stringToIndex.clear();
        Unlock();
    }


    AZ::u32 StringIdPool::GenerateIdForStringWithoutLock(const AZStd::string& objectName)
    {
        // Try to insert it, if we hit a collision, we have the element.
        auto iterator = m_stringToIndex.emplace(objectName, aznumeric_caster(m_strings.size()));
        if (!iterator.second)
        {
            // could not insert, we have the element
            return iterator.first->second;
        }

        // Create the new string object and push it to the string list.
        AZStd::string* newString = new AZStd::string(objectName);
        m_strings.push_back(newString);

        // The string was already added to the hashmap
        return iterator.first->second;
    }


    AZ::u32 StringIdPool::GenerateIdForString(const AZStd::string& objectName)
    {
        Lock();
        const AZ::u32 result = GenerateIdForStringWithoutLock(objectName);
        Unlock();
        return result;
    }


    const AZStd::string& StringIdPool::GetName(AZ::u32 id)
    {
        Lock();
        MCORE_ASSERT(id != InvalidIndex32);
        const AZStd::string* stringAddress = m_strings[id];
        Unlock();
        return *stringAddress;
    }


    void StringIdPool::Reserve(size_t numStrings)
    {
        Lock();
        m_strings.reserve(numStrings);
        Unlock();
    }


    // Wait with execution until we can set the lock.
    void StringIdPool::Lock()
    {
        m_mutex.Lock();
    }


    // Release the lock again.
    void StringIdPool::Unlock()
    {
        m_mutex.Unlock();
    }


    void StringIdPool::Log(bool includeEntries)
    {
        AZ_Printf("EMotionFX", "StringIdPool: NumEntries=%d\n", m_strings.size());

        if (includeEntries)
        {
            const size_t numStrings = m_strings.size();
            for (size_t i = 0; i < numStrings; ++i)
            {
                AZ_Printf("EMotionFX", "   #%d: String='%s', Id=%d\n", i, m_strings[i]->c_str(), GenerateIdForString(m_strings[i]->c_str()));
            }
        }
    }

    class StringIdPoolIndexSerializer
        : public AZ::SerializeContext::IDataSerializer
    {
    public:
        /// Store the class data into a stream.
        size_t Save(const void* classPtr, AZ::IO::GenericStream& stream, bool /*isDataBigEndian = false*/)
        {
            // Look up the string to save
            const AZ::u32 index = static_cast<const StringIdPoolIndex*>(classPtr)->m_index;
            if (index == InvalidIndex32)
            {
                return 0;
            }
            const AZStd::string& string = MCore::GetStringIdPool().GetName(index);

            return stream.Write(string.size(), string.c_str());
        }

        /// Load the class data from a stream.
        bool Load(void* classPtr, AZ::IO::GenericStream& stream, unsigned int /*version*/, bool /*isDataBigEndian = false*/)
        {
            AZStd::string string;
            size_t textLen = stream.GetLength();

            string.resize(textLen);
            stream.Read(textLen, string.data());

            static_cast<StringIdPoolIndex*>(classPtr)->m_index = MCore::GetStringIdPool().GenerateIdForString(string);
            return true;
        }

        /// Convert binary data to text.
        size_t DataToText(AZ::IO::GenericStream& in, AZ::IO::GenericStream& out, bool /*isDataBigEndian = false*/)
        {
            AZ::u64 dataSize = in.GetLength();

            AZStd::string outText;
            outText.resize(dataSize);
            in.Read(dataSize, outText.data());

            return static_cast<size_t>(out.Write(outText.size(), outText.c_str()));
        }

        /// Convert text data to binary, to support loading old version
        // formats. We must respect text version if the text->binary format has
        // changed!
        size_t TextToData(const char* text, unsigned int /*textVersion*/, AZ::IO::GenericStream& stream, bool /*isDataBigEndian = false*/)
        {
            size_t bytesToWrite = strlen(text);

            return static_cast<size_t>(stream.Write(bytesToWrite, reinterpret_cast<const void*>(text)));
        }

        /// Compares two instances of the type.
        /// \return true if they match.
        /// Note: Input pointers are assumed to point to valid instances of the class.
        bool CompareValueData(const void* lhs, const void* rhs)
        {
            return *static_cast<const StringIdPoolIndex*>(lhs) == *static_cast<const StringIdPoolIndex*>(rhs);
        }
    };

    void StringIdPoolIndex::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<StringIdPoolIndex>()
            ->Version(1)
            ->Serializer<StringIdPoolIndexSerializer>()
            ;
    }

} // namespace MCore
