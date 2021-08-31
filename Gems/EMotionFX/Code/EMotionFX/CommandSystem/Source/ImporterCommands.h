/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMFX_IMPORTERCOMMANDS_H
#define __EMFX_IMPORTERCOMMANDS_H

// include the required headers
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include "CommandManager.h"


namespace CommandSystem
{
    // add actor
    MCORE_DEFINECOMMAND_START(CommandImportActor, "Import actor", true)
public:
    uint32  m_previouslyUsedId;
    uint32  m_oldIndex;
    bool    m_oldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END

    // add motion
        MCORE_DEFINECOMMAND_START(CommandImportMotion, "Import motion", true)
public:
    uint32          m_oldMotionId;
    AZStd::string   m_oldFileName;
    bool            m_oldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END

} // namespace CommandSystem


#endif
