#pragma once

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

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            class GraphMetaInfoHandler : public Events::GraphMetaInfoBus::Handler
            {
            public:
                AZ_CLASS_ALLOCATOR_DECL

                GraphMetaInfoHandler();
                ~GraphMetaInfoHandler() override;

                void GetIconPath(AZStd::string& iconPath, const DataTypes::IGraphObject* target) override;
                void GetToolTip(AZStd::string& toolTip, const DataTypes::IGraphObject* target) override;
            };
        } // SceneData
    } // SceneAPI
} // AZ