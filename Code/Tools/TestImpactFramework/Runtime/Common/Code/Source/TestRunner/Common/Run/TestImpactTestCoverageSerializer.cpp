/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/Run/TestImpactTestCoverageSerializer.h>

#include <AzCore/Date/DateFormat.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/XML/rapidxml_print.h>

namespace TestImpact
{
    namespace Cobertura
    {
        // Note: OpenCppCoverage appears to have a very liberal interpretation of the Cobertura coverage file format so consider
        // this implementation to be provisional and coupled to the Windows platform and OpenCppCoverage tool
        AZStd::string SerializeTestCoverage(const TestCoverage& testCoverage, const RepoPath& repoRoot)
        {
            // Keys for pertinent XML node and attribute names
            constexpr const char* Keys[] =
            {
                "line-rate",
                "branch-rate",
                "complexity",
                "branches-covered",
                "branches-valid",
                "timestamp",
                "lines-covered",
                "lines-valid",
                "version",
                "packages",
                "package",
                "name",
                "filename",
                "coverage",
                "classes",
                "class",
                "lines",
                "line",
                "methods",
                "number",
                "hits",
                "sources",
                "source"
            };

            enum Fields
            {
                LineRateKey,
                BranchRateKey,
                ComplexityKey,
                BranchesCoveredKey,
                BranchesValidKey,
                TimestampKey,
                LinesCoveredKey,
                LinesValidKey,
                VersionKey,
                PackagesKey,
                PackageKey,
                NameKey,
                FileNameKey,
                CoverageKey,
                ClassesKey,
                ClassKey,
                LinesKey,
                LineKey,
                MethodsKey,
                NumberKey,
                HitsKey,
                SourcesKey,
                SourceKey,
                // Checksum
                _CHECKSUM_
            };

            static_assert(Fields::_CHECKSUM_ == AZStd::size(Keys));
            AZ::rapidxml::xml_document<> doc;

            AZ::rapidxml::xml_node<>* decl = doc.allocate_node(AZ::rapidxml::node_declaration);
            decl->append_attribute(doc.allocate_attribute("version", "1.0"));
            decl->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
            doc.append_node(decl);

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            AZ::Date::GetIso8601ExtendedFormatNow(iso8601Timestamp);

            AZ::rapidxml::xml_node<>* rootNode = doc.allocate_node(AZ::rapidxml::node_element, Keys[CoverageKey]);
            doc.append_node(rootNode);

            // Line rate (not supported yet at the test level, even though TestCoverage supports this metric)
            rootNode->append_attribute(doc.allocate_attribute(Keys[LineRateKey], "1"));

            // Branch rate (not supported)
            rootNode->append_attribute(doc.allocate_attribute(Keys[BranchRateKey], "0"));

            // Complexity rate (not supported)
            rootNode->append_attribute(doc.allocate_attribute(Keys[ComplexityKey], "0"));

            // Branches covered (not supported)
            rootNode->append_attribute(doc.allocate_attribute(Keys[BranchesCoveredKey], "0"));

            // Branches valid (not supported)
            rootNode->append_attribute(doc.allocate_attribute(Keys[BranchesValidKey], "0"));

            // Timestamp of coverage report generation (aka time of serialization)
            rootNode->append_attribute(doc.allocate_attribute(Keys[TimestampKey], iso8601Timestamp.c_str()));

            // Lines covered (not supported)
            rootNode->append_attribute(doc.allocate_attribute(Keys[LinesCoveredKey], "0"));

            // Lines valid (not supported)
            rootNode->append_attribute(doc.allocate_attribute(Keys[LinesValidKey], "0"));

            // Version (not supported)
            rootNode->append_attribute(doc.allocate_attribute(Keys[VersionKey], "0"));

            // Root drive (this seems to be an unconventional use of the sources section by OpenCppCoverage)
            AZ::rapidxml::xml_node<>* sourcesNode = doc.allocate_node(AZ::rapidxml::node_element, Keys[SourcesKey]);
            AZ::rapidxml::xml_node<>* sourceNode = doc.allocate_node(
                AZ::rapidxml::node_element,
                Keys[SourceKey],
                doc.allocate_string(repoRoot.RootName().String().c_str()));
            sourcesNode->append_node(sourceNode);
            rootNode->append_node(sourcesNode);

            // All modules covered
            AZ::rapidxml::xml_node<>* packagesNode = doc.allocate_node(AZ::rapidxml::node_element, Keys[PackagesKey]);
            rootNode->append_node(packagesNode);

            // Individual modules covered
            for (const auto& moduleCovered : testCoverage.GetModuleCoverages())
            {
                AZ::rapidxml::xml_node<>* packageNode = doc.allocate_node(AZ::rapidxml::node_element, Keys[PackageKey]);

                // Module path
                packageNode->append_attribute(
                    doc.allocate_attribute(Keys[NameKey], doc.allocate_string(moduleCovered.m_path.c_str())));

                // Line rate (not supported yet at the test level, even though TestCoverage supports this metric)
                packageNode->append_attribute(doc.allocate_attribute(Keys[LineRateKey], "1"));

                // Branch rate (not supported)
                packageNode->append_attribute(doc.allocate_attribute(Keys[BranchRateKey], "0"));

                // Complexity rate (not supported)
                packageNode->append_attribute(doc.allocate_attribute(Keys[ComplexityKey], "0"));

                // Covered sources containing node
                AZ::rapidxml::xml_node<>* classesNode = doc.allocate_node(AZ::rapidxml::node_element, Keys[ClassesKey]);

                // Individual sources covered
                for (const auto& sourceCovered : moduleCovered.m_sources)
                {
                    AZ::rapidxml::xml_node<>* classNode = doc.allocate_node(AZ::rapidxml::node_element, Keys[ClassKey]);

                    // Source file (sans full path)
                    classNode->append_attribute(
                        doc.allocate_attribute(Keys[NameKey], doc.allocate_string(sourceCovered.m_path.Filename().String().c_str())));

                     // Source file full path (sans root)
                    classNode->append_attribute(doc.allocate_attribute(
                        Keys[FileNameKey], doc.allocate_string(sourceCovered.m_path.RelativePath().String().c_str())));

                    // Line rate (not supported yet at the test level, even though TestCoverage supports this metric)
                    classNode->append_attribute(doc.allocate_attribute(Keys[LineRateKey], "1"));

                    // Branch rate (not supported)
                    classNode->append_attribute(doc.allocate_attribute(Keys[BranchRateKey], "0"));

                    // Complexity rate (not supported)
                    classNode->append_attribute(doc.allocate_attribute(Keys[ComplexityKey], "0"));

                    // Methods covered (not supported yet)
                    AZ::rapidxml::xml_node<>* methodsNode = doc.allocate_node(AZ::rapidxml::node_element, Keys[MethodsKey]);
                    classNode->append_node(methodsNode);

                    // Lines covered (not supported yet)
                    AZ::rapidxml::xml_node<>* linesNode = doc.allocate_node(AZ::rapidxml::node_element, Keys[LinesKey]);
                    classNode->append_node(linesNode);

                    classesNode->append_node(classNode);
                }

                packageNode->append_node(classesNode);
                packagesNode->append_node(packageNode);
            }

            AZStd::string xmlString;
            AZ::rapidxml::print(std::back_inserter(xmlString), doc);
            doc.clear();

            return xmlString;
        }
    } // namespace Cobertura
} // namespace TestImpact
