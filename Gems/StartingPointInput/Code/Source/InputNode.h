/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/InputNode.generated.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace StartingPointInput
{
    //////////////////////////////////////////////////////////////////////////
    /// Input handles raw input from any source and outputs Pressed, Held, and Released input events
    /// This nodes id deprecated in favor of its nodeable counterpart.
    class InputNode
        : public ScriptCanvas::Node
    {

    public:

        SCRIPTCANVAS_NODE(InputNode);

        InputNode() = default;
        ~InputNode() override = default;
        InputNode(const InputNode&) = default;
        InputNode& operator=(const InputNode&) = default;

        AZStd::string m_eventName;
        float m_value;
    };
}
