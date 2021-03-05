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

#ifndef __EMFX_ATTACHMENTCOMMANDS_H
#define __EMFX_ATTACHMENTCOMMANDS_H

// include the required headers
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>


namespace CommandSystem
{
    // add attachment
    MCORE_DEFINECOMMAND_START(CommandAddAttachment, "Add attachment", true)
public:
    static bool AddAttachment(MCore::Command* command, const MCore::CommandLine& parameters, AZStd::string& outResult, bool remove);
    MCORE_DEFINECOMMAND_END

    // remove attachment
        MCORE_DEFINECOMMAND_START(CommandRemoveAttachment, "Remove attachment", true)
    MCORE_DEFINECOMMAND_END

    // clear attachments
        MCORE_DEFINECOMMAND(CommandClearAttachments, "ClearAttachments", "Clear attachments", true)

    // add multi-node attachment
    MCORE_DEFINECOMMAND_START(CommandAddDeformableAttachment, "Add skin attachment", true)
public:
    static bool AddAttachment(MCore::Command* command, const MCore::CommandLine& parameters, AZStd::string& outResult, bool remove);
    MCORE_DEFINECOMMAND_END
} // namespace CommandSystem


#endif
