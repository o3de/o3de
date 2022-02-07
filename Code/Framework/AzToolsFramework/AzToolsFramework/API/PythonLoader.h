/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzToolsFramework::EmbeddedPython
{
    // When using embedded Python, some platforms need to explicitly load the python library.
    // For any modules that depend on 3rdParty::Python package, the AZ::Module should inherit this class.
    class PythonLoader
    {
    public:
        PythonLoader();
        ~PythonLoader();

    private:
        [[maybe_unused]] void* m_embeddedLibPythonHandle{ nullptr };
    };

} // namespace AzToolsFramework::EmbeddedPython
