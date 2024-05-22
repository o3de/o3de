/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "../EMotionFXConfig.h"
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Endian.h>
#include <MCore/Source/RefCounted.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/std/string/string.h>

MCORE_FORWARD_DECLARE(File);
MCORE_FORWARD_DECLARE(Attribute);

namespace EMotionFX
{
    // forward declarations
    class Actor;
    class MotionSet;
    class Motion;
    class ChunkProcessor;
    class SharedData;
    class VertexAttributeLayerAbstractData;
    class NodeMap;
    class Motion;
    class AnimGraph;

    /**
     * The EMotion FX importer, used to load actors, motions, animgraphs, motion sets and node maps and other EMotion FX related files.
     * The files can be loaded from memory or disk.
     *
     * Basic usage:
     *
     * <pre>
     * EMotionFX::Actor* actor = EMotionFX::GetImporter().LoadActor("TestActor.actor");
     * if (actor == nullptr)
     *     MCore::LogError("Failed to load the actor.");
     * </pre>
     *
     * The same applies to other types like motions, but using the LoadMotion, LoadAnimGraph and LoadMotionSet methods.
     */
    class EMFX_API Importer : 
        public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class Initializer;
        friend class EMotionFXManager;

    public:
        /**
         * The attribute data endian conversion callback.
         * This callback is responsible for converting the endian of the data stored inside a given attribute into the currently expected endian.
         */
        typedef bool (MCORE_CDECL * AttributeEndianConverter)(MCore::Attribute& attribute, MCore::Endian::EEndianType sourceEndianType);

        /**
         * The standard endian conversion callback which converts endian for attribute data.
         * This automatically handles the default types of the attributes.
         * @param attribute The attribute to convert the endian for.
         * @param sourceEndianType The endian type the data is stored in before conversion.
         * @result Returns true when conversion was successful or false when the attribute uses an unknown data type, which we don't know how to convert.
         */
        static bool MCORE_CDECL StandardAttributeEndianConvert(MCore::Attribute& attribute, MCore::Endian::EEndianType sourceEndianType);

        /**
         * The actor load settings.
         * You can pass such struct to the LoadActor method.
         */
        struct EMFX_API ActorSettings
        {
            bool m_loadLimits = true;                               /**< Set to false if you wish to disable loading of joint limits. */
            bool m_loadSkeletalLoDs = true;                         /**< Set to false if you wish to disable loading of skeletal LOD levels. */
            bool m_loadMorphTargets = true;                         /**< Set to false if you wish to disable loading any morph targets. */
            bool m_dualQuatSkinning = false;                        /**< Set to true  if you wish to enable software skinning using dual quaternions. */
            bool m_makeGeomLoDsCompatibleWithSkeletalLoDs = false;  /**< Set to true if you wish to disable the process that makes sure no skinning influences are mapped to disabled bones. Default is false. */
            bool m_unitTypeConvert = true;                          /**< Set to false to disable automatic unit type conversion (between cm, meters, etc). On default this is enabled. */
            bool m_loadSimulatedObjects = true;                     /**< Set to false if you wish to disable loading of simulated objects. */
            bool m_optimizeForServer = false;                       /**< Set to true if you witsh to optimize this actor to be used on server. */
            uint32 m_threadIndex = 0;
            AZStd::vector<uint32>    m_chunkIDsToIgnore;      /**< Add chunk ID's to this array. Chunks with these ID's will not be processed. */

            /**
             * If the actor need to be optimized for server, will overwrite a few other actor settings.
             */
            void OptimizeForServer()
            {
                m_loadSkeletalLoDs = false;
                m_loadMorphTargets = false;
                m_loadSimulatedObjects = false;
            }
        };

        /**
         * The motion import options.
         * This can be used in combination with the LoadMotion method.
         */
        struct EMFX_API MotionSettings
        {
            bool m_forceLoading = false;       /**< Set to true in case you want to load the motion even if a motion with the given filename is already inside the motion manager. */
            bool m_loadMotionEvents = true;    /**< Set to false if you wish to disable loading of motion events. */
            bool m_unitTypeConvert = true;     /**< Set to false to disable automatic unit type conversion (between cm, meters, etc). On default this is enabled. */
            AZStd::vector<uint32> m_chunkIDsToIgnore;  /**< Add the ID's of the chunks you wish to ignore. */
        };

        /**
         * The motion set import options.
         * This can be used in combination with the LoadMotionSet method.
         */
        struct EMFX_API MotionSetSettings
        {
            bool m_isOwnedByRuntime = false;
        };

        /**
         * The node map import options.
         * This can be used in combination with the LoadNodeMap method.
         */
        struct EMFX_API NodeMapSettings
        {
            bool m_autoLoadSourceActor = true;   /**< Should we automatically try to load the source actor? (default=true) */
            bool m_loadNodes = true;             /**< Add nodes to the map? (default=true) */
        };

        struct EMFX_API ImportParameters
        {
            Actor*                              m_actor = nullptr;
            Motion*                             m_motion = nullptr;
            Importer::ActorSettings*            m_actorSettings = nullptr;
            Importer::MotionSettings*           m_motionSettings = nullptr;
            AZStd::vector<SharedData*>*          m_sharedData = nullptr;
            MCore::Endian::EEndianType          m_endianType = MCore::Endian::ENDIAN_LITTLE;

            NodeMap*                            m_nodeMap = nullptr;
            Importer::NodeMapSettings*          m_nodeMapSettings = nullptr;
            bool                                m_isOwnedByRuntime = false;
            bool                                m_additiveMotion = false;
        };


        /**
         * The file types.
         * This can be used to identify the file type, with the CheckFileType methods.
         */
        enum EFileType
        {
            FILETYPE_UNKNOWN        = 0,    /**< An unknown file, or something went wrong. */
            FILETYPE_ACTOR,                 /**< An actor file (.actor). */
            FILETYPE_MOTION,                /**< A motion file (.motion). */
            FILETYPE_ANIMGRAPH,             /**< A animgraph file (.animgraph). */
            FILETYPE_MOTIONSET,             /**< A motion set file (.motionSet). */
            FILETYPE_NODEMAP                /**< A node map file (.nodeMap). */
        };

        struct FileInfo
        {
            MCore::Endian::EEndianType  m_endianType;
        };

        //-------------------------------------------------------------------------------------------------

        static Importer* Create();

        //-------------------------------------------------------------------------------------------------

        /**
         * Load an actor from a given file.
         * A file does not have to be stored on disk, but can also be in memory or in an archive or
         * on some network stream. Anything is possible.
         * @param f The file to load the actor from (after load, the file will be closed).
         * @param settings The settings to use for loading. When set to nullptr, all defaults will be used and everything will be loaded.
         * @param filename The file name to set inside the Actor object. This is not going to load the actor from the file specified to this parameter, but just updates the value returned by actor->GetFileName().
         * @result Returns a pointer to the loaded actor, or nullptr when something went wrong and the actor could not be loaded.
         */
        AZStd::unique_ptr<Actor> LoadActor(MCore::File* f, ActorSettings* settings = nullptr, const char* filename = "");

        /**
         * Loads an actor from a file on disk.
         * @param filename The name of the file on disk.
         * @param settings The settings to use for loading. When set to nullptr, all defaults will be used and everything will be loaded.
         * @result Returns a pointer to the loaded actor, or nullptr when something went wrong and the actor could not be loaded.
         */
        AZStd::unique_ptr<Actor> LoadActor(AZStd::string filename, ActorSettings* settings = nullptr);

        /**
         * Loads an actor from memory.
         * @param memoryStart The start address of the file in memory.
         * @param lengthInBytes The length of the file, in bytes.
         * @param settings The settings to use for loading. When set to nullptr, all defaults will be used and everything will be loaded.
         * @param filename The file name to set inside the Actor object. This is not going to load the actor from the file specified to this parameter, but just updates the value returned by actor->GetFileName().
         * @result Returns a pointer to the loaded actor, or nullptr when something went wrong and the actor could not be loaded.
         */
        AZStd::unique_ptr<Actor> LoadActor(uint8* memoryStart, size_t lengthInBytes, ActorSettings* settings = nullptr, const char* filename = "");

        bool ExtractActorFileInfo(FileInfo* outInfo, const char* filename) const;

        //-------------------------------------------------------------------------------------------------

        /**
         * Load a motion from a given file.
         * A file does not have to be stored on disk, but can also be in memory or in an archive or
         * on some network stream. Anything is possible.
         * @param f The file to load the motion from (after load, the file will be closed).
         * @param settings The settings to use during loading, or nullptr when you want to use default settings, which would load everything.
         * @result Returns a pointer to the loaded motion, or nullptr when something went wrong and the motion could not be loaded.
         */
        Motion* LoadMotion(MCore::File* f, MotionSettings* settings = nullptr);

        /**
         * Loads a motion from a file on disk.
         * @param filename The name of the file on disk.
         * @param settings The settings to use during loading, or nullptr when you want to use default settings, which would load everything.
         * @result Returns a pointer to the loaded motion, or nullptr when something went wrong and the motion could not be loaded.
         */
        Motion* LoadMotion(AZStd::string filename, MotionSettings* settings = nullptr);

        /**
         * Loads a motion from memory.
         * @param memoryStart The start address of the file in memory.
         * @param lengthInBytes The length of the file, in bytes.
         * @param settings The settings to use during loading, or nullptr when you want to use default settings, which would load everything.
         * @result Returns a pointer to the loaded motion, or nullptr when something went wrong and the motion could not be loaded.
         */
        Motion* LoadMotion(uint8* memoryStart, size_t lengthInBytes, MotionSettings* settings = nullptr);

        bool ExtractMotionFileInfo(FileInfo* outInfo, const char* filename) const;

        //-------------------------------------------------------------------------------------------------

        /**
         * Load a anim graph file by filename.
         * @param filename The filename to load from.
         * @param loadFilter The filter descriptor for loading anim graph from file
         * @result The anim graph object, or nullptr in case loading failed.
         */
        AnimGraph* LoadAnimGraph(AZStd::string, const AZ::ObjectStream::FilterDescriptor& loadFilter = AZ::ObjectStream::FilterDescriptor(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));

        /**
         * Load a anim graph file from a memory location.
         * @param memoryStart The start address of the file in memory.
         * @param lengthInBytes The length of the file, in bytes.
         * @result The anim graph object, or nullptr in case loading failed.
         */
        AnimGraph* LoadAnimGraph(uint8* memoryStart, size_t lengthInBytes);

        //-------------------------------------------------------------------------------------------------

        /**
         * Loads a motion set from a file on disk.
         * @param filename The name of the file on disk.
         * @param settings The motion set importer settings, or nullptr to use default settings.
         * @param loadFilter The filter descriptor for loading motion set from file
         * @result The motion set object, or nullptr in case loading failed.
         */
        MotionSet* LoadMotionSet(AZStd::string filename, MotionSetSettings* settings = nullptr, const AZ::ObjectStream::FilterDescriptor& loadFilter = AZ::ObjectStream::FilterDescriptor(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));

        /**
         * Loads a motion set from memory.
         * @param memoryStart The start address of the file in memory.
         * @param lengthInBytes The length of the file, in bytes.
         * @param settings The motion set importer settings, or nullptr to use default settings.
         * @result The motion set object, or nullptr in case loading failed.
         */
        MotionSet* LoadMotionSet(uint8* memoryStart, size_t lengthInBytes, MotionSetSettings* settings = nullptr);

        //-------------------------------------------------------------------------------------------------

        /**
         * Load a node map from a given file.
         * A file does not have to be stored on disk, but can also be in memory or in an archive or  on some network stream. Anything is possible.
         * @param f The file to load the motion set from (after load, the file will be closed).
         * @param settings The node map importer settings, or nullptr to use default settings.
         * @result The node map object, or nullptr in case loading failed.
         */
        NodeMap* LoadNodeMap(MCore::File* f, NodeMapSettings* settings = nullptr);

        /**
         * Loads a node map from a file on disk.
         * @param filename The name of the file on disk.
         * @param settings The node map importer settings, or nullptr to use default settings.
         * @result The node map object, or nullptr in case loading failed.
         */
        NodeMap* LoadNodeMap(AZStd::string filename, NodeMapSettings* settings = nullptr);

        /**
         * Loads a node map from memory.
         * @param memoryStart The start address of the file in memory.
         * @param lengthInBytes The length of the file, in bytes.
         * @param settings The node map importer settings, or nullptr to use default settings.
         * @result The motion set object, or nullptr in case loading failed.
         */
        NodeMap* LoadNodeMap(uint8* memoryStart, size_t lengthInBytes, NodeMapSettings* settings = nullptr);

        //-------------------------------------------------------------------------------------------------

        /**
         * Register a new chunk processor.
         * It can either be a new version of a chunk processor to extend the current file format or
         * a complete new chunk processor. It will be added to the processor database and will then be executable.
         * @param processorToRegister The processor to register to the importer.
         */
        void RegisterChunkProcessor(ChunkProcessor* processorToRegister);

        /**
         * Find shared data objects which have the same type as the ID passed as parameter.
         * @param sharedDataArray The shared data array to search in.
         * @param type The shared data ID to search for.
         * @return A pointer to the shared data object, or nullptr when no shared data of this type has been found.
         */
        static SharedData* FindSharedData(AZStd::vector<SharedData*>* sharedDataArray, uint32 type);

        /**
         * Enable or disable logging.
         * @param enabled Set to true if you want to enable logging, or false when you want to disable it.
         */
        void SetLoggingEnabled(bool enabled);

        /**
         * Check if logging is enabled or not.
         * @return Returns true when the importer will perform logging, or false when it will be totally silent.
         */
        bool GetLogging() const;

        /**
         * Set if the importer should log the processor details or not.
         * @param detailLoggingActive Set to true when you want to enable this feature, otherwise set it to false.
         */
        void SetLogDetails(bool detailLoggingActive);

        /**
         * Check if detail-logging is enabled or not.
         * @return Returns true when detail-logging is enabled, otherwise false is returned.
         */
        bool GetLogDetails() const;

        /**
         * Check the type of a given file on disk.
         * @param filename The file on disk to check.
         * @result Returns the file type. The FILETYPE_UNKNOWN will be returned when something goes wrong or the file format is unknown.
         */
        EFileType CheckFileType(const char* filename);

        /**
         * Check the type of a given file (in memory or disk).
         * The file will be closed after executing this method!
         * @param file The file object to check.
         * @result Returns the file type. The FILETYPE_UNKNOWN will be returned when something goes wrong or the file format is unknown.
         */
        EFileType CheckFileType(MCore::File* file);


    private:
        AZStd::vector<ChunkProcessor*>       m_chunkProcessors;   /**< The registered chunk processors. */
        bool                                m_loggingActive;     /**< Contains if the importer should perform logging or not or not. */
        bool                                m_logDetails;        /**< Contains if the importer should perform detail-logging or not. */

        /**
         * The constructor.
         */
        Importer();

        /**
         * The destructor.
         */
        ~Importer();

        /**
         * Verify if the given file is a valid actor file that can be processed by the importer.
         * Please note that the specified must already been opened and must also be pointing to the location where the
         * Actor file header will be stored (the start of the file). The file will not be closed after this method!
         * The endian type of the file will be written inside the outEndianType parameter.
         * Also note that the file position (read position / cursor) will point after the header after this function has been executed.
         * @param f The file to perform the check on.
         * @param outEndianType The value that will contain the endian type used by the file.
         * @result Returns true when the file is a valid actor file that can be processed by the importer. Otherwise false is returned.
         */
        bool CheckIfIsValidActorFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const;

        /**
         * Verify if the given file is a valid motion file that can be processed by the importer.
         * Please note that the specified must already been opened and must also be pointing to the location where the
         * Motion file header will be stored (the start of the file). The file will not be closed after this method!
         * The endian type of the file will be written inside the outEndianType parameter.
         * Also note that the file position (read position / cursor) will point after the header after this function has been executed.
         * @param f The file to perform the check on.
         * @param outEndianType The value that will contain the endian type used by the file.
         * @result Returns true when the file is a valid actor file that can be processed by the importer. Otherwise false is returned.
         */
        bool CheckIfIsValidMotionFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const;

        /**
         * Verify if the given file is a valid node map file that can be processed by the importer.
         * Please note that the specified must already been opened and must also be pointing to the location where the
         * XPM file header will be stored (the start of the file). The file will not be closed after this method!
         * The endian type of the file will be written inside the outEndianType parameter.
         * Also note that the file position (read position / cursor) will point after the header after this function has been executed.
         * @param f The file to perform the check on.
         * @param outEndianType The value that will contain the endian type used by the file.
         * @result Returns true when the file is a valid actor file that can be processed by the importer. Otherwise false is returned.
         */
        bool CheckIfIsValidNodeMapFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const;

        /**
         * Add shared data.
         * Shared data is stored in the importer and provides the chunk processors to use and modify it.
         * Shared data objects need to be inherited from the Importer::SharedData base class and must get
         * their unique ID. Use shared data if you have several chunk processors that share data between them.
         * This function will add a new shared data object to the importer.
         * @param sharedData The array which holds the shared data objects.
         * @param data A pointer to your shared data object.
         */
        static void AddSharedData(AZStd::vector<SharedData*>& sharedData, SharedData* data);

        /*
         * Precreate the standard shared data objects.
         * @param sharedData The shared data array to work on.
         */
        static void PrepareSharedData(AZStd::vector<SharedData*>& sharedData);

        /**
         * Reset all shared data objects.
         * Resetting these objects will clear/empty their internal data.
         */
        static void ResetSharedData(AZStd::vector<SharedData*>& sharedData);

        /**
         * Find the chunk processor which has a given ID and version number.
         * @param chunkID The ID of the chunk processor.
         * @param version The version number of the chunk processor.
         * @return A pointer to the chunk processor if it has been found, or nullptr when no processor could be found
         *         which is able to process the given chunk or the given version of the chunk.
         */
        ChunkProcessor* FindChunk(uint32 chunkID, uint32 version) const;

        /**
         * Register the standard chunk processors to be able to import all standard LMA chunks.
         * Shared data which is essential for the standard chunks will also be automatically created and added.
         */
        void RegisterStandardChunks();

        /**
         * Read and process the next chunk.
         * @param file The file to read from.
         * @param importParams The import parameters.
         * @result Returns false when the chunk could not be processed, due to an end of file or other file error.
         */
        bool ProcessChunk(MCore::File* file, Importer::ImportParameters& importParams);


        /**
         * Validate and resolve any conflicting settings inside a specific motion import settings object.
         * @param settings The motion settings to verify and fix/modify when needed.
         */
        void ValidateMotionSettings(MotionSettings* settings);
    };
} // namespace EMotionFX
