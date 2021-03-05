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


// Forward declare namespace for SceneAPI to keep code tidy in emotion fx pipeline.
namespace AZ
{
    class ReflectContext;
    namespace SceneAPI
    {
        namespace Containers
        {
        }

        namespace DataTypes
        {
        }

        namespace SceneData
        {
        }

        namespace SceneCore
        {
        }

        namespace Events
        {
            class ExportEventContext;
        }

        namespace Utilities
        {
        }

        namespace UI
        {
        }
    }
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneData = AZ::SceneAPI::SceneData;
        namespace SceneCore = AZ::SceneAPI::SceneCore;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtilities = AZ::SceneAPI::Utilities;
        namespace SceneUI = AZ::SceneAPI::UI;

        // EMotionFX supports 6 total LODs.  1 for the base model then 5 more lods.  
        // The rule captures all lods except the first one so this is set to 5. 
        static const unsigned int g_maxLods = 5;
    }
}
