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
        AZStd::string Path;

        // from project.json
        AZStd::string ProjectName;
        AZStd::string ProductName;
        AZStd::string ExecutableName;
        AZ::Uuid ProjectId;
    } Project_t;

    typedef struct GemDependency
    {
        AZ::Uuid Uuid;
        AZStd::vector<AZStd::string> VersionConstraints;
        AZStd::string Comment; // typically contains the Gem dependency name
    } GemDependency_t;

    typedef struct Gem
    {
        // from o3de_manifest.json
        AZStd::string Path;

        // from gem.json
        AZ::Uuid Uuid;
        AZStd::vector<GemDependency> Dependencies; 
        AZStd::string DisplayName;
        AZStd::string IconPath;
        bool IsGameGem = false;
        bool IsRequired = false;
        AZStd::string LinkType;
        AZStd::string Name;
        AZStd::string Summary;
        AZStd::vector<AZStd::string> Tags;
        AZStd::string Version;
    } Gem_t;

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
