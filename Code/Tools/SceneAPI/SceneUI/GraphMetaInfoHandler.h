#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
