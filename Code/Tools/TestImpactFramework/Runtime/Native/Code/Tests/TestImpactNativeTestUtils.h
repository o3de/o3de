/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>
#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/Static/TestImpactNativeTargetDescriptor.h>
#include <Artifact/Static/TestImpactNativeProductionTargetDescriptor.h>
#include <Artifact/Static/TestImpactNativeTestTargetDescriptor.h>
#include <Artifact/Static/TestImpactNativeTestTargetMeta.h>

namespace UnitTest
{
    // Generate a build target descriptor string in JSON format from the specified build target description
    AZStd::string GenerateNativeTargetDescriptorString(
        const AZStd::string& name,
        const AZStd::string& outputName,
        const TestImpact::RepoPath& path,
        const AZStd::vector<TestImpact::RepoPath>& staticSources,
        const AZStd::vector<TestImpact::RepoPath>& autogenInputs,
        const AZStd::vector<TestImpact::RepoPath>& autogenOutputs);

    // Generate a build target descriptor from the specified build target description
    // Note: no check for correctness of arguments is peformed
    TestImpact::NativeTargetDescriptor GenerateNativeTargetDescriptor(
        const AZStd::string& name,
        const AZStd::string& outputName,
        const TestImpact::RepoPath& path,
        const AZStd::vector<TestImpact::RepoPath>& staticSources,
        const TestImpact::AutogenSources& autogenSources);

    bool operator==(const TestImpact::NativeTargetDescriptor& lhs, const TestImpact::NativeTargetDescriptor& rhs);
    bool operator==(const TestImpact::NativeTestTargetMeta& lhs, const TestImpact::NativeTestTargetMeta& rhs);
    bool operator==(const TestImpact::NativeProductionTargetDescriptor& lhs, const TestImpact::NativeProductionTargetDescriptor& rhs);
    bool operator==(const TestImpact::NativeTestTargetDescriptor& lhs, const TestImpact::NativeTestTargetDescriptor& rhs);
} // namespace UnitTest
