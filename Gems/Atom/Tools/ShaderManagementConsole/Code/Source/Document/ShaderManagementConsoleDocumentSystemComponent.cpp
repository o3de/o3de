/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Document/ShaderManagementConsoleDocument.h>
#include <Document/ShaderManagementConsoleDocumentSystemComponent.h>

namespace ShaderManagementConsole
{
    void ShaderManagementConsoleDocumentSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ShaderManagementConsoleDocumentSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ShaderManagementConsoleDocumentSystemComponent>("ShaderManagementConsoleDocumentSystemComponent", "Manages documents")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ShaderManagementConsoleDocumentRequestBus>("ShaderManagementConsoleDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "shadermanagementconsole")
                ->Event("GetShaderOptionCount", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionCount)
                ->Event("GetShaderOptionDescriptor", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor)
                ->Event("GetShaderVariantCount", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantCount)
                ->Event("GetShaderVariantInfo", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantInfo)
                ;
        }
    }

    void ShaderManagementConsoleDocumentSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AtomToolsDocumentSystemService"));
        required.push_back(AZ_CRC_CE("AssetProcessorToolsConnection"));
        required.push_back(AZ_CRC_CE("AssetDatabaseService"));
        required.push_back(AZ_CRC_CE("PropertyManagerService"));
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void ShaderManagementConsoleDocumentSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ShaderManagementConsoleDocumentSystemService"));
    }

    void ShaderManagementConsoleDocumentSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ShaderManagementConsoleDocumentSystemService"));
    }

    void ShaderManagementConsoleDocumentSystemComponent::Init()
    {
    }

    void ShaderManagementConsoleDocumentSystemComponent::Activate()
    {
        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
            &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType,
            []()
            {
                return aznew ShaderManagementConsoleDocument();
            });
    }

    void ShaderManagementConsoleDocumentSystemComponent::Deactivate()
    {
    }
}
