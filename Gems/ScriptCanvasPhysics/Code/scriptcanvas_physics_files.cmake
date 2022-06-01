#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

get_property(scriptcanvas_gem_root GLOBAL PROPERTY "@GEMROOT:ScriptCanvas@")

set(FILES
    ${scriptcanvas_gem_root}/Code/Include/ScriptCanvas/AutoGen/ScriptCanvasFunctionRegistry_Header.jinja
    ${scriptcanvas_gem_root}/Code/Include/ScriptCanvas/AutoGen/ScriptCanvasFunctionRegistry_Source.jinja
    Source/ScriptCanvasPhysicsSystemComponent.cpp
    Source/ScriptCanvasPhysicsSystemComponent.h
    Source/World.cpp
    Source/World.h
    Source/World.ScriptCanvasFunction.xml
)
