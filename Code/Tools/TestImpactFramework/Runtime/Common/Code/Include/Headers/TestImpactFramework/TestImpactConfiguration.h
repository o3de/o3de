/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>
#include <TestImpactFramework/TestImpactTestSequence.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Meta-data about the configuration. 
    struct ConfigMeta
    {
        AZStd::string m_platform; //!< The platform for which the configuration pertains to.
        AZStd::string m_buildConfig; //!< The build configuration of the binaries.
    };

    //! Repository configuration.
    struct RepoConfig
    {
        RepoPath m_root; //!< The absolute path to the repository root.
        RepoPath m_build; //!< The absolute path to the build configuration's binary output directory.
    };

    //! Temporary workspace configuration.
    struct ArtifactDir
    {
        RepoPath m_testRunArtifactDirectory; //!< Path to read and write test run artifacts to and from.
        RepoPath m_coverageArtifactDirectory; //!< Path to read and write coverage artifacts to and from.
        RepoPath m_enumerationCacheDirectory; //!< Path to the test enumerations cache.
    };

    //! Test impact analysis framework workspace configuration.
    struct WorkspaceConfig
    {
        //! Active persistent data workspace configuration.
        struct Active
        {
            RepoPath m_root; //!< Path to the persistent workspace tracked by the repository.
            RepoPath m_sparTiaFile; //!< Paths to the test impact analysis data file.
        };

        ArtifactDir m_temp;
        Active m_active;
    };

    //! Build target descriptor configuration.
    struct BuildTargetDescriptorConfig
    {
        RepoPath m_mappingDirectory; //!< Path to the source to target mapping files.
        AZStd::vector<AZStd::string> m_staticInclusionFilters; //!< File extensions to include for static files.
        AZStd::string m_inputOutputPairer; //!< Regex for matching autogen input files with autogen outputs files.
        AZStd::vector<AZStd::string> m_inputInclusionFilters; //!< File extensions fo include for autogen input files.
    };

    //! Dependency graph configuration.
    struct DependencyGraphDataConfig
    {
        RepoPath m_graphDirectory; //!< Path to the dependency graph files. 
        AZStd::string m_targetDependencyFileMatcher; //!< Regex for matching dependency graph files to build targets.
        AZStd::string m_targetVertexMatcher; //!< Regex form matching dependency graph vertices to build targets.
    };

    //! Test target meta configuration.
    struct TestTargetMetaConfig
    {
        RepoPath m_metaFile; //!< Path to the test target meta file.
    };

    //! Gem target configuration.
    struct GemTargetConfig
    {
        RepoPath m_metaFile; //!< Path to the gem target file.
    };

    //! Test target excluded from test selection.
    struct ExcludedTarget
    {
        AZStd::string m_name; //!< Name of test target to exclude.
        AZStd::vector<AZStd::string> m_excludedTests; //!< Specific tests to exclude (if empty, all tests are excluded).
    };

    using ExcludedTargets = AZStd::vector<ExcludedTarget>;

    struct RuntimeConfig
    {
        ConfigMeta m_meta;
        RepoConfig m_repo;
        BuildTargetDescriptorConfig m_buildTargetDescriptor;
        DependencyGraphDataConfig m_dependencyGraphData;
        TestTargetMetaConfig m_testTargetMeta;
        GemTargetConfig m_gemTarget;
    };
} // namespace TestImpact
