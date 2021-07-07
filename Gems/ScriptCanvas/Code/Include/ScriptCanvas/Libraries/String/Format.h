/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Internal/Nodes/StringFormatted.h>

#include <Include/ScriptCanvas/Libraries/String/Format.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            //! A String formatting class that produces a data output based on the specified string format and
            //! input values.
            class Format
                : public Internal::StringFormatted
            {
            protected:

                SCRIPTCANVAS_NODE(Format);

                void OnInputSignal(const SlotId& slotId) override;

            };
        }
    }
}
