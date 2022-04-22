/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Name/Internal/NameData.h>
#include <AzCore/std/hash.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Module/Environment.h>
#include <cstring>

namespace AZ
{
    static const char* NameDictionaryInstanceName = "NameDictionaryInstance";

    namespace NameDictionaryInternal
    {
        static AZ::EnvironmentVariable<NameDictionary> s_instance = nullptr;
    }

    void NameDictionary::Create()
    {
        using namespace NameDictionaryInternal;

        AZ_Assert(!s_instance, "NameDictionary already created!");

        if (!s_instance)
        {
            // Because the NameDictionary allocates memory using the AZ::Allocator and it is created
            // in the executable memory space, it's ownership cannot be transferred to other module memory spaces
            // Otherwise this could cause the the NameDictionary to be destroyed in static de-init
            // after the AZ::Allocators have been destroyed
            // Therefore we supply the isTransferOwnership value of false using CreateVariableEx
            s_instance = AZ::Environment::CreateVariableEx<NameDictionary>(NameDictionaryInstanceName, true, false);

            // Load any deferred names stored in our module's deferred name list, if it isn't already pointing at the dictionary's list
            // If Name::s_staticNameBegin is equal to m_deferredHead we can skip this check, as that would make the freshly created name the only
            // entry in the list.
            if (Name::s_staticNameBegin != nullptr && &s_instance->m_deferredHead != Name::s_staticNameBegin)
            {
                s_instance->m_deferredHead.m_nextName = Name::s_staticNameBegin;
                Name::s_staticNameBegin->m_previousName = &s_instance->m_deferredHead;
            }
            Name* current = s_instance->m_deferredHead.m_nextName;
            while (current != nullptr)
            {
                current->m_linkedToDictionary = true;
                current = current->m_nextName;
            }
            s_instance->LoadDeferredNames(&s_instance->m_deferredHead);
        }
    }

    void NameDictionary::Destroy()
    {
        using namespace NameDictionaryInternal;

        AZ_Assert(s_instance, "NameDictionary not created!");

        // Unload deferred names before destroying the name dictionary
        // We need to do this because they may release their NameRefs, which require s_instance to be set
        s_instance->UnloadDeferredNames();
        s_instance.Reset();
    }

    bool NameDictionary::IsReady(bool tryCreate)
    {
        using namespace NameDictionaryInternal;

        if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
        {
            return false;
        }

        if (!s_instance)
        {
            if (tryCreate)
            {
                // Because the NameDictionary allocates memory using the AZ::Allocator and it is created
                // in the executable memory space, it's ownership cannot be transferred to other module memory spaces
                // Otherwise this could cause the the NameDictionary to be destroyed in static de-init
                // after the AZ::Allocators have been destroyed
                // Therefore we supply the isTransferOwnership value of false using CreateVariableEx
                s_instance = AZ::Environment::CreateVariableEx<NameDictionary>(NameDictionaryInstanceName, true, false);
            }
            else
            {
                return false;
            }
        }

        return s_instance.IsConstructed();
    }

    NameDictionary& NameDictionary::Instance()
    {
        using namespace NameDictionaryInternal;

        if (!s_instance)
        {
            s_instance = Environment::FindVariable<NameDictionary>(NameDictionaryInstanceName);
        }

        AZ_Assert(s_instance.IsConstructed(), "NameDictionary has not been initialized yet.");

        return *s_instance;
    }

    NameDictionary::NameDictionary()
        : m_deferredHead(Name::FromStringLiteral("-fixed name dictionary deferred head-"))
    {
        // Ensure a Name that is valid for the life-cycle of this dictionary is the head of our literal linked list
        // This prevents our list head from being destroyed from a module that has shut down its AZ::Environment and
        // invalidating our list.
        m_deferredHead.m_linkedToDictionary = true;
    }
    
    NameDictionary::~NameDictionary()
    {
        // Ensure our module's static name list is up-to-date with what's in our deferred list.
        // This allows the NameDictionary to be recreated and restore and name literals that still exist
        // in e.g. unit tests.
        Name::s_staticNameBegin = m_deferredHead.m_nextName;

        [[maybe_unused]] bool leaksDetected = false;

        for (const auto& keyValue : m_dictionary)
        {
            Internal::NameData* nameData = keyValue.second;
            const int useCount = keyValue.second->m_useCount;
            [[maybe_unused]] const bool hadCollision = keyValue.second->m_hashCollision;

            if (useCount == 0)
            {
                delete nameData;
            }
            else
            {
                leaksDetected = true;
                AZ_TracePrintf("NameDictionary", "\tLeaked Name [%3d reference(s)]: hash 0x%08X, '%.*s'\n", useCount, keyValue.first, AZ_STRING_ARG(keyValue.second->GetName()));
            }
        }

        AZ_Assert(!leaksDetected, "AZ::NameDictionary still has active name references. See debug output for the list of leaked names.");
    }

    Name NameDictionary::FindName(Name::Hash hash) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);
        auto iter = m_dictionary.find(hash);
        if (iter != m_dictionary.end())
        {
            return Name(iter->second);
        }
        return Name();
    }

    void NameDictionary::LoadLiteral(Name& nameLiteral)
    {
        if (nameLiteral.m_data == nullptr)
        {
            // Load name data for the literal, but ensure its m_view is still referring to the original literal.
            Name nameData = MakeName(nameLiteral.m_view);
            nameLiteral.m_data = AZStd::move(nameData.m_data);
            nameLiteral.m_hash = nameData.m_hash;
        }
    }

    void NameDictionary::LoadDeferredName(Name& deferredName)
    {
        // Ensure this name has m_data loaded
        LoadLiteral(deferredName);

        // Link this name to the Name linked list for our module, if it isn't already.
        // This ensures that static Names are restored if the NameDictionary is ever destroyed
        // and then recreated, as the new dictionary will read from our static name list.
        if (!deferredName.m_linkedToDictionary)
        {
            deferredName.m_linkedToDictionary = true;
            // If there's an entry in our deferred list, prepend this name there so that we keep m_deferredHead unchanged
            if (m_deferredHead.m_nextName != nullptr)
            {
                deferredName.LinkStaticName(&m_deferredHead.m_nextName);
            }
            // Otherwise, simply append the name to our deferred head
            else
            {
                m_deferredHead.m_nextName = &deferredName;
                deferredName.m_previousName = &m_deferredHead;
                deferredName.m_linkedToDictionary = true;
            }
        }
    }

    void NameDictionary::LoadDeferredNames(Name* deferredHead)
    {
        if (deferredHead == nullptr)
        {
            return;
        }

        // Ensure this list entry is linked into our global name dictionary.
        LoadDeferredName(*deferredHead);

        // Ensure all Names in the list have their data loaded and are flagged as
        // linked to our static name list.
        Name* current = deferredHead->m_nextName;
        while (current != nullptr)
        {
            LoadLiteral(*current);
            current->m_linkedToDictionary = true;
            current = current->m_nextName;
        }
    }

    void NameDictionary::UnloadDeferredNames()
    {
        // Iterate through all names that still exist at static scope and clear their data
        // They'll retain m_view, so will be able to be recreated by LoadDeferredName
        Name* staticName = &m_deferredHead;
        while (staticName != nullptr)
        {
            staticName->m_data = nullptr;
            staticName->m_hash = 0;
            staticName = staticName->m_nextName;
        }
    }

    Name NameDictionary::MakeName(AZStd::string_view nameString)
    {
        // Null strings should return empty.
        if (nameString.empty())
        {
            return Name();
        }

        Name::Hash hash = CalcHash(nameString);

        // If we find the same name with the same hash, just return it. 
        // This path is faster than the loop below because FindName() takes a shared_lock whereas the
        // loop requires a unique_lock to modify the dictionary.
        Name name = FindName(hash);
        if (name.GetStringView() == nameString)
        {
            return AZStd::move(name);
        }

        // The name doesn't exist in the dictionary, so we have to lock and add it
        AZStd::unique_lock<AZStd::shared_mutex> lock(m_sharedMutex);

        auto iter = m_dictionary.find(hash);
        bool collisionDetected = false;
        while (true)
        {
            // No existing entry, add a new one and we're done
            if (iter == m_dictionary.end())
            {
                Internal::NameData* nameData = aznew Internal::NameData(nameString, hash);
                nameData->m_hashCollision = collisionDetected;
                m_dictionary.emplace(hash, nameData);
                return Name(nameData);
            }
            // Found the desired entry, return it
            else if (iter->second->GetName() == nameString)
            {
                return Name(iter->second);
            }
            // Hash collision, try a new hash
            else
            {
                collisionDetected = true;
                iter->second->m_hashCollision = true; // Make sure the existing entry is flagged as colliding too
                ++hash;
                iter = m_dictionary.find(hash);
            }
        }
    }

    void NameDictionary::TryReleaseName(Name::Hash hash)
    {
        // Note that we don't remove NameData from the dictionary if it has been involved in a collision.
        // This avoids specific edge cases where a Name object could get an incorrect hash value. Consider
        // the following scenario, supposing that "hello" and "world" hash to the to same value [1000]...
        //    - Create "hello" ... insert with hash 1000
        //    - Create "world" ... insert with hash 1001
        //    - Release "hello" ... removed and now 1000 is empty
        //    - Invoke the Name constructor by string with "world". It will hash the string to value 1000,
        //      try to find that hash in the dictionary, and nothing is found. So now "world" is added to
        //      the dictionary *again*, this time with hash value 1000. Name objects pointing to the original
        //      entry and Name objects pointing to the new entry will fail comparison operations.


        AZStd::unique_lock<AZStd::shared_mutex> lock(m_sharedMutex);

        auto dictIt = m_dictionary.find(hash);
        if (dictIt == m_dictionary.end())
        {
            // This check is to safeguard around the following scenario
            // T1, gets into TryReleaseName
            // T2 gets into MakeName, acquires the lock, returns a new Name that increments the counter
            // T2 deletes the Name decrements the counter, gets into TryReleaseName
            // T1 gets the lock, goes to the compare_exchange if and has a counter of 0, deletes
            // Then T2 continues, gets the lock and crashes because nameData was deleted
            return;
        }

        Internal::NameData* nameData = dictIt->second;

        // Check m_hashCollision inside the m_sharedMutex because a new collision could have happened
        // on another thread before taking the lock.
        if (nameData->m_hashCollision)
        {
            return;
        }

        // We need to check the count again in here in case
        // someone was trying to get the name on another thread.
        // Set it to -1 so only this thread will attempt to clean up the
        // dictionary and delete the name.
        int32_t expectedRefCount = 0;
        if (nameData->m_useCount.compare_exchange_strong(expectedRefCount, -1))
        {
            m_dictionary.erase(nameData->GetHash());
            delete nameData;
        }

        ReportStats();
    }

    void NameDictionary::ReportStats() const
    {
#ifdef AZ_DEBUG_BUILD

        // At some point it would be nice to formalize this stats output to be activated on demand by a runtime
        // feature, like a CVAR. But for now we can just activate it by breaking in the debugger.
        static bool reportUsage = false;
        if (reportUsage)
        {
            size_t potentialStringMemoryUsed = 0;
            size_t actualStringMemoryUsed = 0;

            Internal::NameData* longestName = nullptr;
            Internal::NameData* mostRepeatedName = nullptr;

            for (auto& iter : m_dictionary)
            {
                const size_t nameLength = iter.second->m_name.size();
                actualStringMemoryUsed += nameLength;
                potentialStringMemoryUsed += (nameLength * iter.second->m_useCount);

                if (!longestName || longestName->m_name.size() < nameLength)
                {
                    longestName = iter.second;
                }

                if (!mostRepeatedName)
                {
                    mostRepeatedName = iter.second;
                }
                else
                {
                    const size_t mostIndividualSavings = mostRepeatedName->m_name.size() * (mostRepeatedName->m_useCount - 1);
                    const size_t currentIndividualSavings = nameLength * (iter.second->m_useCount - 1);
                    if (currentIndividualSavings > mostIndividualSavings)
                    {
                        mostRepeatedName = iter.second;
                    }
                }
            }

            AZ_TracePrintf("NameDictionary", "NameDictionary Stats\n");
            AZ_TracePrintf("NameDictionary", "Names:              %d\n", m_dictionary.size());
            AZ_TracePrintf("NameDictionary", "Total chars:        %d\n", actualStringMemoryUsed);
            AZ_TracePrintf("NameDictionary", "Logical chars:      %d\n", potentialStringMemoryUsed);
            AZ_TracePrintf("NameDictionary", "Memory saved:       %d\n", potentialStringMemoryUsed - actualStringMemoryUsed);
            if (longestName)
            {
                AZ_TracePrintf("NameDictionary", "Longest name:       \"%.*s\"\n", aznumeric_cast<int>(longestName->m_name.size()), longestName->m_name.data());
                AZ_TracePrintf("NameDictionary", "Longest name size:  %d\n", longestName->m_name.size());
            }
            if (mostRepeatedName)
            {
                AZ_TracePrintf("NameDictionary", "Most repeated name:        \"%.*s\"\n", aznumeric_cast<int>(mostRepeatedName->m_name.size()), mostRepeatedName->m_name.data());
                AZ_TracePrintf("NameDictionary", "Most repeated name size:   %d\n", mostRepeatedName->m_name.size());
                const int refCount = mostRepeatedName->m_useCount.load();
                AZ_TracePrintf("NameDictionary", "Most repeated name count:  %d\n", refCount);
            }

            reportUsage = false;
        }

#endif // AZ_DEBUG_BUILD
    }

    Name::Hash NameDictionary::CalcHash(AZStd::string_view name)
    {
        // AZStd::hash<AZStd::string_view> returns 64 bits but we want 32 bit hashes for the sake
        // of network synchronization. So just take the low 32 bits.
        const uint32_t hash = AZStd::hash<AZStd::string_view>()(name) & 0xFFFFFFFF;
        return hash;
    }
}
