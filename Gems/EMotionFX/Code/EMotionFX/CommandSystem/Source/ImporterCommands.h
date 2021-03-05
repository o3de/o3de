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
    uint32  mPreviouslyUsedID;
    uint32  mOldIndex;
    bool    mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END

    // add motion
        MCORE_DEFINECOMMAND_START(CommandImportMotion, "Import motion", true)
public:
    uint32          mOldMotionID;
    AZStd::string   mOldFileName;
    bool            mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END

} // namespace CommandSystem


#endif
