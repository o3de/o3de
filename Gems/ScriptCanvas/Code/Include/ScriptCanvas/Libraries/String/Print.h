/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>

#include <ScriptCanvas/Internal/Nodes/StringFormatted.h>

#include <Include/ScriptCanvas/Libraries/String/Print.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            //! Prints a formatted string into the console.
            class Print
                : public Internal::StringFormatted
            {
            protected:

                SCRIPTCANVAS_NODE(Print);
                
                void OnInputSignal(const SlotId&) override;

            };
        }
    }
}

