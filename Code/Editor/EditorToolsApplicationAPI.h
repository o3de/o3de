/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        virtual int CreateLevel(AZStd::string_view templateName, AZStd::string_view levelName, bool bUseTerrain) = 0;
        virtual int CreateLevelNoPrompt(AZStd::string_view templateName, AZStd::string_view levelName, int terrainExportTextureSize, bool useTerrain) = 0;

        virtual AZStd::string GetGameFolder() const = 0;
        virtual AZStd::string GetCurrentLevelName() const = 0;
        virtual AZStd::string GetCurrentLevelPath() const = 0;

        //! Retrieve old cry level file extension (With prepending '.')
        virtual const char* GetOldCryLevelExtension() const = 0;
        //! Retrieve default level file extension (With prepending '.')
        virtual const char* GetLevelExtension() const = 0;

        virtual void Exit() = 0;
        virtual void ExitNoPrompt() = 0;
    };

    using EditorToolsApplicationRequestBus = AZ::EBus<EditorToolsApplicationRequests>;
    
} // namespace EditorInternal

