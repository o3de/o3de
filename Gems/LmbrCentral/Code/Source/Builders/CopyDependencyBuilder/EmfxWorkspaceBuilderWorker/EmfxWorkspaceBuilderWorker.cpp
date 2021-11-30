/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EmfxWorkspaceBuilderWorker.h"
#include <AzCore/std/string/regex.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace CopyDependencyBuilder
{
    EmfxWorkspaceBuilderWorker::EmfxWorkspaceBuilderWorker()
        : CopyDependencyBuilderWorker("EmfxWorkspace", true, true)
    {
    }

    void EmfxWorkspaceBuilderWorker::RegisterBuilderWorker()
    {
        AssetBuilderSDK::AssetBuilderDesc emfxWorkspaceBuilderDescriptor;
        emfxWorkspaceBuilderDescriptor.m_name = "EmfxWorkspaceBuilderDescriptor";
        emfxWorkspaceBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.emfxworkspace", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        emfxWorkspaceBuilderDescriptor.m_busId = azrtti_typeid<EmfxWorkspaceBuilderWorker>();
        emfxWorkspaceBuilderDescriptor.m_version = 1;
        emfxWorkspaceBuilderDescriptor.m_createJobFunction =
            AZStd::bind(&EmfxWorkspaceBuilderWorker::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        emfxWorkspaceBuilderDescriptor.m_processJobFunction =
            AZStd::bind(&EmfxWorkspaceBuilderWorker::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(emfxWorkspaceBuilderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, emfxWorkspaceBuilderDescriptor);
    }

    void EmfxWorkspaceBuilderWorker::UnregisterBuilderWorker()
    {
        BusDisconnect();
    }

    bool EmfxWorkspaceBuilderWorker::ParseProductDependencies(
        const AssetBuilderSDK::ProcessJobRequest& request,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& /*productDependencies*/,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
    {

        AZ::IO::FileIOStream fileStream;
        if (!fileStream.Open(request.m_fullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            return false;
        }
        AZ::IO::SizeType length = fileStream.GetLength();
        if (length == 0)
        {
            return true;
        }

        AZStd::vector<char> charBuffer;
        charBuffer.resize_no_construct(length + 1);
        fileStream.Read(length, charBuffer.data());
        charBuffer.back() = 0;

        /* File Contents of EMFX Workspace file looks like
        startScript="ImportActor -filename \"@products@/animationsamples/advanced_rinlocomotion/actor/rinactor.actor\"\nCreateActorInstance
        -actorID %LASTRESULT% -xPos 0.000000 -yPos 0.020660 -zPos 0.000000 -xScale 1.000000 -yScale 1.000000 -zScale 1.000000 -rot 0.00000000,
        0.00000000,0.00000000,0.99997193\n LoadMotionSet -filename \"@products@/AnimationSamples/Advanced_RinLocomotion/AnimationEditorFiles/Advanced_RinLocomotion.motionset\"
        \nLoadAnimGraph -filename \"@products@/AnimationSamples/Advanced_RinLocomotion/AnimationEditorFiles/Advanced_RinLocomotion.animgraph\"
        \nActivateAnimGraph -actorInstanceID %LASTRESULT3% -animGraphID %LASTRESULT1% -motionSetID %LASTRESULT2% -visualizeScale 1.000000\n"
        */
        AZStd::regex pathRegex(R"(\s+-filename\s+\\\"(?:@products@\/)?(\S+)\\\")");
        AZStd::smatch matches;
        AZStd::string::const_iterator searchStart = charBuffer.begin();
        while (AZStd::regex_search(searchStart, matches, pathRegex))
        {
            pathDependencies.emplace(matches[1].str(), AssetBuilderSDK::ProductPathDependencyType::ProductFile);
            searchStart = matches[0].second;
        }

        return true;
    }
}
