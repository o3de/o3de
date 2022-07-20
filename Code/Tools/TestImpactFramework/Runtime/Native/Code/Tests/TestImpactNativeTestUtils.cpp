/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactNativeTestUtils.h>

#include <TestImpactFramework/TestImpactException.h>

namespace UnitTest
{
    AZStd::string GenerateNativeTargetDescriptorString(
        const AZStd::string& name,
        const AZStd::string& outputName,
        const TestImpact::RepoPath& path,
        const AZStd::vector<TestImpact::RepoPath>& staticSources,
        const AZStd::vector<TestImpact::RepoPath>& autogenInputs,
        const AZStd::vector<TestImpact::RepoPath>& autogenOutputs)
    {
        constexpr const char* const targetTemplate =
            "{\n"
            "    \"sources\": {\n"
            "        \"input\": [\n"
            "%s\n"
            "        ],\n"
            "        \"output\": [\n"
            "%s\n"
            "        ],\n"
            "        \"static\": [\n"
            "%s\n"
            "        ]\n"
            "    },\n"
            "    \"target\": {\n"
            "        \"name\": \"%s\",\n"
            "        \"output_name\": \"%s\",\n"
            "        \"path\": \"%s\"\n"
            "    }\n"
            "}\n"
            "\n";

        AZStd::string output = AZStd::string::format(targetTemplate,
            StringVectorToJSONElements(autogenInputs).c_str(),
            StringVectorToJSONElements(autogenOutputs).c_str(),
            StringVectorToJSONElements(staticSources).c_str(),
            name.c_str(),
            outputName.c_str(),
            JSONSafeString(path.String()).c_str());

        return output;
    }

    TestImpact::NativeTargetDescriptor GenerateNativeTargetDescriptor(
        const AZStd::string& name,
        const AZStd::string& outputName,
        const TestImpact::RepoPath& path,
        const AZStd::vector<TestImpact::RepoPath>& staticSources,
        const TestImpact::AutogenSources& autogenSources)
    {
        return TestImpact::NativeTargetDescriptor(TestImpact::TargetDescriptor{ name, path, TestImpact::TargetSources{ staticSources, autogenSources } }, outputName);
    }

    bool operator==(const TestImpact::NativeTargetDescriptor& lhs, const TestImpact::NativeTargetDescriptor& rhs)
    {
        return lhs.m_outputName == rhs.m_outputName &&
            static_cast<const TestImpact::TargetDescriptor&>(lhs) == static_cast<const TestImpact::TargetDescriptor&>(rhs);
    }

    bool operator==(const TestImpact::NativeTestTargetMeta& lhs, const TestImpact::NativeTestTargetMeta& rhs)
    {
        if (!(lhs.m_suiteMeta == rhs.m_suiteMeta))
        {
            return false;
        }
        else if (lhs.m_launchMethod != rhs.m_launchMethod)
        {
            return false;
        }

        return true;
    }

    bool operator==(const TestImpact::NativeProductionTargetDescriptor& lhs, const TestImpact::NativeProductionTargetDescriptor& rhs)
    {
        return static_cast<const TestImpact::NativeTargetDescriptor&>(lhs) == static_cast<const TestImpact::NativeTargetDescriptor&>(rhs);
    }

    bool operator==(const TestImpact::NativeTestTargetDescriptor& lhs, const TestImpact::NativeTestTargetDescriptor& rhs)
    {
        return static_cast<const TestImpact::NativeTargetDescriptor&>(lhs) == static_cast<const TestImpact::NativeTargetDescriptor&>(rhs) &&
            lhs.m_testMetaData == rhs.m_testMetaData;
    }
} // namespace UnitTest
