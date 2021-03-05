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

#include <AzCore/base.h>
#include <AzCore/AzCore_Traits_Platform.h>

namespace AZ
{
    namespace Debug
    {
        struct StackFrame
        {
            StackFrame()
                : m_programCounter(0)  {}
            AZ_FORCE_INLINE bool IsValid() const { return m_programCounter != 0; }

            uintptr_t  m_programCounter;       ///< Currently we store and use only the program counter.
        };

        /**
         *
         */
        class StackRecorder
        {
        public:
            /**
            * Record the current call stack frames (current process, current thread).
            * \param frames array of frames to store the stack into.
            * \param suppressCount This specifies how many levels of the stack to hide. By default it is 0,
            *                      which will just hide this function itself.
            * \param nativeThread pointer to thread native type to capture a stack other than the currently running stack
            */
            static unsigned int Record(StackFrame* frames, unsigned int maxNumOfFrames, unsigned int suppressCount = 0, void* nativeThread = 0);
        };

        class SymbolStorage
        {
        public:
            struct ModuleDataInfoHeader
            {
                // We use chars so we don't need to care about big-little endian.
                char    m_platformId;
                char    m_numModules;
            };

            struct ModuleInfo
            {
                char    m_modName[256];
                char    m_fileName[1024];
                u64     m_fileVersion;
                u64     m_baseAddress;
                u32     m_size;
            };

            typedef char StackLine[256];

            /**
             * Use to load module info data captured at a different system.
             */
            /// Load module data symbols (deprecated platform export)
            static void         LoadModuleData(const void* moduleInfoData, unsigned int moduleInfoDataSize);

            /// Return pointer to the data with module information. Data is platform dependent
            static void         StoreModuleInfoData(void* data, unsigned int dataSize);

            /// Return number of loaded modules.
            static unsigned int GetNumLoadedModules();
            /// Return information for a loaded module.
            static const ModuleInfo*    GetModuleInfo(unsigned int moduleIndex);

            /// Set/Get map filename or symbol path, used to decode frames or consoles or when no other symbol information is available.
            static void         SetMapFilename(const char* fileName);
            static const char*  GetMapFilename();

            /// Registers listeners for dynamically loaded modules used for loading correct symbols for SymbolStorage
            static void         RegisterModuleListeners();
            static void         UnregisterModuleListeners();

            /**
             * Decode frames into a readable text line.
             * IMPORTANT: textLines should point to an array of StackLine at least numFrames long.
             */
            static void     DecodeFrames(const StackFrame* frames, unsigned int numFrames, StackLine* textLines);

            static void     FindFunctionFromIP(void* address, StackLine* func, StackLine* file, StackLine* module, int& line, void*& baseAddr);
        };

    } // namespace Debug
} // namespace AZ
