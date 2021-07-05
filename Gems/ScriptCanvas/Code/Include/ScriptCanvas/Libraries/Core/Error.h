/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class Error
                : public Node
            {
            public:
                AZ_COMPONENT(Error, "{C6928F30-87BA-4FFE-A3C0-B6096C161DD0}", Node);
                
                static void Reflect(AZ::ReflectContext* reflection);

                bool IsDeprecated() const override { return true; }

            protected:
                void OnInit() override;
                void OnInputSignal(const SlotId&) override;
            };
        } 
    } 
} 
