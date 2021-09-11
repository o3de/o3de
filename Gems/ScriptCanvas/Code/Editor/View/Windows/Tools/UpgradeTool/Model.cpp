/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/View/Windows/Tools/UpgradeTool/Model.h>

namespace ModifierCpp
{

}

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        const AZStd::vector<AZStd::string>* Model::GetLogs()
        {
            return &m_log.GetEntries();
        }
    }
}
