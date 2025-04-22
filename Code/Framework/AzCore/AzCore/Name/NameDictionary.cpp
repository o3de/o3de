/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
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
        // Pointer which indicated that the NameDictonary associated with the AZ::Interface
        // was created by the Create function below
        static AZ::EnvironmentVariable<AZStd::unique_ptr<AZ::NameDictionary>> s_staticNameDictionary;
    }

    void NameDictionary::Create()
    {
        using namespace NameDictionaryInternal;

        if (AZ::Interface<AZ::NameDictionary>::Get() != nullptr)
        {
            AZ_Warning("NameDictionary", false, "NameDictionary already registered! Must unregister existing instance to register another one");
            return;
        }

        auto nameDictionary = aznew NameDictionary();
        AZ::Interface<AZ::NameDictionary>::Register(nameDictionary);
        // Store pointer to the newly created NameDictionary
        s_staticNameDictionary = AZ::Environment::CreateVariable<AZStd::unique_ptr<AZ::NameDictionary>>(NameDictionaryInstanceName, nameDictionary);

        // Load any deferred names stored in our module's deferred name list, if it isn't already pointing at the dictionary's list
        // If Name::s_staticNameBegin is equal to m_deferredHead we can skip this check, as that would make the freshly created name the only
        // entry in the list.
        nameDictionary->LoadDeferredNames(AZ::Name::GetDeferredHead());
    }

    void NameDictionary::Destroy()
    {
        using namespace NameDictionaryInternal;

        if (s_staticNameDictionary && *s_staticNameDictionary)
        {
            // If the static name dictionary is registered with the AZ::Interface<AZ::NameDictionary> unregister it
            if (AZ::Interface<AZ::NameDictionary>::Get() == s_staticNameDictionary->get())
            {
                AZ::Interface<AZ::NameDictionary>::Unregister(s_staticNameDictionary->get());
            }

            s_staticNameDictionary = {};
        }
    }

    bool NameDictionary::IsReady()
    {
        return AZ::Interface<AZ::NameDictionary>::Get();
    }

    NameDictionary& NameDictionary::Instance()
    {
        auto nameDictionary = AZ::Interface<AZ::NameDictionary>::Get();
        AZ_Assert(nameDictionary != nullptr, "A NameDictionary has not been globally registered with the AZ::Interface<AZ::NameDictionary>.");

        return *nameDictionary;
    }

    // The deferred head passes in a nullptr NameDictionary as the nameDictionary isn't available
    // until the constructor completes
    NameDictionary::NameDictionary()
        : NameDictionary(static_cast<AZ::u64>(AZStd::numeric_limits<Name::Hash>::max()) + 1)
    {
    }

    NameDictionary::NameDictionary(AZ::u64 maxHashSlots)
        : m_deferredHead(Name::FromStringLiteral("-fixed name dictionary deferred head-", nullptr))
        , m_maxHashSlots(maxHashSlots != 0 ? maxHashSlots : static_cast<AZ::u64>(AZStd::numeric_limits<Name::Hash>::max()) + 1)
    {
        // Ensure a Name that is valid for the life-cycle of this dictionary is the head of our literal linked list
        // This prevents our list head from being destroyed from a module that has shut down its AZ::Environment and
        // invalidating our list.
        m_deferredHead.m_linkedToDictionary = true;
    }
    
    NameDictionary::~NameDictionary()
    {
        // Unload deferred names
        UnloadDeferredNames();

        // Ensure our module's static name list is up-to-date with what's in our deferred list.
        // This allows the NameDictionary to be recreated and restore and name literals that still exist
        // in e.g. unit tests.
        Name::s_staticNameBegin = m_deferredHead.m_nextName;

        [[maybe_unused]] bool leaksDetected = false;

        for (auto i = m_dictionary.begin(), last = m_dictionary.end(); i != last;)
        {
            Internal::NameData* nameData = i->second.m_nameData;
            const int useCount = nameData->m_useCount;

            if (useCount == 0)
            {
                i = m_dictionary.erase(i);
                delete nameData;
            }
            else
            {
                leaksDetected = true;
                AZ_TracePrintf("NameDictionary", "\tLeaked Name [%3d reference(s)]: hash 0x%08X, '%.*s'\n", useCount, i->first, AZ_STRING_ARG(nameData->GetName()));
                ++i;
            }
        }

        AZ_Assert(!leaksDetected, "AZ::NameDictionary still has active name references. See debug output for the list of leaked names.");
    }

    Name NameDictionary::FindName(Name::Hash hash) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);

        // The NameData m_useCount check is to avoid a multithread race condition
        // where thread B is in NameData::release and reduces the m_useCount to 0
        // and this thread(thread A) construct a Name using that NameData pointer
        // causing the m_useCount to go back up to 1.
        // If thread A continues along and releases the NameData again, before thread B can run
        // the the m_useCount can be reduced to 0 and multiple threads can be in the
        // NameData::release `if (m_useCount.fetch_sub(1) == 1)` block
        if (auto iter = m_dictionary.find(hash);
            iter != m_dictionary.end() && iter->second.m_nameData->m_useCount > 0)
        {
            return Name(iter->second.m_nameData);
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
                // Piecewise construct to prevent creating a temporary ScopedNameDataWrapper that destructs
                m_dictionary.emplace(AZStd::piecewise_construct, AZStd::forward_as_tuple(hash), AZStd::forward_as_tuple(*this, nameData));
                return Name(nameData);
            }
            // Found the desired entry, return it
            else if (iter->second.m_nameData->GetName() == nameString)
            {
                return Name(iter->second.m_nameData);
            }
            // Hash collision, try a new hash
            else
            {
                collisionDetected = true;
                iter->second.m_nameData->m_hashCollision = true; // Make sure the existing entry is flagged as colliding too
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

        Internal::NameData* nameData = dictIt->second.m_nameData;

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
                Internal::NameData* nameData = iter.second.m_nameData;
                const size_t nameLength = nameData->m_name.size();
                actualStringMemoryUsed += nameLength;
                potentialStringMemoryUsed += (nameLength * nameData->m_useCount);

                if (!longestName || longestName->m_name.size() < nameLength)
                {
                    longestName = nameData;
                }

                if (!mostRepeatedName)
                {
                    mostRepeatedName = nameData;
                }
                else
                {
                    const size_t mostIndividualSavings = mostRepeatedName->m_name.size() * (mostRepeatedName->m_useCount - 1);
                    const size_t currentIndividualSavings = nameLength * (nameData->m_useCount - 1);
                    if (currentIndividualSavings > mostIndividualSavings)
                    {
                        mostRepeatedName = nameData;
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
        return static_cast<Name::Hash>(hash % m_maxHashSlots);
    }


    // NameDictionary::ScopedNameDataWrapper RAII implementation
    NameDictionary::ScopedNameDataWrapper::ScopedNameDataWrapper(NameDictionary& nameDictionary, Internal::NameData* nameData)
        : m_nameData(nameData)
        , m_nameDictionary(nameDictionary)
    {
        if (m_nameData->m_nameDictionary == nullptr)
        {
            m_nameData->m_nameDictionary = &m_nameDictionary;
        }
    }

    NameDictionary::ScopedNameDataWrapper::ScopedNameDataWrapper(ScopedNameDataWrapper&& other)
        : m_nameData(other.m_nameData)
        , m_nameDictionary(other.m_nameDictionary)
    {
        // Reset to nullptr
        other.m_nameData = nullptr;
    }

    NameDictionary::ScopedNameDataWrapper::~ScopedNameDataWrapper()
    {
        if (m_nameData->m_nameDictionary == &m_nameDictionary)
        {
            m_nameData->m_nameDictionary = nullptr;
        }
    }
}
