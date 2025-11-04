/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/Factory/TestImpactModuleCoverageFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/XML/rapidxml.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace TestImpact
{
    namespace Cobertura
    {
        // Note: OpenCppCoverage appears to have a very liberal interpretation of the Cobertura coverage file format so consider
        // this implementation to be provisional and coupled to the Windows platform and OpenCppCoverage tool
        AZStd::vector<ModuleCoverage> ModuleCoveragesFactory(const AZStd::string& coverageData)
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

            enum Fields
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
                SourceKey,
                // Checksum
                _CHECKSUM_
            };

            static_assert(Fields::_CHECKSUM_ == AZStd::size(Keys));

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
                                        sourceCoverage.m_lineCoverage.emplace_back(LineCoverage{number, hits});
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

    namespace PythonCoverage
    {
        PythonModuleCoverage ModuleCoveragesFactory(const AZStd::string& coverageData)
        {
            PythonModuleCoverage coverage;

            enum
            {
                ParentScript,
                CallingScript,
                Fixture,
                TestCase,
                ModuleCoverage
            };

            AZStd::vector<AZStd::string> lines;
            AZ::StringFunc::Tokenize(coverageData, lines, '\n');

            coverage.m_testSuite = lines[Fixture];
            coverage.m_testCase = lines[TestCase];

            for (auto i = static_cast<size_t>(ModuleCoverage); i < lines.size(); i++)
            {
                const auto& line = lines[i];
                if (line[0] == '\0')
                {
                    continue;
                }

                coverage.m_components.push_back(line);
            }

            return coverage;
        }
    } // namespace PythonCoverage
} // namespace TestImpact
