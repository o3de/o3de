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

#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>

class CContentCGF;

namespace AZ
{
    namespace RC
    {
        struct CgfContainerExportContext;
        struct CgfGroupExportContext;

        void ConfigureCgfContent(CContentCGF& content);
        AZ::SceneAPI::Events::ProcessingResult ProcessMeshes(CgfGroupExportContext& context, CContentCGF& content, const AZStd::vector<AZStd::string>& targetNodes);
        void ProcessMeshType(ContainerExportContext& context, CContentCGF& content, const AZStd::vector<AZStd::string>& targetNodes, EPhysicsGeomType physicalizeType);
        void SetNodeName(const AZStd::string& name, CNodeCGF& node);
    } // namespace RC
} // namespace AZ
