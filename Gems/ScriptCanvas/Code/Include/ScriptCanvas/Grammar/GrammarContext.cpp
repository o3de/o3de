/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GrammarContext.h"

namespace ScriptCanvas
{   
    namespace Grammar
    {
        const SubgraphInterfaceSystem& Context::GetExecutionMapSystem() const
        {
            return m_executionMapSystem;
        }

        SubgraphInterfaceSystem& Context::ModExecutionMapSystem()
        {
            return m_executionMapSystem;
        }
    } 

} 
