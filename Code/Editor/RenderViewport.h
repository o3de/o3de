/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_RENDERVIEWPORT_H
#define CRYINCLUDE_EDITOR_RENDERVIEWPORT_H

#pragma once
// RenderViewport.h : header file
//

#if !defined(Q_MOC_RUN)
#include <Cry_Camera.h>

#include <QSet>

#include "Viewport.h"
#include "Objects/DisplayContext.h"
#include "Undo/Undo.h"
#include "Util/PredefinedAspectRatios.h"

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <MathConversion.h>
#endif

#include <AzFramework/Windowing/WindowBus.h>
#include <AzFramework/Visibility/EntityVisibilityQuery.h>

// forward declarations.
class CBaseObject;
class QMenu;
class QKeyEvent;
class EditorEntityNotifications;
struct ray_hit;
struct IRenderMesh;
struct IVariable;

namespace AzToolsFramework
{
    class ManipulatorManager;
}

#endif
