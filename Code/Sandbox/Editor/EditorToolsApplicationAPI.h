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

namespace EditorInternal
{
    /**
     * Bus used to make general requests to the ToolsApplication.
     */
    class EditorToolsApplicationRequests
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<EditorToolsApplicationRequests>;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual bool OpenLevel(AZStd::string_view levelName) = 0;
        virtual bool OpenLevelNoPrompt(AZStd::string_view levelName) = 0;

        virtual int CreateLevel(AZStd::string_view levelName, bool bUseTerrain) = 0;
        virtual int CreateLevelNoPrompt(AZStd::string_view levelName, int terrainExportTextureSize, bool useTerrain) = 0;

        virtual AZStd::string GetGameFolder() const = 0;
        virtual AZStd::string GetCurrentLevelName() const = 0;
        virtual AZStd::string GetCurrentLevelPath() const = 0;

        virtual void Exit() = 0;
        virtual void ExitNoPrompt() = 0;
    };

    using EditorToolsApplicationRequestBus = AZ::EBus<EditorToolsApplicationRequests>;
    
} // namespace EditorInternal

