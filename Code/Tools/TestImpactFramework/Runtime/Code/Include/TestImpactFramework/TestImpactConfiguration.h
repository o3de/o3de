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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/Path/Path.h>

namespace TestImpact
{
    struct ConfigMeta
    {
        AZStd::string m_platform;
        AZStd::string m_version;
        AZStd::string m_timestamp;
    };

    struct RepoConfig
    {
        AZ::IO::Path m_root;
    };

    struct WorkspaceConfig
    {
        //AZ::IO::Path m_tempDirectory;
        //AZ::IO::Path m_testImpactDataFile;
        struct Temp
        {
            AZ::IO::Path m_root;
            AZ::IO::Path m_artifactDirectory;
        };

        struct Persistent
        {
            AZ::IO::Path m_root;
            AZ::IO::Path m_sparTIAFile;
            AZ::IO::Path m_enumerationCacheDirectory;
        };

        Temp m_temp;
        Persistent m_persistent;
    };

    struct BuildTargetDescriptorConfig
    {
        AZ::IO::Path m_outputDirectory;
        AZStd::vector<AZStd::string> m_staticInclusionFilters;
        AZStd::string m_inputOutputPairer;
        AZStd::vector<AZStd::string> m_inputInclusionFilters;
    };

    struct DependencyGraphDataConfig
    {
        AZ::IO::Path m_outputDirectory;
        AZStd::string m_targetDependencyFileMatcher;
        AZStd::string m_targetVertexMatcher;
    };

    struct TestTargetMetaConfig
    {
        AZ::IO::Path m_file;
    };

    struct TestEngineConfig
    {
        struct TestRunner
        {
            AZ::IO::Path m_binary;
        };

        struct Instrumentation
        {
            AZ::IO::Path m_binary;
        };

        TestRunner m_testRunner;
        Instrumentation m_instrumentation;
    };

    struct TargetConfig
    {  
        struct ShardedTarget
        {
            AZStd::string m_name;
            AZStd::string m_policy;
        };

        AZ::IO::Path m_outputDirectory;
        AZStd::vector<AZStd::string> m_excludedTestTargets;
        AZStd::vector<ShardedTarget> m_shardedTestTargets;
    };

    struct RuntimeConfig
    {
        ConfigMeta m_meta;
        RepoConfig m_repo;
        WorkspaceConfig m_workspace;
        BuildTargetDescriptorConfig m_buildTargetDescriptor;
        DependencyGraphDataConfig m_dependencyGraphData;
        TestTargetMetaConfig m_testTargetMeta;
        TestEngineConfig m_testEngine;
        TargetConfig m_target;
    };
}
