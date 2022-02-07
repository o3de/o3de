/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzCore/std/numeric.h>
#include <AzCore/Debug/Timer.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <MCore/Source/DiskFile.h>
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/LogManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionSet, MotionAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(MotionSet::MotionEntry, MotionAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(MotionSetCallback, MotionAllocator, 0)


    MotionSetCallback::MotionSetCallback()
        : m_motionSet(nullptr)
    {
    }


    MotionSetCallback::MotionSetCallback(MotionSet* motionSet)
        : m_motionSet(motionSet)
    {
    }


    MotionSetCallback::~MotionSetCallback()
    {
    }


    Motion* MotionSetCallback::LoadMotion(MotionSet::MotionEntry* entry)
    {
        AZ_Assert(m_motionSet, "Motion set is nullptr.");

        // Get the full file name and file extension.
        const AZStd::string filename = m_motionSet->ConstructMotionFilename(entry);

        // Check what type of file to load.
        Motion* motion = GetImporter().LoadMotion(filename.c_str(), nullptr);

        // Use the filename without path as motion name.
        if (motion)
        {
            AZStd::string motionName;
            AzFramework::StringFunc::Path::GetFileName(filename.c_str(), motionName);
            motion->SetName(motionName.c_str());
        }

        return motion;
    }

    //--------------------------------------------------------------------------------------------------------

    MotionSet::MotionEntry::MotionEntry()
        : m_motion(nullptr)
        , m_loadFailed(false)
    {
    }


    MotionSet::MotionEntry::MotionEntry(const char* fileName, const AZStd::string& motionId, Motion* motion)
        : m_id(motionId)
        , m_filename(fileName)
        , m_motion(motion)
        , m_loadFailed(false)
    {
    }


    MotionSet::MotionEntry::~MotionEntry()
    {
        // if the motion is owned by the runtime, is going to be deleted by the asset system
        if (m_motion && !m_motion->GetIsOwnedByRuntime())
        {
            m_motion->Destroy();
        }
    }


    const AZStd::string& MotionSet::MotionEntry::GetId() const
    {
        return m_id;
    }


    void MotionSet::MotionEntry::SetId(const AZStd::string& id)
    {
        m_id = id;
    }


    // Check if the given filename is absolute or relative.
    bool MotionSet::MotionEntry::CheckIfIsAbsoluteFilename(const AZStd::string& filename)
    {
        // In the following cases the filename is absolute.
        if (AZ::IO::PathView(filename).IsAbsolute())
        {
            // Absolute filename.
            return true;
        }

        // Relative filename.
        return false;
    }


    // Check if the motion entry filename is absolute or relative.
    bool MotionSet::MotionEntry::CheckIfIsAbsoluteFilename() const
    {
        return CheckIfIsAbsoluteFilename(m_filename);
    }


    void MotionSet::MotionEntry::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionEntry>()
            ->Version(1)
            ->Field("id", &MotionEntry::m_id)
            ->Field("assetId", &MotionEntry::m_filename);
    }

    //----------------------------------------------------------------------------------------

    MotionSet::MotionSet()
        : m_parentSet(nullptr)
        , m_autoUnregister(true)
        , m_dirtyFlag(false)
    {
        m_id                 = aznumeric_caster(MCore::GetIDGenerator().GenerateID());
        m_callback           = aznew MotionSetCallback(this);

#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByRuntime   = false;
        m_isOwnedByAsset     = false;
#endif // EMFX_DEVELOPMENT_BUILD

        // Automatically register the motion set.
        GetMotionManager().AddMotionSet(this);

        GetEventManager().OnCreateMotionSet(this);
    }

    MotionSet::MotionSet(const char* name, MotionSet* parent)
        : MotionSet()
    {
        m_parentSet = parent;
        SetName(name);
    }


    MotionSet::~MotionSet()
    {
        GetEventManager().OnDeleteMotionSet(this);

        // Automatically unregister the motion set.
        if (m_autoUnregister)
        {
            GetMotionManager().Lock();
            GetMotionManager().RemoveMotionSet(this, false);
            GetMotionManager().Unlock();
        }

        // Remove all motion entries from the set.
        Clear();

        // Remove the callback.
        if (m_callback)
        {
            delete m_callback;
        }

        // Get rid of the child motion sets.
        // Can't use a range for loop here, because deleting the child
        // MotionSet also erases the child set from m_childSet, which
        // invalidates the iterators
        // Delete back to front to avoid shifting the elements in m_childSets
        // every time a child is erased
        const size_t numChildSets = m_childSets.size();
        for (size_t i = numChildSets; i > 0; --i)
        {
            MotionSet* childSet = m_childSets[i-1];
            if (!childSet->GetIsOwnedByRuntime() && !childSet->GetIsOwnedByAsset())
            {
                delete childSet;
            }
        }
        m_childSets.clear();
    }


    // Constuct a full valid path for a given motion file.
    AZStd::string MotionSet::ConstructMotionFilename(const MotionEntry* motionEntry)
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Is the motion entry filename empty?
        const AZStd::string& motionEntryFilename = motionEntry->GetFilenameString();
        if (motionEntryFilename.empty())
        {
            return {};
        }

        // Check if the motion entry stores an absolute path and use that directly in this case.
        if (motionEntry->CheckIfIsAbsoluteFilename())
        {
            return motionEntryFilename;
        }

        // Check if the motion entry start with an alias and resolve the path in that case.
        if (motionEntryFilename.starts_with('@'))
        {
            return EMotionFX::EMotionFXManager::ResolvePath(motionEntryFilename.c_str());
        }

        AZStd::string mediaRootFolder = GetEMotionFX().GetMediaRootFolder();
        AZ_Error("MotionSet", !mediaRootFolder.empty(), "No media root folder set. Cannot load file for motion entry '%s'.", motionEntry->GetFilename());

        // Construct the absolute path based on the current media root folder and the relative filename of the motion entry.
        return mediaRootFolder + motionEntry->GetFilename();
    }


    // Remove all motion entries from the set.
    void MotionSet::Clear()
    {
        MCore::LockGuardRecursive lock(m_mutex);

        for (const auto& item : m_motionEntries)
        {
            delete item.second;
        }

        m_motionEntries.clear();
    }


    void MotionSet::AddMotionEntry(MotionEntry* motionEntry)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_motionEntries.insert(AZStd::make_pair<AZStd::string, MotionEntry*>(motionEntry->GetId(), motionEntry));
    }


    // Add a motion set as child set.
    void MotionSet::AddChildSet(MotionSet* motionSet)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_childSets.push_back(motionSet);
    }


    // Build a list of unique string id values from all motion set entries.
    void MotionSet::BuildIdStringList(AZStd::vector<AZStd::string>& idStrings) const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Iterate through all entries and add their unique id strings to the given list.
        const size_t numMotionEntries = m_motionEntries.size();
        idStrings.reserve(numMotionEntries);

        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;

            const AZStd::string& idString = motionEntry->GetId();
            if (idString.empty())
            {
                continue;
            }

            idStrings.push_back(idString);
        }
    }


    const MotionSet::MotionEntries& MotionSet::GetMotionEntries() const
    {
        return m_motionEntries;
    }

    void MotionSet::RecursiveGetMotions(AZStd::unordered_set<Motion*>& childMotions) const
    {
        if (GetIsOwnedByRuntime())
        {
            return;
        }
        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;
            childMotions.insert(motionEntry->GetMotion());
        }
        for (const MotionSet* childMotionSet : m_childSets)
        {
            childMotionSet->RecursiveGetMotions(childMotions);
        }
    }

    void MotionSet::ReserveMotionEntries(size_t numMotionEntries)
    {
        MCore::LockGuardRecursive lock(m_mutex);

        m_motionEntries.reserve(numMotionEntries);
    }


    // Find the motion entry for a given motion.
    MotionSet::MotionEntry* MotionSet::FindMotionEntry(const Motion* motion) const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;

            if (motionEntry->GetMotion() == motion)
            {
                return motionEntry;
            }
        }

        return nullptr;
    }


    void MotionSet::RemoveMotionEntry(MotionEntry* motionEntry)
    {
        MCore::LockGuardRecursive lock(m_mutex);

        m_motionEntries.erase(motionEntry->GetId());
        delete motionEntry;
    }


    // Remove child set with the given id from the motion set.
    void MotionSet::RemoveChildSetByID(uint32 childSetID)
    {
        MCore::LockGuardRecursive lock(m_mutex);

        const size_t numChildSets = m_childSets.size();
        for (size_t i = 0; i < numChildSets; ++i)
        {
            // Compare the ids and remove the child if we found it.
            if (m_childSets[i]->GetID() == childSetID)
            {
                m_childSets.erase(m_childSets.begin() + i);
                return;
            }
        }
    }


    // Recursively find the root motion set.
    MotionSet* MotionSet::FindRootMotionSet() const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Are we dealing with a root motion set already?
        if (!m_parentSet)
        {
            return const_cast<MotionSet*>(this);
        }

        return m_parentSet->FindRootMotionSet();
    }


    // Find the motion entry by a given string.
    MotionSet::MotionEntry* MotionSet::FindMotionEntryById(const AZStd::string& motionId) const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        const auto iterator = m_motionEntries.find(motionId);
        if (iterator == m_motionEntries.end())
        {
            return nullptr;
        }

        return iterator->second;
    }


    // Find the motion entry by string ID.
    MotionSet::MotionEntry* MotionSet::RecursiveFindMotionEntryById(const AZStd::string& motionId) const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Is there a motion entry with the given id in the current motion set?
        MotionEntry* entry = FindMotionEntryById(motionId);
        if (entry)
        {
            return entry;
        }

        // Recursively ask the parent motion sets in case we haven't found the motion id in the current motion set.
        if (m_parentSet)
        {
            return m_parentSet->FindMotionEntryById(motionId);
        }

        // Motion id not found.
        return nullptr;
    }



    // Find motion by string ID.
    Motion* MotionSet::RecursiveFindMotionById(const AZStd::string& motionId, bool loadOnDemand) const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        MotionEntry* entry = RecursiveFindMotionEntryById(motionId);
        if (!entry)
        {
            return nullptr;
        }

        Motion* motion = entry->GetMotion();
        if (loadOnDemand)
        {
            motion = LoadMotion(entry);
        }
        /*
            if (motion == nullptr && loadOnDemand && entry->GetLoadingFailed() == false)    // if loading on demand is enabled and the motion hasn't loaded yet
            {
                // TODO: and replace this with a loading callback

                // get the full file name and file extension
                const AZStd::string fileName  = ConstructMotionFilename( entry );
                const AZStd::string extension = fileName.ExtractFileExtension();

                // check what type of file to load
                if (AzFramework::StringFunc::Equal(extension.c_str(), "motion", false))
                    motion = (Motion*)GetImporter().LoadSkeletalMotion( fileName.AsChar(), nullptr );   // TODO: what about motion load settings?
                else
                if (AzFramework::StringFunc::Equal(extension.c_str(), "xpm", false))
                    motion = (Motion*)GetImporter().LoadMorphMotion( fileName.AsChar(), nullptr );  // TODO: same here, what about the importer loading settings
                else
                    AZ_Warning("EMotionFX", false, "MotionSet - Loading on demand of motion '%s' (id=%s) failed as the file extension isn't known.", fileName.AsChar(), stringID);

                // mark that we already tried to load this motion, so that we don't retry this next time
                if (motion == nullptr)
                    entry->SetLoadingFailed( true );
                else
                    motion->SetName( fileName.AsChar() );

                entry->SetMotion( motion );
            }
        */
        return motion;
    }


    MotionSet* MotionSet::RecursiveFindMotionSetByName(const AZStd::string& motionSetName, bool isOwnedByRuntime) const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        
        if (GetIsOwnedByRuntime() == isOwnedByRuntime && m_name == motionSetName)
        {
            return const_cast<MotionSet*>(this);
        }
        MotionSet* motionSet = nullptr;
        for (const MotionSet* childMotionSet : m_childSets)
        {
            motionSet = childMotionSet->RecursiveFindMotionSetByName(motionSetName, isOwnedByRuntime);
            if (motionSet)
            {
                return motionSet;
            }
        }
        return motionSet;
    }


    void MotionSet::SetMotionEntryId(MotionEntry* motionEntry, const AZStd::string& newMotionId)
    {
        const AZStd::string oldStringId = motionEntry->GetId();

        motionEntry->SetId(newMotionId);

        m_motionEntries.erase(oldStringId);
        m_motionEntries[newMotionId] = motionEntry;
    }


    // Adjust the dirty flag.
    void MotionSet::SetDirtyFlag(bool dirty)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_dirtyFlag = dirty;
    }


    Motion* MotionSet::LoadMotion(MotionEntry* entry) const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        Motion* motion = entry->GetMotion();

        // If loading on demand is enabled and the motion hasn't loaded yet.
        if (!motion && !entry->GetFilenameString().empty() && !entry->GetLoadingFailed())
        {
            motion = m_callback->LoadMotion(entry);

            // Mark that we already tried to load this motion, so that we don't retry this next time.
            if (!motion)
            {
                entry->SetLoadingFailed(true);
            }

            entry->SetMotion(motion);
        }

        return motion;
    }


    // Pre-load all motions.
    void MotionSet::Preload()
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Pre-load motions for all motion entries.
        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;

            if (motionEntry->GetFilenameString().empty())
            {
                continue;
            }

            LoadMotion(motionEntry);
        }

        // Pre-load all child sets.
        for (MotionSet* childSet : m_childSets)
        {
            childSet->Preload();
        }
    }


    // Reload all motions.
    void MotionSet::Reload()
    {
        MCore::LockGuardRecursive lock(m_mutex);

        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;
            motionEntry->Reset();
        }

        // Pre-load the motions again.
        Preload();
    }


    // Adjust the auto unregistering from the motion manager on delete.
    void MotionSet::SetAutoUnregister(bool enabled)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_autoUnregister = enabled;
    }


    // Do we auto unregister from the motion manager on delete?
    bool MotionSet::GetAutoUnregister() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_autoUnregister;
    }


    void MotionSet::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByRuntime = isOwnedByRuntime;
#else
        AZ_UNUSED(isOwnedByRuntime);
#endif
    }


    bool MotionSet::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return m_isOwnedByRuntime;
#else
        return true;
#endif
    }


    void MotionSet::SetIsOwnedByAsset(bool isOwnedByAsset)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByAsset = isOwnedByAsset;
#else
        AZ_UNUSED(isOwnedByAsset);
#endif
    }


    bool MotionSet::GetIsOwnedByAsset() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return m_isOwnedByAsset;
#else
        return true;
#endif
    }


    bool MotionSet::GetDirtyFlag() const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        return m_dirtyFlag || AZStd::any_of(begin(m_childSets), end(m_childSets), [](const MotionSet* childSet)
        {
            return childSet->GetDirtyFlag();
        });
    }


    void MotionSet::SetCallback(MotionSetCallback* callback, bool delExistingOneFromMem)
    {
        MCore::LockGuardRecursive lock(m_mutex);

        if (delExistingOneFromMem && callback)
        {
            delete m_callback;
        }

        m_callback = callback;
        if (callback)
        {
            callback->SetMotionSet(this);
        }
    }


    void MotionSet::SetID(uint32 id)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_id = id;
    }


    uint32 MotionSet::GetID() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_id;
    }


    void MotionSet::SetName(const char* name)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_name = name;
    }


    const char* MotionSet::GetName() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_name.c_str();
    }


    const AZStd::string& MotionSet::GetNameString() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_name;
    }


    void MotionSet::SetFilename(const char* filename)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_filename = filename;
    }


    const char* MotionSet::GetFilename() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_filename.c_str();
    }


    const AZStd::string& MotionSet::GetFilenameString() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_filename;
    }


    size_t MotionSet::GetNumChildSets() const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        size_t result = 0;
        for (const MotionSet* motionSet : m_childSets)
        {
            result += !motionSet->GetIsOwnedByRuntime();
        }
        return result;
    }


    MotionSet* MotionSet::GetChildSet(size_t index) const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        const auto foundChildSet = AZStd::find_if(begin(m_childSets), end(m_childSets), [iter = index](const MotionSet* motionSet) mutable
        {
            return !motionSet->GetIsOwnedByRuntime() && iter-- == 0;
        });
        return foundChildSet != end(m_childSets) ? *foundChildSet : nullptr;
    }

    void MotionSet::RecursiveGetMotionSets(AZStd::vector<const MotionSet*>& childMotionSets, bool isOwnedByRuntime) const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        if (GetIsOwnedByRuntime() == isOwnedByRuntime)
        {
            childMotionSets.push_back(this);
            for (MotionSet* childMotion : m_childSets)
            {
                childMotion->RecursiveGetMotionSets(childMotionSets, isOwnedByRuntime);
            }
        }
    }

    MotionSet* MotionSet::GetParentSet() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_parentSet;
    }


    MotionSetCallback* MotionSet::GetCallback() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_callback;
    }


    void MotionSet::Log()
    {
        AZ_Printf("EMotionFX", " - MotionSet");
        AZ_Printf("EMotionFX", "     + Name = '%s'", m_name.c_str());
        AZ_Printf("EMotionFX", "     - Entries (%d)", GetNumMotionEntries());

        int nr = 0;
        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;
            AZ_Printf("EMotionFX", "         + #%d: Id='%s', Filename='%s'", nr, motionEntry->GetId().c_str(), motionEntry->GetFilename());
            nr++;
        }
    }


    size_t MotionSet::GetNumMorphMotions() const
    {
        size_t numMorphMotions = 0;
        const MotionEntries& motionEntries = GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            const MotionEntry* motionEntry = item.second;
            const Motion* motion = motionEntry->GetMotion();           
            if (motion)
            {
                if (motion->GetMotionData()->GetNumMorphs() > 0)
                {
                    numMorphMotions++;
                }
            }
        }
        return numMorphMotions;
    }


    void MotionSet::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionSet>()
            ->Version(1)
            ->Field("name", &MotionSet::m_name)
            ->Field("motionEntries", &MotionSet::m_motionEntries)
            ->Field("childSets", &MotionSet::m_childSets);
    }


    MotionSet* MotionSet::LoadFromFile(const AZStd::string& filename, AZ::SerializeContext* context, const AZ::ObjectStream::FilterDescriptor& loadFilter)
    {
        AZ::Debug::Timer loadTimer;
        loadTimer.Stamp();

        MotionSet* result = AZ::Utils::LoadObjectFromFile<MotionSet>(filename, context, loadFilter);
        if (result)
        {
            result->InitAfterLoading();

            const float loadTimeInMs = loadTimer.GetDeltaTimeInSeconds() * 1000.0f;
            AZ_Printf("EMotionFX", "Loaded motion set from %s in %.1f ms.", filename.c_str(), loadTimeInMs);
        }
        
        return result;
    }


    MotionSet* MotionSet::LoadFromBuffer(const void* buffer, const AZStd::size_t length, AZ::SerializeContext* context)
    {
        AZ::Debug::Timer loadTimer;
        loadTimer.Stamp();

        MotionSet* result = AZ::Utils::LoadObjectFromBuffer<MotionSet>(buffer, length, context);

        if (result)
        {
            result->InitAfterLoading();

            const float loadTimeInMs = loadTimer.GetDeltaTimeInSeconds() * 1000.0f;
            AZ_Printf("EMotionFX", "Loaded motion set from buffer in %.1f ms.", loadTimeInMs);
        }

        return result;
    }


    bool MotionSet::SaveToFile(const AZStd::string& filename, AZ::SerializeContext* context) const
    {
        AZ::Debug::Timer saveTimer;
        saveTimer.Stamp();

        const bool result = AZ::Utils::SaveObjectToFile<MotionSet>(filename, AZ::ObjectStream::ST_XML, this, context);
        if (result)
        {
            const float saveTimeInMs = saveTimer.GetDeltaTimeInSeconds() * 1000.0f;
            AZ_Printf("EMotionFX", "Saved motion set to %s in %.1f ms.", filename.c_str(), saveTimeInMs);
        }

        return result;
    }


    void MotionSet::RecursiveRewireParentSets(MotionSet* motionSet)
    {
        const size_t numChildSets = motionSet->GetNumChildSets();
        for (size_t i = 0; i < numChildSets; ++i)
        {
            MotionSet* childSet = motionSet->GetChildSet(i);
            childSet->m_parentSet = motionSet;
            RecursiveRewireParentSets(childSet);
        }
    }


    void MotionSet::InitAfterLoading()
    {
        RecursiveRewireParentSets(this);
    }
} // namespace EMotionFX
