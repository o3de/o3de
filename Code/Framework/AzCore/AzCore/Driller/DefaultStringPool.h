/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_DRILLER_DEFAULT_STRING_POOL_H
#define AZCORE_DRILLER_DEFAULT_STRING_POOL_H

#include <AzCore/Driller/Stream.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

namespace AZ
{
    namespace Debug
    {
        template<class Key, class Mapped>
        struct unordered_map
        {
            typedef AZStd::unordered_map<Key, Mapped, AZStd::hash<Key>, AZStd::equal_to<Key>, OSStdAllocator> type;
        };

        template<class Key>
        struct unordered_set
        {
            typedef AZStd::unordered_set<Key, AZStd::hash<Key>, AZStd::equal_to<Key>, OSStdAllocator> type;
        };

        /**
         * Default implementation of a string pool.
         */
        class DrillerDefaultStringPool
            : public DrillerStringPool
        {
        public:
            virtual ~DrillerDefaultStringPool()
            {
                Reset();
            }

            typedef unordered_map<AZ::u32, const char*>::type CrcToStringMapType;
            typedef unordered_set<const char*>::type OwnedStringsMapType;

            /**
             * Add a copy of the string to the pool. If we return true the string was added otherwise it was already in the bool.
             * In both cases the crc32 of the string and the pointer to the shared copy is returned (optional for poolStringAddress)!
             */
            virtual bool    InsertCopy(const char* string, unsigned int length, AZ::u32& crc32, const char** poolStringAddress = nullptr)
            {
                crc32 = AZ::Crc32(string, length);
                CrcToStringMapType::pair_iter_bool insertIt = m_crcToStringMap.insert_key(crc32);
                if (insertIt.second)
                {
                    char* newString = reinterpret_cast<char*>(azmalloc(length + 1, 1, AZ::OSAllocator));
                    memcpy(newString, string, length);
                    newString[length] = '\0'; // terminate
                    m_ownedStrings.insert(newString);
                    insertIt.first->second = newString;
                }
                if (poolStringAddress)
                {
                    *poolStringAddress = insertIt.first->second;
                }
                return insertIt.second;
            }

            /**
             * Same as the InsertCopy above without actually coping the string into the pool. The pool assumes that
             * none of the strings added to the pool will be deleted.
             */
            virtual bool    Insert(const char* string, unsigned int length, AZ::u32& crc32)
            {
                crc32 = AZ::Crc32(string, length);
                return m_crcToStringMap.insert(AZStd::make_pair(crc32, string)).second;
            }

            /// Finds a string in the pool by crc32.
            virtual const char* Find(AZ::u32 crc32)
            {
                CrcToStringMapType::iterator it = m_crcToStringMap.find(crc32);
                if (it != m_crcToStringMap.end())
                {
                    return it->second;
                }
                return NULL;
            }

            virtual void    Erase(AZ::u32 crc32)
            {
                CrcToStringMapType::iterator it = m_crcToStringMap.find(crc32);
                if (it != m_crcToStringMap.end())
                {
                    OwnedStringsMapType::iterator ownerIt = m_ownedStrings.find(it->second);
                    if (ownerIt != m_ownedStrings.end())
                    {
                        azfree(const_cast<char*>(it->second), AZ::OSAllocator);
                        m_ownedStrings.erase(ownerIt);
                    }
                    m_crcToStringMap.erase(it);
                }
            }

            virtual void    Reset()
            {
                for (OwnedStringsMapType::iterator it = m_ownedStrings.begin(); it != m_ownedStrings.end(); ++it)
                {
                    azfree(const_cast<char*>(*it), AZ::OSAllocator);
                }
                m_crcToStringMap.clear();
                m_ownedStrings.clear();
            }
        protected:
            CrcToStringMapType  m_crcToStringMap;
            OwnedStringsMapType m_ownedStrings;
        };
    } // namespace Debug
} // namespace AZ

#endif // AZCORE_DRILLER_DEFAULT_STRING_POOL_H
#pragma once


