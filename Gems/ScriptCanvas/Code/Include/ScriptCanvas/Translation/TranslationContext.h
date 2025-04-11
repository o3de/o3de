/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include "Translation.h"

namespace ScriptCanvas
{
    namespace Translation
    {
        // put Translation state globals in here, things that can be useful across several parses of graphs
        class Context
        {
        public:
            AZ_CLASS_ALLOCATOR(Context, AZ::SystemAllocator);

            static AZStd::string GetCategoryLibraryName(const AZStd::string& categoryName);

            Context() = default;
            ~Context() = default;
            
            const AZStd::string& FindAbbreviation(AZStd::string_view dependency) const;
            const AZStd::string& FindLibrary(AZStd::string_view dependency) const;

        protected:
            void InitializeNames();

        private:
            AZStd::unordered_map<AZStd::string, AZStd::string> m_tableAbbreviations;
            AZStd::unordered_map<AZStd::string, AZStd::string> m_categoryToLibraryName;
        };
    } 

} 
