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
