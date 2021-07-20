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
            class ErrorHandler
                : public Node
            {
            public:
                AZ_COMPONENT(ErrorHandler, "{CF23B5A6-827C-4364-9714-EA99612D6CAE}", Node);

                static void Reflect(AZ::ReflectContext* reflection);

                AZStd::vector<AZStd::pair<Node*, const SlotId>> GetSources() const;

                bool IsDeprecated() const override { return true; }

            protected:
                static const char* k_sourceName;
                void OnInit() override;
            };

        } 
    } 
} 
