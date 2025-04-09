// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${Name}.h"

/////////////////////////////////////////////////////////////
// This registration only needs to happen once per module
// You can keep it here, or move it into this module's 
// system component
#include <ScriptCanvas/AutoGen/ScriptCanvasAutoGenRegistry.h>
#include <Source/${Name}_Nodeables.generated.h>
REGISTER_SCRIPTCANVAS_AUTOGEN_NODEABLE(${Name}Object);
/////////////////////////////////////////////////////////////

namespace ScriptCanvas::Nodes
{
    void ${SanitizedCppName}::In()
    {

    }

} // namespace ScriptCanvas::Nodes
