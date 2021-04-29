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

#include <Artifact/Factory/TestImpactModuleCoverageFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/XML/rapidxml.h>
#include <AzCore/std/string/conversions.h>

namespace TestImpact
{
    namespace
    {
        // Keys for pertinent XML node and attribute names
        constexpr const char* Keys[] =
        {
            "packages",
            "name",
            "filename",
            "coverage",
            "classes",
            "lines",
            "line",
            "number",
            "hits",
            "sources",
            "source"
        };

        enum
        {
            PackagesKey,
            NameKey,
            FileNameKey,
            CoverageKey,
            ClassesKey,
            LinesKey,
            LineKey,
            NumberKey,
            HitsKey,
            SourcesKey,
            SourceKey
        };
    } // namespace

    namespace Cobertura
    {
        // Note: OpenCppCoverage appears to have a very liberal interpretation of the Cobertura coverage file format so consider
        // this implementation to be provisional and coupled to the Windows platform and OpenCppCoverage tool
        AZStd::vector<ModuleCoverage> ModuleCoveragesFactory(const AZStd::string& coverageData)
        {
            AZ_TestImpact_Eval(!coverageData.empty(), ArtifactException, "Cannot parse coverage, string is empty");
            AZStd::vector<ModuleCoverage> modules;
            AZStd::vector<char> rawData(coverageData.begin(), coverageData.end());

            try
            {
                AZ::rapidxml::xml_document<> doc;
                // Parse the XML doc with default flags
                doc.parse<0>(rawData.data());

                // Coverage
                const auto coverage_node = doc.first_node(Keys[CoverageKey]);
                AZ_TestImpact_Eval(coverage_node, ArtifactException, "Could not parse coverage node");

                // Sources
                const auto sources_node = coverage_node->first_node(Keys[SourcesKey]);
                if (!sources_node)
                {
                    return {};
                }

                // Source
                const auto source_node = sources_node->first_node(Keys[SourceKey]);
                if (!source_node)
                {
                    return {};
                }

                // Root drive (this seems to be an unconventional use of the sources section by OpenCppCoverage)
                const AZStd::string pathRoot = AZStd::string(source_node->value(), source_node->value() + source_node->value_size()) + "\\";

                const auto packages_node = coverage_node->first_node(Keys[PackagesKey]);
                if (packages_node)
                {
                    // Modules
                    for (auto package_node = packages_node->first_node(); package_node; package_node = package_node->next_sibling())
                    {
                        // Module
                        ModuleCoverage moduleCoverage;
                        moduleCoverage.m_path = package_node->first_attribute(Keys[NameKey])->value();

                        const auto classes_node = package_node->first_node(Keys[ClassesKey]);
                        if (classes_node)
                        {
                            // Sources
                            for (auto class_node = classes_node->first_node(); class_node; class_node = class_node->next_sibling())
                            {
                                // Source
                                SourceCoverage sourceCoverage;
                                sourceCoverage.m_path = pathRoot + class_node->first_attribute(Keys[FileNameKey])->value();

                                const auto lines_node = class_node->first_node(Keys[LinesKey]);
                                if (lines_node)
                                {
                                    // Lines
                                    for (auto line_node = lines_node->first_node(); line_node; line_node = line_node->next_sibling())
                                    {
                                        // Line
                                        const size_t number =
                                            AZStd::stol(AZStd::string(line_node->first_attribute(Keys[NumberKey])->value()));
                                        const size_t hits = AZStd::stol(AZStd::string(line_node->first_attribute(Keys[HitsKey])->value()));
                                        sourceCoverage.m_coverage.emplace_back(LineCoverage{number, hits});
                                    }
                                }

                                moduleCoverage.m_sources.emplace_back(AZStd::move(sourceCoverage));
                            }
                        }

                        modules.emplace_back(AZStd::move(moduleCoverage));
                    }
                }
            }
            catch (const std::exception& e)
            {
                AZ_Error("ModuleCoveragesFactory", false, e.what());
                throw ArtifactException(e.what());
            }
            catch (...)
            {
                throw ArtifactException("An unknown error occurred parsing the XML data");
            }

            return modules;
        }
    } // namespace Cobertura
} // namespace TestImpact
