/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactPythonTestTargetMeta.h>
#include <Target/Common/TestImpactTestTarget.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //! Build target specialization for native test targets (build targets containing test code and no production code).
    class PythonTestTarget 
        : public TestTarget
    {
    public:
        PythonTestTarget(TargetDescriptor&& descriptor, PythonTestTargetMeta&& testMetaData);

        //! Returns the path to the script to execute this test.
        const RepoPath& GetScriptPath() const;

        //! Returns the command to execute this test.
        const AZStd::string& GetCommand() const;

        // TestTarget overrides ...
        bool CanEnumerate() const override;

    private:
        PythonTargetScriptMeta m_scriptMetaData;
    };
} // namespace TestImpact
