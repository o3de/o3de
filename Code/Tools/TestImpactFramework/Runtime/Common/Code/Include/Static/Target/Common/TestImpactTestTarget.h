/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactTestTargetMeta.h>
#include <Target/Common/TestImpactTarget.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //! Representation of a generic test target in the repository.
    class TestTarget 
        : public Target
    {
    public:
        TestTarget(TargetDescriptor&& descriptor, TestTargetMeta&& testTargetMeta);

        //! Returns the test target suite.
        const AZStd::string& GetSuite() const;

        //! Returns the test run timeout.
        AZStd::chrono::milliseconds GetTimeout() const;

        //! Returns the namespace this test target resides in (if any).
        const AZStd::string& GetNamespace() const;

        //! Returns the suite label set.
        const SuiteLabelSet& GetSuiteLabelSet() const;

        //! Returns `true` if the test target can enumerate its tests, otherwise `false`.
        virtual bool CanEnumerate() const = 0; 

    private:
        TestTargetMeta m_testTargetMeta;
    };
} // namespace TestImpact
