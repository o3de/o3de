/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "NodeMap.h"
#include "Actor.h"
#include <EMotionFX/Source/ActorManager.h>
#include "EMotionFXManager.h"
#include "Importer/Importer.h"
#include "Importer/NodeMapFileFormat.h"
#include <MCore/Source/DiskFile.h>
#include <MCore/Source/LogManager.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(NodeMap, NodeAllocator)


    // constructor
    NodeMap::NodeMap()
        : MCore::RefCounted()
    {
        m_sourceActor            = nullptr;
    }


    // creation
    NodeMap* NodeMap::Create()
    {
        return aznew NodeMap();
    }


    // preallocate space
    void NodeMap::Reserve(size_t numEntries)
    {
        m_entries.reserve(numEntries);
    }


    // resize the entries array
    void NodeMap::Resize(size_t numEntries)
    {
        m_entries.resize(numEntries);
    }


    // modify the first name of a given entry
    void NodeMap::SetFirstName(size_t entryIndex, const char* name)
    {
        m_entries[entryIndex].m_firstNameId = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // modify the second name
    void NodeMap::SetSecondName(size_t entryIndex, const char* name)
    {
        m_entries[entryIndex].m_secondNameId = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // modify a given entry
    void NodeMap::SetEntry(size_t entryIndex, const char* firstName, const char* secondName)
    {
        m_entries[entryIndex].m_firstNameId = MCore::GetStringIdPool().GenerateIdForString(firstName);
        m_entries[entryIndex].m_secondNameId = MCore::GetStringIdPool().GenerateIdForString(secondName);
    }


    // set the entry of the firstName item to the second name
    void NodeMap::SetEntry(const char* firstName, const char* secondName, bool addIfNotExists)
    {
        // check if there is already an entry for this name
        const size_t entryIndex = FindEntryIndexByName(firstName);
        if (entryIndex == InvalidIndex)
        {
            // if there is no such entry yet, and we also don't want to add a new one, then there is nothing to do
            if (addIfNotExists == false)
            {
                return;
            }

            // add a new entry
            AddEntry(firstName, secondName);
            return;
        }

        // modify it
        SetSecondName(entryIndex, secondName);
    }


    // add an entry
    void NodeMap::AddEntry(const char* firstName, const char* secondName)
    {
        MCORE_ASSERT(GetHasEntry(firstName) == false);  // prevent duplicates
        m_entries.emplace_back();
        SetEntry(m_entries.size() - 1, firstName, secondName);
    }


    // remove a given entry by its index
    void NodeMap::RemoveEntryByIndex(size_t entryIndex)
    {
        m_entries.erase(AZStd::next(begin(m_entries), entryIndex));
    }


    // remove a given entry by its name
    void NodeMap::RemoveEntryByName(const char* firstName)
    {
        const size_t entryIndex = FindEntryIndexByName(firstName);
        if (entryIndex == InvalidIndex)
        {
            return;
        }

        m_entries.erase(AZStd::next(begin(m_entries), entryIndex));
    }


    // remove a given entry by its name ID
    void NodeMap::RemoveEntryByNameID(uint32 firstNameID)
    {
        const size_t entryIndex = FindEntryIndexByNameID(firstNameID);
        if (entryIndex == InvalidIndex)
        {
            return;
        }

        m_entries.erase(AZStd::next(begin(m_entries), entryIndex));
    }


    // set the filename
    void NodeMap::SetFileName(const char* fileName)
    {
        m_fileName = fileName;
    }


    // get the filename
    const char* NodeMap::GetFileName() const
    {
        return m_fileName.c_str();
    }


    // get the filename
    const AZStd::string& NodeMap::GetFileNameString() const
    {
        return m_fileName;
    }


    // try to write a string to the file
    bool NodeMap::WriteFileString(MCore::DiskFile* f, const AZStd::string& textToSave, MCore::Endian::EEndianType targetEndianType) const
    {
        MCORE_ASSERT(f);

        // convert endian and write the number of characters that follow
        if (textToSave.empty())
        {
            uint32 numCharacters = 0;
            MCore::Endian::ConvertUnsignedInt32To(&numCharacters, targetEndianType);
            if (f->Write(&numCharacters, sizeof(uint32)) == 0)
            {
                return false;
            }

            return true;
        }
        else
        {
            uint32 numCharacters = static_cast<uint32>(textToSave.size());
            MCore::Endian::ConvertUnsignedInt32To(&numCharacters, targetEndianType);
            if (f->Write(&numCharacters, sizeof(uint32)) == 0)
            {
                return false;
            }
        }

        // write the string in UTF8 format
        if (f->Write(textToSave.c_str(), textToSave.size()) == 0)
        {
            return false;
        }

        return true;
    }


    // calculate the size of a string on disk
    uint32 NodeMap::CalcFileStringSize(const AZStd::string& text) const
    {
        return static_cast<uint32>(sizeof(uint32) + text.size() * sizeof(char));
    }


    // calculate the chunk size
    uint32 NodeMap::CalcFileChunkSize() const
    {
        // add the node map info header
        size_t numBytes = sizeof(FileFormat::NodeMapChunk);

        // for all entries
        const size_t numEntries = m_entries.size();
        for (size_t i = 0; i < numEntries; ++i)
        {
            numBytes += CalcFileStringSize(GetFirstNameString(i));
            numBytes += CalcFileStringSize(GetSecondNameString(i));
        }

        // return the number of bytes
        return aznumeric_caster(numBytes);
    }


    // build a node map
    bool NodeMap::Save(const char* fileName, MCore::Endian::EEndianType targetEndianType) const
    {
        // try to create the file
        MCore::DiskFile f;
        if (f.Open(fileName, MCore::DiskFile::WRITE) == false)
        {
            MCore::LogError("NodeMap::Save() - Cannot write to file '%s', is the file maybe in use by another application?", fileName);
            return false;
        }

        // try to write the file header
        FileFormat::NodeMap_Header header{};
        header.m_fourCc[0] = 'N';
        header.m_fourCc[1] = 'O';
        header.m_fourCc[2] = 'M';
        header.m_fourCc[3] = 'P';
        header.m_hiVersion   = 1;
        header.m_loVersion   = 0;
        header.m_endianType  = (uint8)targetEndianType;
        if (f.Write(&header, sizeof(FileFormat::NodeMap_Header)) == 0)
        {
            MCore::LogError("NodeMap::Save() - Cannot write the header to file '%s', is the file maybe in use by another application?", fileName);
            return false;
        }

        // write the chunk header
        FileFormat::FileChunk chunkHeader{};
        chunkHeader.m_chunkId        = FileFormat::CHUNK_NODEMAP;
        chunkHeader.m_version        = 1;
        chunkHeader.m_sizeInBytes    = CalcFileChunkSize();// calculate the chunk size
        MCore::Endian::ConvertUnsignedInt32To(&chunkHeader.m_chunkId,       targetEndianType);
        MCore::Endian::ConvertUnsignedInt32To(&chunkHeader.m_sizeInBytes,   targetEndianType);
        MCore::Endian::ConvertUnsignedInt32To(&chunkHeader.m_version,       targetEndianType);
        if (f.Write(&chunkHeader, sizeof(FileFormat::FileChunk)) == 0)
        {
            MCore::LogError("NodeMap::Save() - Cannot write the chunk header to file '%s', is the file maybe in use by another application?", fileName);
            return false;
        }

        // the main info
        FileFormat::NodeMapChunk nodeMapChunk{};
        nodeMapChunk.m_numEntries = aznumeric_caster(m_entries.size());
        MCore::Endian::ConvertUnsignedInt32To(&nodeMapChunk.m_numEntries, targetEndianType);
        if (f.Write(&nodeMapChunk, sizeof(FileFormat::NodeMapChunk)) == 0)
        {
            MCore::LogError("NodeMap::Save() - Cannot write the node map chunk to file '%s', is the file maybe in use by another application?", fileName);
            return false;
        }

        // fill in the source actor string placeholder. This field was
        // deprecated but kept for backwards compatibility of loading old
        // files.
        if (WriteFileString(&f, "", targetEndianType) == false)
        {
            return false;
        }

        // for all entries
        const uint32 numEntries = aznumeric_caster(m_entries.size());
        for (uint32 i = 0; i < numEntries; ++i)
        {
            if (WriteFileString(&f, GetFirstNameString(i), targetEndianType) == false)
            {
                MCore::LogError("NodeMap::Save() - Cannot write the first string to file '%s', is the file maybe in use by another application?", fileName);
                return false;
            }

            // write the second string
            if (WriteFileString(&f, GetSecondNameString(i), targetEndianType) == false)
            {
                MCore::LogError("NodeMap::Save() - Cannot write the second string to file '%s', is the file maybe in use by another application?", fileName);
                return false;
            }
        }

        // success
        f.Close();
        return true;
    }


    // update the source actor pointer
    void NodeMap::SetSourceActor(Actor* actor)
    {
        m_sourceActor = actor;
    }


    // get the source actor pointer
    Actor* NodeMap::GetSourceActor() const
    {
        return m_sourceActor;
    }


    // get the number of entries
    size_t NodeMap::GetNumEntries() const
    {
        return m_entries.size();
    }


    // get the first name as char pointer
    const char* NodeMap::GetFirstName(size_t entryIndex) const
    {
        return MCore::GetStringIdPool().GetName(m_entries[entryIndex].m_firstNameId).c_str();
    }


    // get the second node name as char pointer
    const char* NodeMap::GetSecondName(size_t entryIndex) const
    {
        return MCore::GetStringIdPool().GetName(m_entries[entryIndex].m_secondNameId).c_str();
    }


    // get the first node name as string
    const AZStd::string& NodeMap::GetFirstNameString(size_t entryIndex) const
    {
        return MCore::GetStringIdPool().GetName(m_entries[entryIndex].m_firstNameId);
    }


    // get the second node name as string
    const AZStd::string& NodeMap::GetSecondNameString(size_t entryIndex) const
    {
        return MCore::GetStringIdPool().GetName(m_entries[entryIndex].m_secondNameId);
    }


    // check if we already have an entry for this name
    bool NodeMap::GetHasEntry(const char* firstName) const
    {
        return (FindEntryIndexByName(firstName) != InvalidIndex);
    }


    // find an entry index by its name
    size_t NodeMap::FindEntryIndexByName(const char* firstName) const
    {
        const auto foundEntry = AZStd::find_if(begin(m_entries), end(m_entries), [firstName](const MapEntry& entry)
        {
            return MCore::GetStringIdPool().GetName(entry.m_firstNameId) == firstName;
        });
        return foundEntry != end(m_entries) ? AZStd::distance(begin(m_entries), foundEntry) : InvalidIndex;
    }


    // find an entry index by its name ID
    size_t NodeMap::FindEntryIndexByNameID(uint32 firstNameID) const
    {
        const auto foundEntry = AZStd::find_if(begin(m_entries), end(m_entries), [firstNameID](const MapEntry& entry)
        {
            return entry.m_firstNameId == firstNameID;
        });
        return foundEntry != end(m_entries) ? AZStd::distance(begin(m_entries), foundEntry) : InvalidIndex;
    }


    // find the second name for a given first name
    const char* NodeMap::FindSecondName(const char* firstName) const
    {
        const size_t entryIndex = FindEntryIndexByName(firstName);
        if (entryIndex == InvalidIndex)
        {
            return nullptr;
        }

        return GetSecondName(entryIndex);
    }


    // find the second name based on a first given name
    void NodeMap::FindSecondName(const char* firstName, AZStd::string* outString)
    {
        const size_t entryIndex = FindEntryIndexByName(firstName);
        if (entryIndex == InvalidIndex)
        {
            outString->clear();
            return;
        }

        *outString = GetSecondNameString(entryIndex);
    }
}   // namespace EMotionFX
