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

// include the required headers
#include "MCoreSystem.h"
#include "MemoryManager.h"
#include "LogManager.h"
#include "MemoryCategoriesCore.h"
#include "IDGenerator.h"
#include "StringIdPool.h"
#include "AttributeFactory.h"
#include "MultiThreadManager.h"
#include "MemoryTracker.h"
#include "FileSystem.h"
#include <MCore/Source/AttributeAllocator.h>


namespace MCore
{
    // some globals
    AZ::EnvironmentVariable<MCoreSystem*> gMCore;

    Initializer::InitSettings::InitSettings()
    {
        mMemAllocFunction       = StandardAllocate;
        mMemReallocFunction     = StandardRealloc;
        mMemFreeFunction        = StandardFree;
        mTrackMemoryUsage       = false;                // do not track memory usage on default, for maximum performance and pretty much zero tracking overhead
    }

    // static main init method
    bool Initializer::Init(InitSettings* settings)
    {
        AZ::AllocatorInstance<AttributeAllocator>::Create();

        // use the defaults if a nullptr is specified
        InitSettings defaultSettings;
        InitSettings* realSettings = (settings) ? settings : &defaultSettings;

        // create the gMCore object
        if (!gMCore)
        {
            // create the main core object using placement new
            gMCore = AZ::Environment::CreateVariable<MCoreSystem*>(kMCoreInstanceVarName);
            gMCore.Set(new(realSettings->mMemAllocFunction(sizeof(MCoreSystem), MCORE_MEMCATEGORY_MCORESYSTEM, 0, MCORE_FILE, MCORE_LINE))MCoreSystem());
        }
        else
        {
            return true;
        }

        // try to initialize
        return gMCore.Get()->Init(*realSettings);
    }


    // static main shutdown method
    void Initializer::Shutdown()
    {
        if (gMCore == nullptr)
        {
            return;
        }

        // delete the core system
        FreeCallback freeFunction = gMCore.Get()->GetFreeFunction();
        gMCore.Get()->~MCoreSystem();
        freeFunction(gMCore.Get());

        gMCore.Reset();

        AZ::AllocatorInstance<AttributeAllocator>::Destroy();
    }

    //-----------------------------------------------------------------

    // constructor
    MCoreSystem::MCoreSystem()
        : mAllocateFunction(StandardAllocate)
        , mReallocFunction(StandardRealloc)
        , mFreeFunction(StandardFree)
    {
        mLogManager         = nullptr;
        mIDGenerator        = nullptr;
        mStringIdPool       = nullptr;
        mAttributeFactory   = nullptr;
        mMemoryTracker      = nullptr;
        mMemTempBuffer      = nullptr;
        mMemTempBufferSize  = 0;
        mTrackMemory        = true;
        mMemoryMutex        = new Mutex();
    }


    // destructor
    MCoreSystem::~MCoreSystem()
    {
        Shutdown();
    }


    // init the mcore system
    bool MCoreSystem::Init(const MCore::Initializer::InitSettings& settings)
    {
        if (settings.mMemAllocFunction)
        {
            mAllocateFunction = settings.mMemAllocFunction;
        }
        else
        {
            mAllocateFunction = StandardAllocate;
        }
        if (settings.mMemReallocFunction)
        {
            mReallocFunction = settings.mMemReallocFunction;
        }
        else
        {
            mReallocFunction = StandardRealloc;
        }
        if (settings.mMemFreeFunction)
        {
            mFreeFunction = settings.mMemFreeFunction;
        }
        else
        {
            mFreeFunction = StandardFree;
        }

        // allocate new objects
        mMemoryTracker      = new MemoryTracker();
        mTrackMemory        = settings.mTrackMemoryUsage;
        mLogManager         = new LogManager();
        mIDGenerator        = new IDGenerator();
        mStringIdPool       = new StringIdPool();
        mAttributeFactory   = new AttributeFactory();
        mMemTempBufferSize  = 256 * 1024;
        mMemTempBuffer      = Allocate(mMemTempBufferSize, MCORE_MEMCATEGORY_SYSTEM);// 256 kb
        MCORE_ASSERT(mMemTempBuffer);

        if (mTrackMemory)
        {
            RegisterMemoryCategories(*mMemoryTracker);
        }

        return true;
    }


    // shutdown the mcore system
    void MCoreSystem::Shutdown()
    {
        // free any mem temp buffer
        MCore::Free(mMemTempBuffer);
        mMemTempBuffer = nullptr;
        mMemTempBufferSize = 0;

        // shutdown the log manager
        delete mLogManager;
        mLogManager = nullptr;

        // delete the ID generator
        delete mIDGenerator;
        mIDGenerator = nullptr;

        // Delete the string based ID generator.
        delete mStringIdPool;
        mStringIdPool = nullptr;

        // delete the attribute factory
        delete mAttributeFactory;
        mAttributeFactory = nullptr;

        // Clear the memory of the file system secure save path.
        FileSystem::mSecureSavePath.clear();

        // log memory leaks
        if (mTrackMemory)
        {
            mMemoryTracker->LogLeaks();
        }

        // delete the memory tracker
        delete mMemoryTracker;
        mMemoryTracker = nullptr;

        delete mMemoryMutex;
        mMemoryMutex = nullptr;
    }


    // make sure the temp buffer is of a given minimum size
    void MCoreSystem::MemTempBufferAssureSize(size_t numBytes)
    {
        // if the buffer is already big enough, we can just return
        if (mMemTempBufferSize >= numBytes)
        {
            return;
        }

        // resize the buffer (make it bigger)
        mMemTempBuffer = Realloc(mMemTempBuffer, numBytes, MCORE_MEMCATEGORY_SYSTEM);
        MCORE_ASSERT(mMemTempBuffer);

        mMemTempBufferSize = numBytes;
    }


    // free the temp buffer
    void MCoreSystem::MemTempBufferFree()
    {
        MCore::Free(mMemTempBuffer);
        mMemTempBuffer = nullptr;
        mMemTempBufferSize = 0;
    }


    // register the MCore memory categories
    void MCoreSystem::RegisterMemoryCategories(MemoryTracker& memTracker)
    {
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_UNKNOWN,             "MCORE_MEMCATEGORY_UNKNOWN");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_ARRAY,               "MCORE_MEMCATEGORY_ARRAY");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_STRING,              "MCORE_MEMCATEGORY_STRING");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_DISKFILE,            "MCORE_MEMCATEGORY_DISKFILE");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_MEMORYFILE,          "MCORE_MEMCATEGORY_MEMORYFILE");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_MATRIX,              "MCORE_MEMCATEGORY_MATRIX");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_HASHTABLE,           "MCORE_MEMCATEGORY_HASHTABLE");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_TRILISTOPTIMIZER,    "MCORE_MEMCATEGORY_TRILISTOPTIMIZER");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_LOGMANAGER,          "MCORE_MEMCATEGORY_LOGMANAGER");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_COMMANDLINE,         "MCORE_MEMCATEGORY_COMMANDLINE");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_LOGFILECALLBACK,     "MCORE_MEMCATEGORY_LOGFILECALLBACK");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_HALTONSEQ,           "MCORE_MEMCATEGORY_HALTONSEQ");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_SMALLARRAY,          "MCORE_MEMCATEGORY_SMALLARRAY");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_COORDSYSTEM,         "MCORE_MEMCATEGORY_COORDSYSTEM");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_MCORESYSTEM,         "MCORE_MEMCATEGORY_MCORESYSTEM");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_COMMANDSYSTEM,       "MCORE_MEMCATEGORY_COMMANDSYSTEM");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_ATTRIBUTES,          "MCORE_MEMCATEGORY_ATTRIBUTES");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_IDGENERATOR,         "MCORE_MEMCATEGORY_IDGENERATOR");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_WAVELETS,            "MCORE_MEMCATEGORY_WAVELETS");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_HUFFMAN,             "MCORE_MEMCATEGORY_HUFFMAN");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_ABSTRACTDATA,        "MCORE_MEMCATEGORY_ABSTRACTDATA");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_SYSTEM,              "MCORE_MEMCATEGORY_SYSTEM");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_THREADING,           "MCORE_MEMCATEGORY_THREADING");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_ATTRIBUTEPOOL,       "MCORE_MEMCATEGORY_ATTRIBUTEPOOL");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_ATTRIBUTEFACTORY,    "MCORE_MEMCATEGORY_ATTRIBUTEFACTORY");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_RANDOM,              "MCORE_MEMCATEGORY_RANDOM");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_STRINGOPS,           "MCORE_MEMCATEGORY_STRINGOPS");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_FRUSTUM,             "MCORE_MEMCATEGORY_FRUSTUM");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_STREAM,              "MCORE_MEMCATEGORY_STREAM");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_MULTITHREADMANAGER,  "MCORE_MEMCATEGORY_MULTITHREADMANAGER");
        memTracker.RegisterCategory(MCORE_MEMCATEGORY_MISC,                "MCORE_MEMCATEGORY_MISC");
    }
}   // namespace MCore
