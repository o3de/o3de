/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "StandardHeaders.h"
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include "MultiThreadManager.h"

namespace AZ
{
    class ReflectContext;
}


namespace MCore
{
    class MCORE_API StringIdPool
    {
        friend class Initializer;
        friend class MCoreSystem;

    public:
        /**
         * Generate a unique id for the given string which contains the object name.
         * This method is not thread safe.
         * @param objectName The name of the node to generate an id for.
         * @return The unique id of the given object.
         */
        AZ::u32 GenerateIdForStringWithoutLock(const AZStd::string& objectName);

        /**
         * Generate a unique id for the given string which contains the object name.
         * This method is thread safe.
         * @param objectName The name of the node to generate an id for.
         * @return The unique id of the given object.
         */
        AZ::u32 GenerateIdForString(const AZStd::string& objectName);

        /**
        * Return the name of the given id.
        * @param id The unique id to search for the name.
        * @return The name of the given object.
        */
        const AZStd::string& GetName(AZ::u32 id);

        /**
         * Reserve space for a given amount of strings.
         * @param numStrings The number of strings to reserve space for.
         */
        void Reserve(size_t numStrings);

        void Log(bool includeEntries = true);

        void Clear();
        void Lock();
        void Unlock();

    private:

        AZStd::vector<AZStd::string*>                   m_strings;
        AZStd::unordered_map<AZStd::string, AZ::u32>    m_stringToIndex;         /**< The string to index table, where the index maps into m_names array and is directly the ID. */
        Mutex                                           m_mutex;                 /**< The multithread lock. */

        StringIdPool();
        ~StringIdPool();
    };

    /**
     * The StringIdPoolIndex is a helper class to aid with serialization of
     * class members that store indexes into the StringIdPool. Members of this
     * type will serialize to a string, and deserialize to a AZ::u32, while
     * allowing the StringIdPool to deduplicate the strings.
     */
    struct StringIdPoolIndex
    {
        AZ::u32 m_index{};

        operator AZ::u32() const { return m_index; }
        bool operator==(AZ::u32 rhs) const { return m_index == rhs; }

        static void Reflect(AZ::ReflectContext* context);
    };

    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_DECL(StringIdPoolIndex);

} // namespace MCore
