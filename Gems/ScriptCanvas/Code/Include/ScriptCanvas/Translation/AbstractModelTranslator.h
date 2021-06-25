/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>

namespace ScriptCanvas
{
    class Graph;

    namespace Grammar
    {
        class AbstractCodeModel;
    }

    namespace Translation
    {
        // move the shared functionality here\
        
        // avoid virtual calls with a virtual function call
        // that defines characters for single line comment
        // block comment open/close, etc
        // function delcarations, etc
        // scope resolution

    } 

}
