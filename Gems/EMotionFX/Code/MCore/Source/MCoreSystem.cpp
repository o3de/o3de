/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        m_memAllocFunction       = StandardAllocate;
        m_memReallocFunction     = StandardRealloc;
        m_memFreeFunction        = StandardFree;
        m_trackMemoryUsage       = false;                // do not track memory usage on default, for maximum performance and pretty much zero tracking overhead
    }

    // static main init method
    bool Initializer::Init(InitSettings* settings)
    {
        // use the defaults if a nullptr is specified
        InitSettings defaultSettings;
        InitSettings* realSettings = (settings) ? settings : &defaultSettings;

        // create the gMCore object
        if (!gMCore)
        {
            // create the main core object using placement new
            gMCore = AZ::Environment::CreateVariable<MCoreSystem*>(kMCoreInstanceVarName);
            gMCore.Set(new(realSettings->m_memAllocFunction(sizeof(MCoreSystem), MCORE_MEMCATEGORY_MCORESYSTEM, 0, MCORE_FILE, MCORE_LINE))MCoreSystem());
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
    }

    //-----------------------------------------------------------------

    // constructor
    MCoreSystem::MCoreSystem()
        : m_allocateFunction(StandardAllocate)
        , m_reallocFunction(StandardRealloc)
        , m_freeFunction(StandardFree)
    {
        m_logManager         = nullptr;
        m_idGenerator        = nullptr;
        m_stringIdPool       = nullptr;
        m_attributeFactory   = nullptr;
        m_memoryTracker      = nullptr;
        m_memTempBuffer      = nullptr;
        m_memTempBufferSize  = 0;
        m_trackMemory        = true;
        m_memoryMutex        = new Mutex();
    }


    // destructor
    MCoreSystem::~MCoreSystem()
    {
        Shutdown();
    }


    // init the mcore system
    bool MCoreSystem::Init(const MCore::Initializer::InitSettings& settings)
    {
        if (settings.m_memAllocFunction)
        {
            m_allocateFunction = settings.m_memAllocFunction;
        }
        else
        {
            m_allocateFunction = StandardAllocate;
        }
        if (settings.m_memReallocFunction)
        {
            m_reallocFunction = settings.m_memReallocFunction;
        }
        else
        {
            m_reallocFunction = StandardRealloc;
        }
        if (settings.m_memFreeFunction)
        {
            m_freeFunction = settings.m_memFreeFunction;
        }
        else
        {
            m_freeFunction = StandardFree;
        }

        // allocate new objects
        m_memoryTracker      = new MemoryTracker();
        m_trackMemory        = settings.m_trackMemoryUsage;
        m_logManager         = new LogManager();
        m_idGenerator        = new IDGenerator();
        m_stringIdPool       = new StringIdPool();
        m_attributeFactory   = new AttributeFactory();
        m_memTempBufferSize  = 256 * 1024;
        m_memTempBuffer      = Allocate(m_memTempBufferSize, MCORE_MEMCATEGORY_SYSTEM);// 256 kb
        MCORE_ASSERT(m_memTempBuffer);

        if (m_trackMemory)
        {
            RegisterMemoryCategories(*m_memoryTracker);
        }

        return true;
    }


    // shutdown the mcore system
    void MCoreSystem::Shutdown()
    {
        // free any mem temp buffer
        MCore::Free(m_memTempBuffer);
        m_memTempBuffer = nullptr;
        m_memTempBufferSize = 0;

        // shutdown the log manager
        delete m_logManager;
        m_logManager = nullptr;

        // delete the ID generator
        delete m_idGenerator;
        m_idGenerator = nullptr;

        // Delete the string based ID generator.
        delete m_stringIdPool;
        m_stringIdPool = nullptr;

        // delete the attribute factory
        delete m_attributeFactory;
        m_attributeFactory = nullptr;

        // Clear the memory of the file system secure save path.
        FileSystem::s_secureSavePath.clear();

        // log memory leaks
        if (m_trackMemory)
        {
            m_memoryTracker->LogLeaks();
        }

        // delete the memory tracker
        delete m_memoryTracker;
        m_memoryTracker = nullptr;

        delete m_memoryMutex;
        m_memoryMutex = nullptr;
    }


    // make sure the temp buffer is of a given minimum size
    void MCoreSystem::MemTempBufferAssureSize(size_t numBytes)
    {
        // if the buffer is already big enough, we can just return
        if (m_memTempBufferSize >= numBytes)
        {
            return;
        }

        // resize the buffer (make it bigger)
        m_memTempBuffer = Realloc(m_memTempBuffer, numBytes, MCORE_MEMCATEGORY_SYSTEM);
        MCORE_ASSERT(m_memTempBuffer);

        m_memTempBufferSize = numBytes;
    }


    // free the temp buffer
    void MCoreSystem::MemTempBufferFree()
    {
        MCore::Free(m_memTempBuffer);
        m_memTempBuffer = nullptr;
        m_memTempBufferSize = 0;
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
