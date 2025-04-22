/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Distance.h>
#include "ThreadData.h"
#include <MCore/Source/RefCounted.h>

#include <AzCore/Module/Environment.h>

MCORE_FORWARD_DECLARE(MemoryTracker);

namespace AZ::Render
{
    class RenderActorSettings;
}

namespace EMotionFX
{
    // forward declarations
    class Importer;
    class ActorManager;
    class MotionManager;
    class EventManager;
    class SoftSkinManager;
    class AnimGraphManager;
    class Recorder;
    class MotionInstancePool;
    class EventDataFactory;
    class DebugDraw;
    class PoseDataFactory;

    // versions
#define EMFX_HIGHVERSION 4
#define EMFX_LOWVERSION 0000

    // media root replacement string
#define EMFX_MEDIAROOTFOLDER_STRING "$(MEDIAROOT)"

    //-----------------------------------------------------------------------------

    /**
     * The EMotion FX manager.
     * This class can be used to obtain things such as version information about the current library version.
     * Also it will provide initialization functionality which will automatically register things such as
     * the EMotionFX related memory categories to the Core memory manager.
     */
    class EMFX_API EMotionFXManager
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class Initializer;

    public:
        static EMotionFXManager* Create();

        /**
         * Log information about this EMotion FX version.
         */
        void LogInfo();

        /**
         * Get the version string, which can be like: "EMotion FX v3.0".
         * @result The version string.
         */
        const char* GetVersionString() const;

        /**
         * Get the compilation date string.
         * @result The compilation date string.
         */
        const char* GetCompilationDate() const;

        /**
         * Get the high version number.
         * This would be 3 in case of EMotion FX v3.01.
         * @result The high version number.
         */
        uint32 GetHighVersion() const;

        /**
         * Get the low version number.
         * This would be 0 in case of EMotion FX v3.0.
         * And it would be 10 in case of v3.01, or 100 in case of v3.10.
         * @result The low version number.
         */
        uint32 GetLowVersion() const;

        /**
         * Get the global simulation speed factor. This value is multiplied with the timePassedInSeconds values (the frame times) when doing update calls.
         * The default value is 1.0.
         * @result The currently set simulation play speed.
         */
        float GetGlobalSimulationSpeed() const;

        /**
         * Set the global simulation speed factor.
         * On default this is 1.0, which is normal speed. The value you set is basically being multiplied with the timePassedInSeconds values (the frame times) when doing update calls.
         * @param speedFactor The speed factor, which can be any number equal to zero or higher.
         */
        void SetGlobalSimulationSpeed(float speedFactor);

        /**
         * Build the low version string.
         * In case of EMotion FX v3.9 this would make the output string equal to "9".
         * In case of EMotion FX v3.05 this would make it equal to "05".
         * @param outLowVersionString The output string to store the low version number in.
         */
        void BuildLowVersionString(AZStd::string& outLowVersionString);

        /**
         * Update EMotion FX with a given time delta.
         * You normally will call this only once per frame. It will update all actor instances internally.
         * @param timePassedInSeconds The time delta to update with, so basically the time passed since the last time you called Update.
         */
        void Update(float timePassedInSeconds);

        /**
         * Get the importer, which can be used to load actors and motions.
         * This can also be accessed with the GetImporter() macro.
         * @result A pointer to the importer.
         */
        MCORE_INLINE Importer* GetImporter() const                                  { return m_importer; }

        /**
         * Get the actor manager.
         * This can also be accessed with the GetActorManager() macro.
         * @result A pointer to the actor manager.
         */
        MCORE_INLINE ActorManager* GetActorManager() const                          { return m_actorManager; }

        /**
         * Get the motion manager.
         * This can also be accessed with the GetMotionManager() macro.
         * @result A pointer to the motion manager.
         */
        MCORE_INLINE MotionManager* GetMotionManager() const                        { return m_motionManager; }

        /**
         * Get the event manager.
         * This can also be accessed with the GetEventManager() macro.
         * @result A pointer to the event manager.
         */
        MCORE_INLINE EventManager* GetEventManager() const                          { return m_eventManager; }

        /**
         * Get the soft-skin manager.
         * This manager is responsible for creating the best suitable softskin deformer type.
         * On SSE powered PC's this could for example return an SSE optimized softskin mesh deformer, while
         * on other systems it would return a native C++ version.
         * This can also be accessed with the GetSoftSkinManager() macro.
         * @result A pointer to the soft-skinning manager.
         */
        MCORE_INLINE SoftSkinManager* GetSoftSkinManager() const                    { return m_softSkinManager; }

        /**
         * Get the motion instance pool.
         * This can also be accessed with the GetMotionInstancePool() macro.
         * @result A pointer to the motion instance pool.
         */
        MCORE_INLINE MotionInstancePool* GetMotionInstancePool()    const           { return m_motionInstancePool; }

        /**
         * Get the animgraph manager;
         * This can also be accessed with the GetAnimGraphManager() macro.
         * @result A pointer to the animgraph manager.
         */
        MCORE_INLINE AnimGraphManager* GetAnimGraphManager() const                { return m_animGraphManager; }

        /**
         * Get the recorder.
         * This can also be accessed with the EMFX_RECODRER macro.
         * @result A pointer to the recorder.
         */
        MCORE_INLINE Recorder* GetRecorder() const                                  { return m_recorder; }

        /**
         * Get the debug drawing class.
         * @result A pointer to the wavelet cache.
         */
        MCORE_INLINE DebugDraw* GetDebugDraw() const                                { return m_debugDraw; }

        MCORE_INLINE PoseDataFactory* GetPoseDataFactory() const                    { return m_poseDataFactory; }

        /**
         * Get the render actor settings
         * @result A pointer to global render actor settings.
         */
        AZ::Render::RenderActorSettings* GetRenderActorSettings() const
        {
            return m_renderActorSettings.get();
        }

        /**
         * Set the path of the media root directory.
         * @param path The path of the media root folder.
         */
        void SetMediaRootFolder(const char* path);

        /**
         * Initialize the asset source and cache folder paths.
         */
        void InitAssetFolderPaths();

        /**
         * Convert relative filename (e.g. '$(MEDIAROOT)MyFolder/MyMotion.motion') to absolute one (e.g. 'C:/MyGame/Data/MyFolder/MyMotion.motion').
         * Converts the input filename to match the folder separator character and replaces the media root keyword with the actual media root folder location.
         * @param relativeFilename Relative filename containing the '$(MEDIAROOT)' keyword.
         * @param outAbsoluteFilename Will contain the absolute filename after execution.
         */
        void ConstructAbsoluteFilename(const char* relativeFilename, AZStd::string& outAbsoluteFilename);

        /**
         * Convert relative filename (e.g. '$(MEDIAROOT)MyFolder/MyMotion.motion') to absolute one (e.g. 'C:/MyGame/Data/MyFolder/MyMotion.motion').
         * Converts the input filename to match the folder separator character and replaces the media root keyword with the actual media root folder location.
         * @param relativeFilename Relative filename containing the '$(MEDIAROOT)' keyword.
         * @result Will contain the absolute filename after execution.
         */
        AZStd::string ConstructAbsoluteFilename(const char* relativeFilename);

        /**
         * Convert the passed absolute filename to one relative to the media root folder.
         * @param inOutFileName Absolute filename to be changed to a relative one using the current media root folder.
         */
        void GetFilenameRelativeToMediaRoot(AZStd::string* inOutFilename) const;

        /**
         * Convert the given absolute filename to one relative to the given folder path.
         * @param folderPath Base folder path to remove.
         * @param inOutFileName Absolute filename to be changed to a relative one.
         */
        static void GetFilenameRelativeTo(AZStd::string* inOutFilename, const char* folderPath);

        /**
         * Accesses the FileIO instance to resolve file alias to full paths
         * @param path file path to resolve file aliases
         * @result The path with file aliases substituted to the registered
         * path entries in the FileIO instance
         */
        static AZStd::string ResolvePath(const char* path);

        /**
         * Get the path of the media root folder.
         * @result The path of the media root directory.
         */
        MCORE_INLINE const char* GetMediaRootFolder() const                         { return m_mediaRootFolder.c_str(); }

        /**
         * Get the path of the media root folder as a string object.
         * @result The path of the media root directory.
         */
        MCORE_INLINE const AZStd::string& GetMediaRootFolderString() const          { return m_mediaRootFolder; }

        /**
         * Get the asset source folder path.
         * @result The path of the asset source folder.
         */
        MCORE_INLINE const AZStd::string& GetAssetSourceFolder() const              { return m_assetSourceFolder; }

        /**
         * Get the asset cache folder path.
         * @result The path of the asset cache folder.
         */
        MCORE_INLINE const AZStd::string& GetAssetCacheFolder() const               { return m_assetCacheFolder; }

        /**
         * Get the unique per thread data for a given thread by index.
         * @param threadIndex The thread index, which must be between [0..GetNumThreads()-1].
         * @return The unique thread data for this thread.
         */
        MCORE_INLINE ThreadData* GetThreadData(uint32 threadIndex) const            { MCORE_ASSERT(threadIndex < m_threadDatas.size()); return m_threadDatas[threadIndex]; }

        /**
         * Get the number of threads that are internally created.
         * @return The number of threads that we have internally created.
         */
        MCORE_INLINE size_t GetNumThreads() const                                   { return m_threadDatas.size(); }

        /**
         * Shrink the memory pools, to reduce memory usage.
         * When you create many actor instances and destroy them later again, the pools have been grown internally, which increases memory usage.
         * To shrink the pools back to their minimum size again this method can be called.
         * Keep in mind that this will free blocks of memory and can cause new allocations to happen again in next frames.
         * So it is not advised to run this every frame.
         */
        void ShrinkPools();

        /**
         * Register the EMotion FX related memory categories to a given memory tracker.
         * @param memTracker The memory tracker to register to.
         */
        void RegisterMemoryCategories(MCore::MemoryTracker& memTracker);

        /**
         * Get the current unit type used. On default this is set to meters, so where one unit is one meter.
         * @result The unit type currently used.
         */
        MCore::Distance::EUnitType GetUnitType() const;

        /**
         * Initialize the internal unit type to a given one. A unit type is for example meters, centimeters, etc.
         * This normally is done at init time. All loaded data will be converted into this unit type scale.
         * On default EMotion FX uses meters, so if you have an actor that is 180 centimeters high, it will be 1.8 units in height after loading.
         * It is recommended to save all your files into the same unit system as your game uses. Set EMotion Studio to this same unit type and save your data (actors, motions, animgraphs).
         * Please keep in mind switching the unit type using this method after you have actors loaded does not modify the already loaded actors. The same goes for motions and animgraphs and anything else.
         * @param unitType The unit type to use internally.
         */
        void SetUnitType(MCore::Distance::EUnitType unitType);

        /**
         * Get if EMotionFX is in editor mode or not
         * @return True if EMotionFX is configured in editor mode (e.g. ly editor or EMStudio).
         */
        bool GetIsInEditorMode() const { return m_isInEditorMode; }

        /**
         * Sets EMotionFX's editor mode. 
         * @param editorMode Indicating if EMotionFX is running in editor mode.
         */
        void SetIsInEditorMode(bool isInEditorMode) { m_isInEditorMode = isInEditorMode; }

        /**
         * Get if EMotionFX is in server mode or not
         * @return True if EMotionFX is configured in server mode.
         */
        bool GetIsInServerMode() const { return m_isInServerMode; }

        /**
         * Sets EMotionFX's server mode.
         * @param editorMode Indicating if EMotionFX is running in server mode.
         */
        void SetIsInServerMode(bool isInServerMode) { m_isInServerMode = isInServerMode; }

        /**
         * Get if EMotionFX is enabled for server optimization.
         * @return True if EMotionFX is in server mode and enabled for server optimization.
         */
        bool GetEnableServerOptimization() const { return m_isInServerMode && m_enableServerOptimization; }

    private:
        AZStd::string               m_versionString;         /**< The version string. */
        AZStd::string               m_compilationDate;       /**< The compilation date string. */
        AZStd::string               m_mediaRootFolder;       /**< The path of the media root directory. */
        AZStd::string               m_assetSourceFolder;     /**< The absolute path of the asset source folder. */
        AZStd::string               m_assetCacheFolder;      /**< The absolute path of the asset cache folder. */
        uint32                      m_highVersion;           /**< The higher version, which would be 3 in case of v3.01. */
        uint32                      m_lowVersion;            /**< The low version, which would be 100 in case of v3.10 or 10 in case of v3.01. */
        Importer*                   m_importer;              /**< The importer that can load actors and motions. */
        ActorManager*               m_actorManager;          /**< The actor manager. */
        MotionManager*              m_motionManager;         /**< The motion manager. */
        EventManager*               m_eventManager;          /**< The motion event manager. */
        SoftSkinManager*            m_softSkinManager;       /**< The softskin manager. */
        AnimGraphManager*           m_animGraphManager;      /**< The animgraph manager. */
        PoseDataFactory*            m_poseDataFactory;
        Recorder*                   m_recorder;              /**< The recorder. */
        MotionInstancePool*         m_motionInstancePool;    /**< The motion instance pool. */        
        DebugDraw*                  m_debugDraw;             /**< The debug drawing system. */
        AZStd::unique_ptr<AZ::Render::RenderActorSettings> m_renderActorSettings;   /**< The global render actor settings. */

        AZStd::vector<ThreadData*>   m_threadDatas;           /**< The per thread data. */
        MCore::Distance::EUnitType  m_unitType;              /**< The unit type, on default it is MCore::Distance::UNITTYPE_METERS. */
        float                       m_globalSimulationSpeed; /**< The global simulation speed, default is 1.0. */
        bool                        m_isInEditorMode;       /**< True when the runtime requires to support an editor. Optimizations can be made if there is no need for editor support. */
        bool                        m_isInServerMode;       /**< True when emotionfx is running on server. */
        bool                        m_enableServerOptimization; /**< True when optimization can be made when emotionfx is running in server mode. */

        /**
         * The constructor.
         */
        EMotionFXManager();

        /**
         * The destructor.
         */
        ~EMotionFXManager();

        /**
         * Set the importer.
         * @param importer The importer to use.
         */
        void SetImporter(Importer* importer);

        /**
         * Set the actor manager.
         * @param manager The actor manager to use.
         */
        void SetActorManager(ActorManager* manager);

        /**
         * Set the motion manager.
         * @param manager The motion manager to use.
         */
        void SetMotionManager(MotionManager* manager);

        /**
         * Set the event manager.
         * @param manager The event manager to use.
         */
        void SetEventManager(EventManager* manager);

        /**
         * Set the softskin manager.
         * @param manager The softskin manager to use.
         */
        void SetSoftSkinManager(SoftSkinManager* manager);

        /**
         * Set the animgraph manager.
         * @param manager The anim graph manager.
         */
        void SetAnimGraphManager(AnimGraphManager* manager);

        /**
         * Set the recorder.
         * @param recorder The recorder object.
         */
        void SetRecorder(Recorder* recorder);

        /**
         * Set the debug draw object.
         * @param draw The debug drawing object.
         */
        void SetDebugDraw(DebugDraw* draw);

        /**
         * Set the motion instance pool.
         * @param pool The motion instance pool.
         */
        void SetMotionInstancePool(MotionInstancePool* pool);

        void SetPoseDataFactory(PoseDataFactory* poseDataFactory) { m_poseDataFactory = poseDataFactory; }

        /**
         * Set the number of threads to use.
         * @param numThreads The number of threads to use internally. This must be a value of 1 or above.
         */
        void SetNumThreads(uint32 numThreads);
    };

    //-----------------------------------------------------------------------------

    /**
     * The EMotion FX initializer class.
     * This class is responsible for initializing and shutdown of EMotion FX.
     * Before you make any calls to EMotionFX you have to call the EMotionFX::Initializer::Init() method.
     * Also don't forget to call EMotionFX::Initializer::Shutdown() after you stop using EMotion FX.
     * The EMotionFX::Initializer::Shutdown() method should be called most likely at application shutdown.
     *
     * Please note that BEFORE you call EMotionFX::Initializer::Init() you also have called MCore::Initializer::Init().
     * And also be sure that AFTER you call EMotionFX::Initializer::Shutdown() you also call MCore::Initializer::Shutdown(), unless your code
     * still uses other MCore classes/functions after that point.
     */
    class EMFX_API Initializer
    {
    public:
        /**
         * The initialization settings for the EMotion FX API.
         * Here you can for example specify the unit type to use.
         */
        struct EMFX_API InitSettings
        {
            MCore::Distance::EUnitType  m_unitType;      /**< The unit type to use. This specifies the size of one unit. On default this is a meter. */

            InitSettings()
            {
                m_unitType = MCore::Distance::UNITTYPE_METERS;
            }
        };

        /**
         * Initializes EMotion FX.
         * After calling this method you can use all EMotion FX API functionality.
         * Be sure to call MCore::Initializer::Init() before calling this method, if you haven't initialized MCore yet.
         * If default init settings are used (when no parameter or nullptr is passed to this method) it will use the standard DirectX style coordinate system (right=x+, up=y+, forward=z+) and one 3D unit will be a meter.
         * @param initSettings The init settings, or nullptr to use defaults.
         * @result Returns true when initialization has succeeded. Otherwise false is returned.
         */
        static bool MCORE_CDECL Init(InitSettings* initSettings = nullptr);

        /**
         * Shutdown EMotion FX.
         * After calling this method calling the EMotion FX API should not be used anymore.
         * Be sure to call MCore::Initializer::Shutdown() AFTER calling this method, and not before.
         */
        static void MCORE_CDECL Shutdown();
    };

    //-----------------------------------------------------------------------------

    /**
     * The EMotion FX global main object.
     * This object contains all the managers, and importer etc.
     * On default this object is nullptr, but after calling EMotionFX::Initializer::Init() this will be created
     * and calls to macros like GetImporter() will be valid after that point.
     * Don't forget to call EMotionFX::Initializer::Shutdown() as well.
     * For more information about Init and Shutdown have a look at the class EMotionFX::Initializer.
     */
    extern EMFX_API AZ::EnvironmentVariable<EMotionFXManager*> gEMFX;
    static const char* kEMotionFXInstanceVarName = "EMotionFXInstance";

    //
    MCORE_INLINE EMotionFXManager&          GetEMotionFX() /**< Get the EMotion FX manager object. */
    {
        if (!gEMFX)
        {
            gEMFX = AZ::Environment::FindVariable<EMotionFXManager*>(kEMotionFXInstanceVarName);
        }
        return *(gEMFX.Get());
    }

    MCORE_INLINE Importer&                  GetImporter()               { return *GetEMotionFX().GetImporter(); }           /**< Get the importer that can load actors and motions. */
    MCORE_INLINE ActorManager&              GetActorManager()           { return *GetEMotionFX().GetActorManager(); }       /**< Get the actor manager. */
    MCORE_INLINE MotionManager&             GetMotionManager()          { return *GetEMotionFX().GetMotionManager(); }      /**< Get the motion manager. */
    MCORE_INLINE EventManager&              GetEventManager()           { return *GetEMotionFX().GetEventManager(); }       /**< Get the motion event manager. */
    MCORE_INLINE SoftSkinManager&           GetSoftSkinManager()        { return *GetEMotionFX().GetSoftSkinManager(); }    /**< Get the softskin manager. */
    MCORE_INLINE AnimGraphManager&          GetAnimGraphManager()       { return *GetEMotionFX().GetAnimGraphManager(); }   /**< Get the animgraph manager. */
    MCORE_INLINE Recorder&                  GetRecorder()               { return *GetEMotionFX().GetRecorder(); }           /**< Get the recorder. */
    MCORE_INLINE MotionInstancePool&        GetMotionInstancePool()     { return *GetEMotionFX().GetMotionInstancePool(); } /**< Get the motion instance pool. */
    MCORE_INLINE DebugDraw&                 GetDebugDraw()              { return *GetEMotionFX().GetDebugDraw(); }          /**< Get the debug drawing. */
    MCORE_INLINE PoseDataFactory&           GetPoseDataFactory()        { return *GetEMotionFX().GetPoseDataFactory(); }
    MCORE_INLINE AZ::Render::RenderActorSettings& GetRenderActorSettings() { return *GetEMotionFX().GetRenderActorSettings(); }/**< Get the render actor settings. */
}   // namespace EMotionFX
