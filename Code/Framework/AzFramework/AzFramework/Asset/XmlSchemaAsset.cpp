/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "XmlSchemaAsset.h"

namespace AzFramework
{
    void VersionSearchRule::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VersionSearchRule>()
                ->Version(1)
                ->Field("RootNodeAttributeName", &VersionSearchRule::m_rootNodeAttributeName);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<VersionSearchRule>("Version Search Rule", "Rule for getting the attribute of the root node which specifies the version")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VersionSearchRule::m_rootNodeAttributeName, "Root Node Attribute Name", "Attribute name of the root node which specifies the version. Example: versionnumber");
            }
        }
    }

    AZStd::string VersionSearchRule::GetRootNodeAttributeName() const
    {
        return m_rootNodeAttributeName;
    }

    void MatchingRule::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MatchingRule>()
                ->Version(1)
                ->Field("FilePathPattern", &MatchingRule::m_filePathPattern)
                ->Field("ExcludedFilePathPattern", &MatchingRule::m_excludedFilePathPattern)
                ->Field("VersionConstraints", &MatchingRule::m_versionConstraints);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MatchingRule>("Matching Rules", "Rules for matchup")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MatchingRule::m_filePathPattern, "File Path Pattern", "Pattern of the file path. Example: *Fonts/*.xml")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MatchingRule::m_excludedFilePathPattern, "Excluded File Path Pattern", "Pattern of the excluded file path. Example: *Fonts/*.xml")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MatchingRule::m_versionConstraints, "Version Constraints", "Data file versions these rules adapt to. These constraints follow the rules of Semantic Versioning. Example: >=1.2.3, ~>1.2.3");
            }
        }
    }

    bool MatchingRule::Valid() const
    {
        return !m_filePathPattern.empty();
    }

    AZStd::string MatchingRule::GetFilePathPattern() const
    {
        return m_filePathPattern;
    }

    AZStd::string MatchingRule::GetExcludedFilePathPattern() const
    {
        return m_excludedFilePathPattern;
    }

    AZStd::vector<AZStd::string> MatchingRule::GetVersionConstraints() const
    {
        return m_versionConstraints;
    }

    void XmlSchemaAttribute::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<XmlSchemaAttribute>()
                ->Version(6)
                ->Field("Name", &XmlSchemaAttribute::m_name)
                ->Field("ExpectedExtension", &XmlSchemaAttribute::m_expectedExtension)
                ->Field("MatchPattern", &XmlSchemaAttribute::m_matchPattern)
                ->Field("FindPattern", &XmlSchemaAttribute::m_findPattern)
                ->Field("ReplacePattern", &XmlSchemaAttribute::m_replacePattern)
                ->Field("Type", &XmlSchemaAttribute::m_type)
                ->Field("PathDependencyType", &XmlSchemaAttribute::m_pathDependencyType)
                ->Field("RelativeToSourceAssetFolder", &XmlSchemaAttribute::m_relativeToSourceAssetFolder)
                ->Field("Optional", &XmlSchemaAttribute::m_optional)
                ->Field("CacheRelativePath", &XmlSchemaAttribute::m_cacheRelativePath);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<XmlSchemaAttribute>("XmlSchemaAttribute", "XML Schema attribute")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_name, "Name", "Name of the attribute")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_expectedExtension, "Expected Extension", "Expected extension for the file name.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_matchPattern, "Match Pattern", "(Optional) Values that don't match this regex pattern will be rejected.  Case-insensitive.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_findPattern, "Find Pattern", "(Optional) Regex pattern to use to match against the value for replacing.  Case-insensitive.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_replacePattern, "Replace Pattern", "(Optional) Regex pattern to use to replace the value.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &XmlSchemaAttribute::m_type, "Type", "Type of the attribute. Select from RelativePath, AssetId, etc.")
                    ->EnumAttribute(AttributeType::RelativePath, "RelativePath")
                    ->EnumAttribute(AttributeType::Asset, "Asset")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &XmlSchemaAttribute::m_pathDependencyType, "Path Dependency Type", "Path dependency type of the attribute. Select from SourceFile and ProductFile")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &XmlSchemaAttribute::GetVisibilityProperty)
                    ->EnumAttribute(AttributePathDependencyType::SourceFile, "SourceFile")
                    ->EnumAttribute(AttributePathDependencyType::ProductFile, "ProductFile")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_relativeToSourceAssetFolder, "RelativeToSourceAssetFolder", "Whether the file path is relative to the source asset folder")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_optional, "Optional", "Whether the attribute is optional")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_cacheRelativePath, "CacheRelativePath", "CacheRelative allows dependent assets to be from other scan folders.");

            }
        }
    }

    AZStd::string XmlSchemaAttribute::GetName() const
    {
        return m_name;
    }

    AZStd::string XmlSchemaAttribute::GetExpectedExtension() const
    {
        return m_expectedExtension;
    }

    AZStd::string XmlSchemaAttribute::GetMatchPattern() const
    {
        return m_matchPattern;
    }

    AZStd::string XmlSchemaAttribute::GetFindPattern() const
    {
        return m_findPattern;
    }

    AZStd::string XmlSchemaAttribute::GetReplacePattern() const
    {
        return m_replacePattern;
    }

    XmlSchemaAttribute::AttributeType XmlSchemaAttribute::GetType() const
    {
        return m_type;
    }

    XmlSchemaAttribute::AttributePathDependencyType XmlSchemaAttribute::GetPathDependencyType() const
    {
        return m_pathDependencyType;
    }

    bool XmlSchemaAttribute::IsRelativeToSourceAssetFolder() const
    {
        return m_relativeToSourceAssetFolder;
    }

    bool XmlSchemaAttribute::CacheRelativePath() const
    {
        return m_cacheRelativePath;
    }

    bool XmlSchemaAttribute::IsOptional() const
    {
        return m_optional;
    }

    AZ::Crc32 XmlSchemaAttribute::GetVisibilityProperty() const
    {
        return m_type == AttributeType::RelativePath ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void XmlSchemaElement::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<XmlSchemaElement>()
                ->Version(2)
                ->Field("Name", &XmlSchemaElement::m_name)
                ->Field("ChildElements", &XmlSchemaElement::m_childElements)
                ->Field("Attributes", &XmlSchemaElement::m_attributes)
                ->Field("Optional", &XmlSchemaElement::m_optional);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<XmlSchemaElement>("XmlSchemaElement", "XML Schema Element")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaElement::m_name, "Name", "Name of the element")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaElement::m_childElements, "Child Elements", "Children of the element")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaElement::m_attributes, "Attributes", "Attributes of the element")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaElement::m_optional, "Optional", "Whether the element is optional");
            }
        }
    }

    AZStd::string XmlSchemaElement::GetName() const
    {
        return m_name;
    }
    AZStd::vector<XmlSchemaElement> XmlSchemaElement::GetChildElements() const
    {
        return m_childElements;
    }

    AZStd::vector<XmlSchemaAttribute> XmlSchemaElement::GetAttributes() const
    {
        return m_attributes;
    }

    bool XmlSchemaElement::IsOptional() const
    {
        return m_optional;
    }

    bool XmlSchemaElement::Valid() const
    {
        if (m_name.empty())
        {
            return false;
        }

        for (const XmlSchemaElement& childElement : m_childElements)
        {
            if (!childElement.Valid())
            {
                return false;
            }
        }

        return true;
    }

    void SearchRuleDefinition::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SearchRuleDefinition>()
                ->Version(1)
                ->Field("SearchRuleStructure", &SearchRuleDefinition::m_searchRuleStructure)
                ->Field("RelativeToXmlRoot", &SearchRuleDefinition::m_relativeToXmlRoot);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SearchRuleDefinition>("SearchRuleDefinition", "Definition for the dependency search rule")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchRuleDefinition::m_searchRuleStructure, "Search Rule Structure", "Search rule structure which contain element and attribute nodes")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchRuleDefinition::m_relativeToXmlRoot, "Relative to XML Root", "Whether the element is relative to XML root");
            }
        }
    }

    XmlSchemaElement SearchRuleDefinition::GetSearchRuleStructure() const
    {
        return m_searchRuleStructure;
    }

    bool SearchRuleDefinition::IsRelativeToXmlRoot() const
    {
        return m_relativeToXmlRoot;
    }

    void DependencySearchRule::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DependencySearchRule>()
                ->Version(2)
                ->Field("SearchRuleDefinitions", &DependencySearchRule::m_searchRuleDefinitions)
                ->Field("VersionConstraints", &DependencySearchRule::m_versionConstraints);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DependencySearchRule>("DependencySearchRule", "Dependency search rules")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DependencySearchRule::m_searchRuleDefinitions, "Search Rule Definitions", "A list of Definitions for dependency search rules")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DependencySearchRule::m_versionConstraints, "Version Constraints", "Data file versions these rules adapt to. These constraints follow the rules of Semantic Versioning. Example: >=1.2.3: Minimum: 1.2.3 Maximum: None");
            }
        }
    }

    AZStd::vector<SearchRuleDefinition> DependencySearchRule::GetSearchRules() const
    {
        return m_searchRuleDefinitions;
    }

    AZStd::vector<AZStd::string> DependencySearchRule::GetVersionConstraints() const
    {
        return m_versionConstraints;
    }

    bool DependencySearchRule::Valid() const
    {
        for (const SearchRuleDefinition& searchRuleDefinition : m_searchRuleDefinitions)
        {
            if (!searchRuleDefinition.GetSearchRuleStructure().Valid())
            {
                return false;
            }
        }

        return true;
    }

    void XmlSchemaAsset::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<XmlSchemaAsset>()
                ->Version(3)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("VersionSearchRule", &XmlSchemaAsset::m_versionSearchRule)
                ->Field("MatchingRules", &XmlSchemaAsset::m_matchingRules)
                ->Field("DependencySearchRules", &XmlSchemaAsset::m_dependencySearchRules)
                ->Field("useAZSerialization", &XmlSchemaAsset::m_useAZSerialization);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<XmlSchemaAsset>("Definition", "Definition of the schema asset")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAsset::m_versionSearchRule, "Version Search Rule", "VersionSearchRule")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAsset::m_matchingRules, "Matching Rules", "A list of matching rules defined by the current schema")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAsset::m_useAZSerialization, "Use AZ Serialization for dependencies", "Use AZ serialization to extract dependencies from matching files")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAsset::m_dependencySearchRules, "Dependency Search Rules", "A list of dependency search rules defined by the current schema");
            }
        }
    }

    VersionSearchRule XmlSchemaAsset::GetVersionSearchRule() const
    {
        return m_versionSearchRule;
    }

    AZStd::vector<MatchingRule> XmlSchemaAsset::GetMatchingRules() const
    {
        return m_matchingRules;
    }

    AZStd::vector<DependencySearchRule> XmlSchemaAsset::GetDependencySearchRules() const
    {
        return m_dependencySearchRules;
    }
}
