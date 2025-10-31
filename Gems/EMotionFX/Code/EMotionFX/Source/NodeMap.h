/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required files
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StringIdPool.h>
#include <MCore/Source/Endian.h>

// MCore forward declarations
MCORE_FORWARD_DECLARE(DiskFile);


namespace EMotionFX
{
    // forward declarations
    class Actor;

    /**
     * A node map maps a set of nodes from one given node name to another.
     * This can create a linkage/mapping between two different hierarchies. For example the node "Bip01 L Head" in one Actor could be mapped to
     * a node named "Head" in another Actor. A node map is for example used during retargeting of motions who's hierarchies are different.
     * The first name is always the key, and the second name is the name it is mapped to.
     * You should not have any duplicated firstName entries. The multiple entries can have the same second name though.
     */
    class EMFX_API NodeMap
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        struct MapEntry
        {
            uint32 m_firstNameId = InvalidIndex32;   /**< The first name ID, which is the primary key in the map. */
            uint32 m_secondNameId = InvalidIndex32;  /**< The second name ID. */
        };

        static NodeMap* Create();

        // prealloc space in the map
        void Reserve(size_t numEntries);
        void Resize(size_t numEntries);

        // get data
        size_t GetNumEntries() const;
        const char* GetFirstName(size_t entryIndex) const;
        const char* GetSecondName(size_t entryIndex) const;
        const AZStd::string& GetFirstNameString(size_t entryIndex) const;
        const AZStd::string& GetSecondNameString(size_t entryIndex) const;
        bool GetHasEntry(const char* firstName) const;
        size_t FindEntryIndexByName(const char* firstName) const;
        size_t FindEntryIndexByNameID(uint32 firstNameID) const;
        const char* FindSecondName(const char* firstName) const;
        void FindSecondName(const char* firstName, AZStd::string* outString);

        // set/modify
        void SetFirstName(size_t entryIndex, const char* name);
        void SetSecondName(size_t entryIndex, const char* name);
        void SetEntry(size_t entryIndex, const char* firstName, const char* secondName);
        void AddEntry(const char* firstName, const char* secondName);
        void SetEntry(const char* firstName, const char* secondName, bool addIfNotExists);
        void RemoveEntryByIndex(size_t entryIndex);
        void RemoveEntryByName(const char* firstName);
        void RemoveEntryByNameID(uint32 firstNameID);

        // filename
        void SetFileName(const char* fileName);
        const char* GetFileName() const;
        const AZStd::string& GetFileNameString() const;

        // source actor
        void SetSourceActor(Actor* actor);
        Actor* GetSourceActor() const;

        // saving
        bool Save(const char* fileName, MCore::Endian::EEndianType targetEndianType) const;

    private:
        AZStd::vector<MapEntry>  m_entries;                   /**< The array of entries. */
        AZStd::string           m_fileName;                  /**< The filename. */
        Actor*                  m_sourceActor;               /**< The source actor. */

        // constructor and destructor
        NodeMap();
        ~NodeMap() = default;

        // file saving
        bool WriteFileString(MCore::DiskFile* f, const AZStd::string& textToSave, MCore::Endian::EEndianType targetEndianType) const;
        uint32 CalcFileChunkSize() const;
        uint32 CalcFileStringSize(const AZStd::string& text) const;
    };
}   // namespace EMotionFX
