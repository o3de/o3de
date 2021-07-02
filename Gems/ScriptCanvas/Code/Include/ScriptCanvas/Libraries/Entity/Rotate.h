/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>

#include <AzCore/Math/Vector3.h>

#include <Include/ScriptCanvas/Libraries/Entity/Rotate.generated.h>

// LY-103055 Temporary workaround to fix compile error when building Android NDK 19 and above due to AzCodeGenerator
// not receiving the correct clang system includes
namespace AZ
{
    class EntityId;
    class Vector3;
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Entity
        {
            //! Deprecated: see Entity Transform class' reflection of method "Rotate"
            class Rotate 
                : public Node
            {

            public:

                SCRIPTCANVAS_NODE(Rotate);

                void OnInputSignal(const SlotId&) override;

            };
        }
    }
}
