/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
