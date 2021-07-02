/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/osstring.h>

namespace AZ
{
    class DynamicModuleDescriptor;

    namespace Internal
    {
        /**
         * Helper class to attempt to set a module path in a platform specific way (if possible) so that
         * modules can be located with attempting to do a load library
         */
        class ModuleManagerSearchPathTool
        {
        public:
            ModuleManagerSearchPathTool();
            ~ModuleManagerSearchPathTool();

            /// Attempts to set the system's current module search path
            /// \param moduleDesc   The module descriptor to extract the path to set the module search path
            void SetModuleSearchPath(const AZ::DynamicModuleDescriptor& moduleDesc);
        private:
            /// Given a dynamic module descriptor, parse out the folder it resides in in order to
            /// set it as a module search path
            /// \param moduleDesc   The module descriptor to extract the path from where it resides
            AZ::OSString GetModuleDirectory(const AZ::DynamicModuleDescriptor& moduleDesc);

            AZ::OSString m_restorePath;
        };
    } // namespace Internal
} // namespace AZ
