/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


/*
    Any class that implements a nodeable AzAutoGen driver (i.e. *.ScriptCanvasNodeable.xml)
    requires that the SCRIPTCANVAS_NODE macro be declared within its class declaration.

    Example:

    class CustomNode
        : public ScriptCanvas::Nodeable
    {
    public:
            SCRIPTCANVAS_NODE(CustomNode);
    };

    What will happen is that when AzAutoGen runs it will generate a preprocessor directive:

    SCRIPTCANVAS_NODE_CustomNode

    Which will define all of the node's boilerplate code and definitions. When CustomNode
    is compiled, the preprocessor will replace the macro with the auto generated
    code.
*/

#define SCRIPTCANVAS_NODE(ClassName) SCRIPTCANVAS_NODE_##ClassName

