/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

// include required headers
#include "Config.h"

#include <AzCore/std/functional.h>
#include <AzCore/Module/Environment.h>


namespace MCore
{
    // forward declarations
    class LogManager;
    class IDGenerator;
    class StringIdPool;
    class AttributeFactory;
    class MemoryTracker;
    class Mutex;

    typedef void* (MCORE_CDECL * AllocateCallback)(size_t numBytes, uint16 categoryID, uint16 blockID, const char* filename, uint32 lineNr);
    typedef void* (MCORE_CDECL * ReallocCallback)(void* memory, size_t numBytes, uint16 categoryID, uint16 blockID, const char* filename, uint32 lineNr);
    typedef void  (MCORE_CDECL * FreeCallback)(void* memory);

    /**
     * The MCore system initializer.
     * This class handles the initialization and shutdown of the core system.
     */
    class MCORE_API Initializer
    {
    public:
        struct MCORE_API InitSettings
        {
            AllocateCallback            mMemAllocFunction;          /**< The memory allocation function, defaults to nullptr, which means the standard malloc function will be used. */
            ReallocCallback             mMemReallocFunction;        /**< The memory reallocation function, defaults to nullptr, which means the standard realloc function will be used. */
            FreeCallback                mMemFreeFunction;           /**< The memory free function, defaults to nullptr, which means the standard free function will be used. */
            bool                        mTrackMemoryUsage;          /**< Enable this to track memory usage statistics. This has a bit of an impact on memory allocation and release speed and memory usage though. You should really only use this in debug mode. On default it is disabled. */

            InitSettings();
        };

        /**
         * The init function, which creates a new MCore::gMCore object.
         * Initializing twice is no problem, in this case nothing will happen. Unless you called Shutdown before your next Init of course.
         * @param initSettings A pointer to the init settings struct, or nullptr to use all default settings.
         * @result Returns true when successful or false when the init has failed for some reason.
         */
        static bool MCORE_CDECL Init(InitSettings* initSettings = nullptr);

        /**
         * The shutdown method, which destructs the MCore::gMCore object.
         * Please keep in mind that you do not have any more allocations such as string or array allocations remaining
         * after you call the Shutdown method. Also don't create any String or Array allocations before MCore::Init() has been called.
         */
        static void MCORE_CDECL Shutdown();
    };


    /**
     * The core main system, which stores some manager objects.
     * This includes a memory manager, coordinate system converter and log manager.
     * An instance from this class can be accessed with the MCore::gMCore global variable.
     * This global object is created when you call MCore::Init() and it is deleted when you call MCore::Shutdown().
     * Please keep in mind that you do not have any more allocations such as string or array allocations remaining
     * after you call the Shutdown method. Also don't create any String or Array allocations before MCore::Init() has been called.
     */
    class MCORE_API MCoreSystem
    {
        friend class Initializer;

    public:
        /**
         * Get the log manager.
         * @result A reference to the log manager.
         */
        MCORE_INLINE LogManager& GetLogManager()                        { return *mLogManager; }

        /**
         * Get the ID generator.
         * @result A reference to the ID generator.
         */
        MCORE_INLINE IDGenerator& GetIDGenerator()                      { return *mIDGenerator; }

        /**
         * Get the string based ID generator.
         * @result A reference to the string based ID generator.
         */
        MCORE_INLINE StringIdPool& GetStringIdPool()          { return *mStringIdPool; }

        /**
         * Get the attribute factory.
         * @result A reference to the attribute factory, which is used to create attributes of a certain type.
         */
        MCORE_INLINE AttributeFactory& GetAttributeFactory()            { return *mAttributeFactory; }

        /**
         * Get the memory tracker.
         * @result A reference to the memory tracker, which can be used to track memory allocations and usage.
         */
        MCORE_INLINE MemoryTracker& GetMemoryTracker()                  { return *mMemoryTracker; }
        MCORE_INLINE bool GetIsTrackingMemory() const                   { return mTrackMemory; }

        MCORE_INLINE void* GetMemTempBuffer()                           { return mMemTempBuffer; }
        MCORE_INLINE size_t GetMemTempBufferSize() const                { return mMemTempBufferSize; }
        void MemTempBufferAssureSize(size_t numBytes);
        void MemTempBufferFree();

        void RegisterMemoryCategories(MemoryTracker& memTracker);

        MCORE_INLINE Mutex& GetMemoryMutex()                            { return *mMemoryMutex; }

        MCORE_INLINE AllocateCallback GetAllocateFunction()             { return mAllocateFunction; }
        MCORE_INLINE ReallocCallback GetReallocFunction()               { return mReallocFunction; }
        MCORE_INLINE FreeCallback GetFreeFunction()                     { return mFreeFunction; }

    private:
        LogManager*             mLogManager;        /**< The log manager. */
        IDGenerator*            mIDGenerator;       /**< The ID generator. */
        StringIdPool*           mStringIdPool; /**< The string based ID generator. */
        AttributeFactory*       mAttributeFactory;  /**< The attribute factory. */
        MemoryTracker*          mMemoryTracker;     /**< The memory tracker. */
        Mutex*                  mMemoryMutex;
        AllocateCallback        mAllocateFunction;
        ReallocCallback         mReallocFunction;
        FreeCallback            mFreeFunction;
        void*                   mMemTempBuffer;     /**< A buffer with temp memory, used by the MCore::AlignedRealloc, to assure data integrity after reallocating memory. */
        size_t                  mMemTempBufferSize; /**< The size in bytes, of the MemTempBuffer. */
        bool                    mTrackMemory;       /**< Check if we want to track memory or not. */

        /**
         * The constructor.
         * This creates all default managers.
         */
        MCoreSystem();

        /**
         * The destructor.
         * Automatically destroys all currently set managers.
         */
        ~MCoreSystem();

        /**
         * Initialize the MCore system.
         */
        bool Init(const MCore::Initializer::InitSettings& settings);

        /**
         * Shutdown the MCore system.
         */
        void Shutdown();
    };

    //-----------------------------------------------------------------------------


    /**
     * The global core system that contains the managers, such as log manager, coordinate system converter etc.
     * Use MCore::Initializer::Init() and MCore::Initializer::Shutdown() to init and shutdown MCore.
     */
    extern MCORE_API AZ::EnvironmentVariable<MCoreSystem*> gMCore;
    static const char* kMCoreInstanceVarName = "MCoreInstance";


    // shortcuts to access given managers/systems
    MCORE_INLINE MCoreSystem& GetMCore()
    {
        if (!gMCore)
        {
            gMCore = AZ::Environment::FindVariable<MCoreSystem*>(kMCoreInstanceVarName);
        }
        return *(gMCore.Get());
    }

    MCORE_INLINE LogManager& GetLogManager()                   { return GetMCore().GetLogManager(); }
    MCORE_INLINE IDGenerator& GetIDGenerator()                 { return GetMCore().GetIDGenerator(); }
    MCORE_INLINE StringIdPool& GetStringIdPool()               { return GetMCore().GetStringIdPool(); }
    MCORE_INLINE AttributeFactory& GetAttributeFactory()       { return GetMCore().GetAttributeFactory(); }
    MCORE_INLINE MemoryTracker& GetMemoryTracker()             { return GetMCore().GetMemoryTracker(); }
} // namespace MCore