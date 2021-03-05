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

#include <AzCore/Debug/StackTracer.h>

namespace AZ
{
    namespace Debug
    {
        unsigned int StackRecorder::Record(StackFrame*, unsigned int, unsigned int, void*) 
        { 
            return false; 
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
