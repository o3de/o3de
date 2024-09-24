/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <MCore/Source/StringIdPool.h>


namespace EMotionFX
{
    class Motion;
    class MotionSetCallback;

    /**
     *
     */
    class EMFX_API MotionSet
    {
    public:
        AZ_RTTI(MotionSet, "{FE63321D-3593-4214-AFA4-F620CDC17B9B}")
        AZ_CLASS_ALLOCATOR_DECL

        class EMFX_API MotionEntry
        {
            friend class MotionSet;
        public:
            AZ_RTTI(MotionSet::MotionEntry, "{A1859687-2DE7-4B29-8A39-07836FDF5956}");
            AZ_CLASS_ALLOCATOR_DECL

            MotionEntry();

            /**
             * Constructor.
             * @param[in] filename The local filename of the motion.
             * @param[in] motionId The motion name.
             * @param[in] motion A pointer to the motion.
             */
            MotionEntry(const char* filename, const AZStd::string& motionId, Motion* motion = nullptr);

            virtual ~MotionEntry();

            /**
             * Set the filename of the motion.
             * @param[in] filename The filename of the motion.
             */
            void SetFilename(const char* filename)                                                              { m_filename = filename; }

            /**
             * Get the filename of the motion.
             * @result The filename of the motion.
             */
            const char* GetFilename() const                                                                     { return m_filename.c_str(); }

            /**
             * Returns the filename of the motion, as a string object.
             * @result The filename of the motion.
             */
            const AZStd::string& GetFilenameString() const                                                      { return m_filename; }

            /**
             * Get the motion for this entry.
             * @result The motion pointer.
             */
            Motion* GetMotion() const                                                                           { return m_motion; }

            /**
             * Set the motion pointer.
             * @param[in] motion The motion to use for this entry.
             */
            void SetMotion(Motion* motion)                                                                      { m_motion = motion; }

            /**
             * Get the id string of the motion.
             * @result The string identification of the the motion.
             */
            const AZStd::string& GetId() const;

            /**
             * Check if the last loading attempt failed or not.
             * It is possible that the motion pointer is nullptr, but that this was because loading failed, rather than not having tried to load before.
             * @result Returns true when loading failed before, otherwise false is returned.
             */
            bool GetLoadingFailed() const                                                                       { return m_loadFailed; }

            /**
             * Set the flag that specifies whether loading failed or not.
             * This flag is used to skip trying to reload the motion on demand when it is nullptr and failed to load before.
             * @param failed Set to true to skip trying to reload on demand again.
             */
            void SetLoadingFailed(bool failed)                                                                  { m_loadFailed = failed; }

            /**
             * Reset the entry motion so that it will reload it next time automatically.
             */
            void Reset()                                                                                        { SetLoadingFailed(false); SetMotion(nullptr); }

            /**
             * Check if the filename assigned to the motion entry contains a file path relative to the media root folder or if it is an absolute file path.
             * @return True in case the filename is absolute, false in case it is relative to the media root folder.
             */
            bool CheckIfIsAbsoluteFilename() const;

            /**
             * Check if the given filename contains a file path relative to the media root folder or if it is an absolute file path.
             * @return True in case the filename is absolute, false in case it is relative to the media root folder.
             */
            static bool CheckIfIsAbsoluteFilename(const AZStd::string& filename);

            static void Reflect(AZ::ReflectContext* context);

        private:
            AZStd::string   m_filename;     /**< The local filename of the motion. */
            AZStd::string   m_id;           /**< The motion name. */
            Motion*         m_motion;       /**< A pointer to the motion. */
            bool            m_loadFailed;   /**< Did the last load attempt fail? */

            void SetId(const AZStd::string& id);
        };

        /**
         * Default constructor.
         */
        MotionSet();

        /**
         * Constructor.
         * @param[in] name The name of the motion set.
         * @param[in] parent The parent motion set. This parameter is optional.
         */
        MotionSet(const char* name, MotionSet* parent = nullptr);

        /**
         * Destructor.
         */
        virtual ~MotionSet();

        /**
         * Set the unique identification number for the motion set.
         * @param[in] id The unique identification number.
         */
        void SetID(uint32 id);

        /**
         * Get the unique identification number for the motion set.
         * @return The unique identification number.
         */
        uint32 GetID() const;

        /**
         * Set the name of the motion set.
         * @param name The name of the motion set.
         */
        void SetName(const char* name);

        /**
         * Returns the name of the motion set.
         * @result The name of the motion set.
         */
        const char* GetName() const;

        /**
         * Returns the name of the motion set, as a string object.
         * @result The name of the motion set.
         */
        const AZStd::string& GetNameString() const;

        /**
         * Set the filename of the motion set.
         * @param[in] filename The filename of the motion set.
         */
        void SetFilename(const char* filename);

        /**
         * Get the filename of the motion set.
         * @result The filename of the motion set.
         */
        const char* GetFilename() const;

        /**
         * Returns the filename of the motion set, as a string object.
         * @result The filename of the motion set.
         */
        const AZStd::string& GetFilenameString() const;

        /**
         *
         *
         */
        AZStd::string ConstructMotionFilename(const MotionEntry* motionEntry);

        /**
         * Add a motion entry to the motion set.
         * @param[in] motionEntry A pointer to the motion entry to add.
         */
        void AddMotionEntry(MotionEntry* motionEntry);

        /**
         * Get the number of motion entries.
         * @result The number of motion entries.
         */
        MCORE_INLINE size_t GetNumMotionEntries() const                                                                         { return m_motionEntries.size(); }

        /**
         * Get the container holding the motion entries for entry traversal.
         * @result The motion entry hashmap.
         */
        typedef AZStd::unordered_map<AZStd::string, MotionSet::MotionEntry*> MotionEntries;
        const MotionEntries& GetMotionEntries() const;

        /**
         * Gets child motions recursively.
         * Every Motion* returned in childMotions will have directly or indirectly this MotionSet as a parent
         * @param[out] childMotions set where the child motions are returned.
         */
        void RecursiveGetMotions(AZStd::unordered_set<Motion*>& childMotions) const;

        /**
         * Preallocate the array of motion entries.
         * This will NOT grow the motion entries array as reported by GetNumMotionEntries(). However, it internally pre-allocates memory to make the AddMotionEntry() calls faster.
         * @param[in] numMotionEntries The number of motion entries to peallocate
         */
        void ReserveMotionEntries(size_t numMotionEntries);

        /**
         * Remove all motion entries from the motion set.
         */
        void Clear();

        /**
         * Remove the given motion entry from the motion set.
         * @param[in] motionEntry A pointer to the motion entry to remove.
         */
        void RemoveMotionEntry(MotionEntry* motionEntry);

        /**
         * Find motion entry by motion pointer.
         * @param[in] motion A pointer to the motion.
         * @result A pointer to the motion entry for the given motion. nullptr in case no motion entry has been found.
         */
        MotionEntry* FindMotionEntry(const Motion* motion) const;


        MotionEntry* FindMotionEntryById(const AZStd::string& motionId) const;
        MotionEntry* RecursiveFindMotionEntryById(const AZStd::string& motionId) const;
        Motion* RecursiveFindMotionById(const AZStd::string& motionId, bool loadOnDemand = true) const;
        MotionSet* RecursiveFindMotionSetByName(const AZStd::string& motionSetName, bool isOwnedByRuntime = false) const;

        /**
         * Set a new motion id for the given motion entry.
         * This will adjust the actual motion entry as well as update the hashmap.
         * @param[in] motionEntry The motion entry to change the id for.
         * @param[in] newMotionId The new motion id.
         */
        void SetMotionEntryId(MotionEntry* motionEntry, const AZStd::string& newMotionId);

        /**
         * Load the motion for a given entry.
         * @param[in] entry The entry to load the motion for.
         * @result The motion object.
         */
        Motion* LoadMotion(MotionEntry* entry) const;

        /**
         * Pre-load all motions inside the set and all its child sets.
         * This prevents them from loading on demand only, in case loading performance is an issue.
         */
        void Preload();

        /**
         * Resets all motion entries and pre-loads them once again using the latest media root folder path.
         */
        void Reload();

        /**
         * Add the given motion set as a child set.
         * @param[in] motionSet A pointer to the motion set that will be added as child set.
         */
        void AddChildSet(MotionSet* motionSet);

        /**
         * Get the number of child motion sets.
         * @result The number of child sets.
         */
        size_t GetNumChildSets() const;

        /**
         * Get the given child motion set.
         * @param[in] index The index of the child set to get. The index must be in range [0, GetNumChildSets()].
         * @result A pointer to the child set at the given index.
         */
        MotionSet* GetChildSet(size_t index) const;

        /**
         * Gets child motion sets recursively.
         * Every MotionSet* returned in childMotionSets will have directly or indirectly this MotionSet as a parent
         * @param[out] childMotionSets set where the child motion sets are returned.
         */
        void RecursiveGetMotionSets(AZStd::vector<const MotionSet*>& childMotionSets, bool isOwnedByRuntime = false) const;

        /**
         * Remove the child set with the given id.
         * @param[in] childSetID The id of the child set which will be removed.
         */
        void RemoveChildSetByID(uint32 childSetID);

        /**
         * Get a pointer to the parent motion set. nullptr if this is a root set.
         * @result A pointer to the parent motion set. In case this is a root set nullptr will be returned.
         */
        MotionSet* GetParentSet() const;

        /**
         * Generate a list of motion identifications strings. This will walk over all motions and get their id strings.
         * @param[out] idStrings A list of motion identification strings from the motions inside the motion set. Make sure the array is empty before calling this function.
         */
        void BuildIdStringList(AZStd::vector<AZStd::string>& idStrings) const;

        /**
         * Recursively find the root motion set.
         * This will return a pointer to itself when this is a root motion set already.
         * @result A pointer to the root motion set.
         */
        MotionSet* FindRootMotionSet() const;

        /**
         * Set the dirty flag which indicates whether the user has made changes to the motion set. This indicator is set to true
         * when the user changed something like adding a new motion entry. When the user saves the motion set, the indicator is usually set to false.
         * @param dirty The dirty flag.
         */
        void SetDirtyFlag(bool dirty);

        /**
         * Get the dirty flag which indicates whether the user has made changes to the motion set. This indicator is set to true
         * when the user changed something like adding a new motion entry. When the user saves the motion set, the indicator is usually set to false.
         * @return The dirty flag.
         */
        bool GetDirtyFlag() const;

        /**
         * Set if we want to automatically unregister this motion from the motion manager when we delete this motion.
         * On default this is set to true.
         * @param enabled Set to true when you wish to automatically have the motion unregistered, otherwise set it to false.
         */
        void SetAutoUnregister(bool enabled);

        /**
         * Check if this motion is automatically being unregistered from the motion manager when this motion gets deleted or not.
         * @result Returns true when it will get automatically deleted, otherwise false is returned.
         */
        bool GetAutoUnregister() const;

        /**
         * Marks the object as used by the engine runtime, as opposed to the tool suite.
         */
        void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        bool GetIsOwnedByRuntime() const;

        /**
         * Marks the object as owned by an asset, as opposed to the tool suite.
         */
        void SetIsOwnedByAsset(bool isOwnedByAsset);
        bool GetIsOwnedByAsset() const;

        void SetCallback(MotionSetCallback* callback, bool delExistingOneFromMem = true);

        MotionSetCallback* GetCallback() const;

        static void Reflect(AZ::ReflectContext* context);

        static MotionSet* LoadFromFile(const AZStd::string& filename, AZ::SerializeContext* context, const AZ::ObjectStream::FilterDescriptor& loadFilter = AZ::ObjectStream::FilterDescriptor(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
        static MotionSet* LoadFromBuffer(const void* buffer, const AZStd::size_t length, AZ::SerializeContext* context);

        bool SaveToFile(const AZStd::string& filename, AZ::SerializeContext* context) const;

        void Log();

        /**
        * Get the number of motion entries that contain morph data.
        * @result The number of motion entries that contain morph data.
        */
        size_t GetNumMorphMotions() const;


    private:
        void RecursiveRewireParentSets(MotionSet* motionSet);
        void InitAfterLoading();

        MotionEntries                               m_motionEntries;        /**< Hash map for storing the motion entries. Key is the 32-bit unsigned integer representing the motion id string using the string id generator. */
        AZStd::vector<MotionSet*>                   m_childSets;            /**< The array of pointers to the child motion sets. */
        AZStd::string                               m_name;                 /**< The name of the motion set. */
        AZStd::string                               m_filename;             /**< The filename of the motion set. */
        mutable MCore::MutexRecursive               m_mutex;                /**< The multithread mutex. */
        MotionSet*                                  m_parentSet;            /**< A pointer to the parent motion set. */
        MotionSetCallback*                          m_callback;             /**< The motion set callback, which handles the loading. */
        uint32                                      m_id;                   /**< The unique identification number for the motion set. */
        bool                                        m_dirtyFlag;            /**< The dirty flag which indicates whether the user has made changes to the motion set since the last file save operation. */
        bool                                        m_autoUnregister;       /**< Specifies whether we will automatically unregister this motion set from the motion manager or not, when deleting this object. */

#if defined(EMFX_DEVELOPMENT_BUILD)
        bool                                        m_isOwnedByRuntime; /**< Set if this motion set belongs to the engine runtime, as opposed to the tool suite. */
        bool                                        m_isOwnedByAsset;        /**< Set if the anim graph is used/owned by an asset. */
#endif // EMFX_DEVELOPMENT_BUILD
    };


    /**
     *
     *
     *
     */
    class EMFX_API MotionSetCallback
    {
    public:
        AZ_RTTI(MotionSetCallback, "{687F0769-75F4-49B9-9BC9-FBE6EA472ED4}")
        AZ_CLASS_ALLOCATOR_DECL

        MotionSetCallback();
        MotionSetCallback(MotionSet* motionSet);
        virtual ~MotionSetCallback();

        virtual Motion* LoadMotion(MotionSet::MotionEntry* entry);

        MotionSet* GetMotionSet() const                 { return m_motionSet; }
        void SetMotionSet(MotionSet* motionSet)         { m_motionSet = motionSet; }

    protected:
        MotionSet* m_motionSet;
    };
} // namespace EMotionFX
