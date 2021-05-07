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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace O3DE::ProjectManager
{
    typedef struct Project
    {
        // from o3de_manifest.json and o3de_projects.json
        AZStd::string m_path;

        // from project.json
        AZStd::string m_projectName;
        AZStd::string m_productName;
        AZStd::string m_executableName;
        AZ::Uuid m_projectId;
    } Project;

    typedef struct GemDependency
    {
        AZ::Uuid m_uuid;
        AZStd::vector<AZStd::string> m_versionConstraints;
        AZStd::string m_comment; // typically contains the Gem dependency name
    } GemDependency;

    typedef struct Gem
    {
        // from o3de_manifest.json
        AZStd::string m_path;

        // from gem.json
        AZ::Uuid m_uuid;
        AZStd::vector<GemDependency> m_dependencies; 
        AZStd::string m_displayName;
        AZStd::string m_iconPath;
        bool m_isGameGem = false;
        bool m_isRequired = false;
        AZStd::string m_linkType;
        AZStd::string m_name;
        AZStd::string m_summary;
        AZStd::vector<AZStd::string> m_tags;
        AZStd::string m_version;
    } Gem;

    //! Interface used to interact with the o3de cli python functions
    class IPythonBindings
    {
    public:
        AZ_RTTI(O3DE::ProjectManager::IPythonBindings, "{C2B72CA4-56A9-4601-A584-3B40E83AA17C}");
        AZ_DISABLE_COPY_MOVE(IPythonBindings);

        IPythonBindings() = default;
        virtual ~IPythonBindings() = default;

        //! Get the current project 
        virtual Project GetCurrentProject() = 0;
    };

    using PythonBindingsInterface = AZ::Interface<IPythonBindings>;
} // namespace O3DE::ProjectManager
