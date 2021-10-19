/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/StackTracer.h>

namespace AZ
{
    namespace Debug
    {
        unsigned int StackRecorder::Record(StackFrame*, unsigned int, unsigned int, void*) 
        { 
            return false; 
        }

        unsigned int StackConverter::FromNative(StackFrame*, unsigned int, void*)
        {
            return 0;
        }

        void SymbolStorage::LoadModuleData(const void*, unsigned int)
        {}

        void SymbolStorage::StoreModuleInfoData(void*, unsigned int)
        {}

        unsigned int SymbolStorage::GetNumLoadedModules()
        {
            return 0; 
        }

        const SymbolStorage::ModuleInfo* SymbolStorage::GetModuleInfo(unsigned int)
        {
            return 0;
        }

        void SymbolStorage::RegisterModuleListeners()
        {
        }

        void SymbolStorage::UnregisterModuleListeners()
        {
        }

        void SymbolStorage::SetMapFilename(const char*)
        {
        }

        const char* SymbolStorage::GetMapFilename()
        {
            return 0;
        }

        void SymbolStorage::DecodeFrames(const StackFrame*, unsigned int, StackLine*)
        {}
    }
}
