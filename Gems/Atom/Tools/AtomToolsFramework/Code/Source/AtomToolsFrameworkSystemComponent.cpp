/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsAnyDocument.h>
#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystem.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNode.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeManager.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodePaletteItem.h>
#include <AtomToolsFramework/Graph/GraphCompiler.h>
#include <AtomToolsFramework/Graph/GraphDocument.h>
#include <AtomToolsFramework/Graph/GraphViewConstructPresets.h>
#include <AtomToolsFramework/Graph/GraphViewSettings.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFrameworkSystemComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/string/regex.h>
#include <Inspector/PropertyWidgets/PropertyStringBrowseEditCtrl.h>

namespace AtomToolsFramework
{
    void AtomToolsFrameworkSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AtomToolsDocument::Reflect(context);
        AtomToolsAnyDocument::Reflect(context);
        AtomToolsDocumentSystem::Reflect(context);
        CreateDynamicNodeMimeEvent::Reflect(context);
        DynamicNode::Reflect(context);
        DynamicProperty::Reflect(context);
        DynamicPropertyGroup::Reflect(context);
        EntityPreviewViewportSettingsSystem::Reflect(context);
        GraphCompiler::Reflect(context);
        GraphDocument::Reflect(context);
        GraphViewSettings::Reflect(context);
        GraphViewConstructPresets::Reflect(context);
        InspectorWidget::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->RegisterGenericType<AZStd::unordered_map<AZStd::string, bool>>();
            serialize->RegisterGenericType<AZStd::map<AZStd::string, AZStd::vector<AZStd::string>>>();

            serialize->Class<AtomToolsFrameworkSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (auto editContext = serialize->GetEditContext())
            {
                editContext->Class<AtomToolsFrameworkSystemComponent>("AtomToolsFrameworkSystemComponent", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomToolsFrameworkSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomToolsFrameworkSystemService"));
    }

    void AtomToolsFrameworkSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomToolsFrameworkSystemService"));
    }

    void AtomToolsFrameworkSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AtomToolsFrameworkSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AtomToolsFrameworkSystemComponent::Init()
    {
    }

    void AtomToolsFrameworkSystemComponent::Activate()
    {
        ReadSettings();
        RegisterStringBrowseEditHandler();
        AtomToolsFrameworkSystemRequestBus::Handler::BusConnect();

        // Monitor and update registry settings related to file utility functions
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            m_settingsNotifyEventHandler = registry->RegisterNotifier(
                [this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
                {
                    if (AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual(
                            "/O3DE/Atom/Tools", notifyEventArgs.m_jsonKeyPath) ||
                        AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual(
                            "/O3DE/AtomToolsFramework", notifyEventArgs.m_jsonKeyPath))
                    {
                        ReadSettings();
                    }
                });
        }
    }

    void AtomToolsFrameworkSystemComponent::Deactivate()
    {
        AtomToolsFrameworkSystemRequestBus::Handler::BusDisconnect();
    }

    void AtomToolsFrameworkSystemComponent::ReadSettings()
    {
        AZ::IO::FixedMaxPath cachePath = AZ::Utils::GetProjectPath();
        cachePath /= "Cache";

        m_cacheFolder = cachePath.LexicallyNormal().StringAsPosix();
        m_ignoreCacheFolder = GetSettingsValue("/O3DE/AtomToolsFramework/Application/IgnoreCacheFolder", true);
        m_ignoredPathRegexPatterns = GetSettingsObject<AZStd::vector<AZStd::string>>("/O3DE/AtomToolsFramework/Application/IgnoredPathRegexPatterns");
        m_editablePathSettings = GetSettingsObject<AZStd::unordered_map<AZStd::string, bool>>("/O3DE/Atom/Tools/EditablePathSettings");
        m_previewablePathSettings = GetSettingsObject<AZStd::unordered_map<AZStd::string, bool>>("/O3DE/Atom/Tools/PreviewablePathSettings");
    }

    bool AtomToolsFrameworkSystemComponent::IsPathIgnored(const AZStd::string& path) const
    {
        if (path.empty())
        {
            return true;
        }

        // Ignoring the cache folder is currently the most common case for tools that want to ignore intermediate assets
        const AZStd::string pathWithoutAlias = GetPathWithoutAlias(path);
        if (m_ignoreCacheFolder && pathWithoutAlias.starts_with(m_cacheFolder))
        {
            return true;
        }

        // For more extensive customization, pattern matching is also supported via IgnoredPathRegexPatterns. This is empty by default.
        for (const auto& patternStr : m_ignoredPathRegexPatterns)
        {
            if (!patternStr.empty())
            {
                AZStd::regex patternRegex(patternStr, AZStd::regex::flag_type::icase);
                if (AZStd::regex_match(pathWithoutAlias, patternRegex))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool AtomToolsFrameworkSystemComponent::IsPathEditable(const AZStd::string& path) const
    {
        if (!path.empty())
        {
            const AZStd::string pathWithoutAlias = GetPathWithoutAlias(path);
            for (const auto& [storedPath, flag] : m_editablePathSettings)
            {
                if (pathWithoutAlias == GetPathWithoutAlias(storedPath))
                {
                    return flag;
                }
            }
        }

        return true;
    }

    bool AtomToolsFrameworkSystemComponent::IsPathPreviewable(const AZStd::string& path) const
    {
        if (!path.empty())
        {
            const AZStd::string pathWithoutAlias = GetPathWithoutAlias(path);
            for (const auto& [storedPath, flag] : m_previewablePathSettings)
            {
                if (pathWithoutAlias == GetPathWithoutAlias(storedPath))
                {
                    return flag;
                }
            }
        }

        return true;
    }
} // namespace AtomToolsFramework
