/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AWSCore
{
    static constexpr int IconSize = 16;

    static constexpr const int NameIndex = 0;
    static constexpr const int IdentIndex = 1;
    static constexpr const int IconIndex = 2;
    static constexpr const int URLIndex = 3;

    static constexpr const char* NewToAWS[] =
    {
        "Getting started with AWS?",
        "o3de.menu.editor.aws.newtoaws",
        ":/Notifications/link.svg",
        "https://o3de.org/docs/user-guide/gems/reference/aws/"
    };

    static constexpr const char* AWSAndO3DEGlobalDocs[] =
    {
        "AWS && O3DE",
        "o3de.menu.editor.aws.aws_and_o3de",
        "",
        ""
    };

    static constexpr const char* AWSAndO3DEGettingStarted[] =
    {
        "Getting started",
        "o3de.menu.editor.aws.getting_started",
        ":/Notifications/link.svg",
        "https://o3de.org/docs/user-guide/gems/reference/aws/aws-core/getting-started/"
    };

    static constexpr const char* AWSCredentialConfiguration[] =
    {
        "AWS credential configuration",
        "o3de.menu.editor.aws.aws_credential_configuration",
        ":/Notifications/link.svg",
        "https://o3de.org/docs/user-guide/gems/reference/aws/aws-core/configuring-credentials/"
    };

    static constexpr const char* AWSAndO3DEMappingsFile[] =
    {
        "Use AWS resources in O3DE",
        "o3de.menu.editor.aws.use_aws_resources_in_o3de",
        ":/Notifications/link.svg",
        "https://o3de.org/docs/user-guide/gems/reference/aws/aws-core/resource-mapping-files/"
    };

    static constexpr const char* AWSAndO3DEMappingsTool[] =
    {
        "Building a resource mappings file",
        "o3de.menu.editor.aws.building_a_resource_mappings_file",
        ":/Notifications/link.svg",
        "https://o3de.org/docs/user-guide/gems/reference/aws/aws-core/resource-mapping-tool/"
    };

    static constexpr const char* AWSAndO3DEScripting[] =
    {
        "Scripting reference",
        "o3de.menu.editor.aws.scripting_reference",
        ":/Notifications/link.svg",
        "https://o3de.org/docs/user-guide/gems/reference/aws/aws-core/scripting/"
    };

    static constexpr const char* O3DEAndAWS[] =
    {
        "O3DE && AWS",
        "o3de.menu.editor.aws.o3de_and_aws",
        "",
        ""
    };


    static constexpr const char AWSResourceMappingToolActionText[] = "AWS Resource Mapping Tool...";

    static constexpr const char* AWSClientAuth[] =
    {
         "Client Auth Gem" ,
         "client_auth_gem" ,
         ":/Notifications/download.svg",
         ""
    };

    static constexpr const char* AWSClientAuthGemOverview[] =
    {
         "Client Auth Gem overview" ,
         "client_auth_gem_overview" ,
         "",
         "https://o3de.org/docs/user-guide/gems/reference/aws/aws-client-auth/"
    };

    static constexpr const char AWSClientAuthActionText[] = "Client Auth Gem";
    static constexpr const char AWSClientAuthGemOverviewActionText[] = "Client Auth Gem overview";
    static constexpr const char AWSClientAuthGemSetupActionText[] = "Setup Client Auth Gem";
    static constexpr const char AWSClientAuthCDKAndResourcesActionText[] = "CDK application and resource mappings";
    static constexpr const char AWSClientAuthScriptCanvasAndLuaActionText[] = "Scripting reference";
    static constexpr const char AWSClientAuth3rdPartyAuthProviderActionText[] = "3rd Party developer authentication provider support";
    static constexpr const char AWSClientAuthCustomAuthProviderActionText[] = "Custom developer authentication provider support";
    static constexpr const char AWSClientAuthAPIReferenceActionText[] = "API reference";




    static constexpr const char* AWSGameLift[] =
    {
         "GameLift Gem" ,
         "gamelift_gem" ,
         ":/Notifications/checkmark.svg",
         ""
    };

    static constexpr const char* AWSGameLiftOverview[] =
    {
         "Gem overview" ,
         "gamelift_gem_overview" ,
         ":/Notifications/link.svg",
         ""
    };
    static constexpr const char AWSGameLiftActionText[] = "GameLift Gem";
    static constexpr const char AWSGameLiftGemOverviewActionText[] = "Gem overview";
    static constexpr const char AWSGameLiftGemSetupActionText[] = "Setup";
    static constexpr const char AWSMGameLiftScriptingActionText[] = "Scripting reference";
    static constexpr const char AWSGameLiftAPIReferenceActionText[] = "API reference";
    static constexpr const char AWSGameLiftAdvancedTopicsActionText[] = "Advanced topics";
    static constexpr const char AWSGameLiftLocalTestingActionText[] = "Local testing";
    static constexpr const char AWSGameLiftBuildPackagingActionText[] = "Build packaging (Windows)";
    static constexpr const char AWSGameLiftResourceManagementActionText[] = "Resource Management";
} // namespace AWSCore
