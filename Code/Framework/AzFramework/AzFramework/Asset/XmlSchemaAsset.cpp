/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "XmlSchemaAsset.h"
#include <AzFramework/Translation/TranslationDef.h>

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
                editContext->Class<VersionSearchRule>(
                    QT_TRANSLATE_NOOP("AzFramework", "Version Search Rule"),
                    QT_TRANSLATE_NOOP("AzFramework", "Rule for getting the attribute of the root node which specifies the version"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VersionSearchRule::m_rootNodeAttributeName,
                        QT_TRANSLATE_NOOP("AzFramework", "Root Node Attribute Name"),
                        QT_TRANSLATE_NOOP("AzFramework", "Attribute name of the root node which specifies the version. Example: versionnumber"));
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
                editContext->Class<MatchingRule>(
                    QT_TRANSLATE_NOOP("AzFramework", "Matching Rules"),
                    QT_TRANSLATE_NOOP("AzFramework", "Rules for matchup"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MatchingRule::m_filePathPattern,
                        QT_TRANSLATE_NOOP("AzFramework", "File Path Pattern"),
                        QT_TRANSLATE_NOOP("AzFramework", "Pattern of the file path. Example: *Fonts/*.xml"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MatchingRule::m_excludedFilePathPattern,
                        QT_TRANSLATE_NOOP("AzFramework", "Excluded File Path Pattern"),
                        QT_TRANSLATE_NOOP("AzFramework", "Pattern of the excluded file path. Example: *Fonts/*.xml"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MatchingRule::m_versionConstraints,
                        QT_TRANSLATE_NOOP("AzFramework", "Version Constraints"),
                        QT_TRANSLATE_NOOP("AzFramework", "Data file versions these rules adapt to. These constraints follow the rules of Semantic Versioning. Example: >=1.2.3, ~>1.2.3"));
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
                editContext->Class<XmlSchemaAttribute>(
                    QT_TRANSLATE_NOOP("AzFramework", "XmlSchemaAttribute"),
                    QT_TRANSLATE_NOOP("AzFramework", "XML Schema attribute"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_name,
                        QT_TRANSLATE_NOOP("AzFramework", "Name"),
                        QT_TRANSLATE_NOOP("AzFramework", "Name of the attribute"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_expectedExtension,
                        QT_TRANSLATE_NOOP("AzFramework", "Expected Extension"),
                        QT_TRANSLATE_NOOP("AzFramework", "Expected extension for the file name."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_matchPattern,
                        QT_TRANSLATE_NOOP("AzFramework", "Match Pattern"),
                        QT_TRANSLATE_NOOP("AzFramework", "(Optional) Values that don't match this regex pattern will be rejected.  Case-insensitive."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_findPattern,
                        QT_TRANSLATE_NOOP("AzFramework", "Find Pattern"),
                        QT_TRANSLATE_NOOP("AzFramework", "(Optional) Regex pattern to use to match against the value for replacing.  Case-insensitive."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_replacePattern,
                        QT_TRANSLATE_NOOP("AzFramework", "Replace Pattern"),
                        QT_TRANSLATE_NOOP("AzFramework", "(Optional) Regex pattern to use to replace the value."))
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &XmlSchemaAttribute::m_type,
                        QT_TRANSLATE_NOOP("AzFramework", "Type"),
                        QT_TRANSLATE_NOOP("AzFramework", "Type of the attribute. Select from RelativePath, AssetId, etc."))
                    ->EnumAttribute(AttributeType::RelativePath, "RelativePath")
                    ->EnumAttribute(AttributeType::Asset, "Asset")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &XmlSchemaAttribute::m_pathDependencyType,
                        QT_TRANSLATE_NOOP("AzFramework", "Path Dependency Type"),
                        QT_TRANSLATE_NOOP("AzFramework", "Path dependency type of the attribute. Select from SourceFile and ProductFile"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &XmlSchemaAttribute::GetVisibilityProperty)
                    ->EnumAttribute(AttributePathDependencyType::SourceFile, "SourceFile")
                    ->EnumAttribute(AttributePathDependencyType::ProductFile, "ProductFile")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_relativeToSourceAssetFolder,
                        QT_TRANSLATE_NOOP("AzFramework", "RelativeToSourceAssetFolder"),
                        QT_TRANSLATE_NOOP("AzFramework", "Whether the file path is relative to the source asset folder"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_optional,
                        QT_TRANSLATE_NOOP("AzFramework", "Optional"),
                        QT_TRANSLATE_NOOP("AzFramework", "Whether the attribute is optional"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAttribute::m_cacheRelativePath,
                        QT_TRANSLATE_NOOP("AzFramework", "CacheRelativePath"),
                        QT_TRANSLATE_NOOP("AzFramework", "CacheRelative allows dependent assets to be from other scan folders."));

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
                editContext->Class<XmlSchemaElement>(
                    QT_TRANSLATE_NOOP("AzFramework", "XmlSchemaElement"),
                    QT_TRANSLATE_NOOP("AzFramework", "XML Schema Element"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaElement::m_name,
                        QT_TRANSLATE_NOOP("AzFramework", "Name"),
                        QT_TRANSLATE_NOOP("AzFramework", "Name of the element"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaElement::m_childElements,
                        QT_TRANSLATE_NOOP("AzFramework", "Child Elements"),
                        QT_TRANSLATE_NOOP("AzFramework", "Children of the element"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaElement::m_attributes,
                        QT_TRANSLATE_NOOP("AzFramework", "Attributes"),
                        QT_TRANSLATE_NOOP("AzFramework", "Attributes of the element"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaElement::m_optional,
                        QT_TRANSLATE_NOOP("AzFramework", "Optional"),
                        QT_TRANSLATE_NOOP("AzFramework", "Whether the element is optional"));
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
                editContext->Class<SearchRuleDefinition>(
                    QT_TRANSLATE_NOOP("AzFramework", "SearchRuleDefinition"),
                    QT_TRANSLATE_NOOP("AzFramework", "Definition for the dependency search rule"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchRuleDefinition::m_searchRuleStructure,
                        QT_TRANSLATE_NOOP("AzFramework", "Search Rule Structure"),
                        QT_TRANSLATE_NOOP("AzFramework", "Search rule structure which contain element and attribute nodes"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchRuleDefinition::m_relativeToXmlRoot,
                        QT_TRANSLATE_NOOP("AzFramework", "Relative to XML Root"),
                        QT_TRANSLATE_NOOP("AzFramework", "Whether the element is relative to XML root"));
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
                editContext->Class<DependencySearchRule>(
                    QT_TRANSLATE_NOOP("AzFramework", "DependencySearchRule"),
                    QT_TRANSLATE_NOOP("AzFramework", "Dependency search rules"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DependencySearchRule::m_searchRuleDefinitions,
                        QT_TRANSLATE_NOOP("AzFramework", "Search Rule Definitions"),
                        QT_TRANSLATE_NOOP("AzFramework", "A list of Definitions for dependency search rules"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DependencySearchRule::m_versionConstraints,
                        QT_TRANSLATE_NOOP("AzFramework", "Version Constraints"),
                        QT_TRANSLATE_NOOP("AzFramework", "Data file versions these rules adapt to. These constraints follow the rules of Semantic Versioning. Example: >=1.2.3: Minimum: 1.2.3 Maximum: None"));
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
                editContext->Class<XmlSchemaAsset>(
                    QT_TRANSLATE_NOOP("AzFramework", "Definition"),
                    QT_TRANSLATE_NOOP("AzFramework", "Definition of the schema asset"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAsset::m_versionSearchRule,
                        QT_TRANSLATE_NOOP("AzFramework", "Version Search Rule"),
                        QT_TRANSLATE_NOOP("AzFramework", "VersionSearchRule"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAsset::m_matchingRules,
                        QT_TRANSLATE_NOOP("AzFramework", "Matching Rules"),
                        QT_TRANSLATE_NOOP("AzFramework", "A list of matching rules defined by the current schema"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAsset::m_useAZSerialization,
                        QT_TRANSLATE_NOOP("AzFramework", "Use AZ Serialization for dependencies"),
                        QT_TRANSLATE_NOOP("AzFramework", "Use AZ serialization to extract dependencies from matching files"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XmlSchemaAsset::m_dependencySearchRules,
                        QT_TRANSLATE_NOOP("AzFramework", "Dependency Search Rules"),
                        QT_TRANSLATE_NOOP("AzFramework", "A list of dependency search rules defined by the current schema"));
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
