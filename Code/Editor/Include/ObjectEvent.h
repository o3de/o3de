/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_INCLUDE_OBJECTEVENT_H
#define CRYINCLUDE_EDITOR_INCLUDE_OBJECTEVENT_H
#pragma once

//! Standart objects types.
enum ObjectType
{
    OBJTYPE_AZENTITY        = 1 << 21,
};

//////////////////////////////////////////////////////////////////////////
//! Events that objects may want to handle.
//! Passed to OnEvent method of CBaseObject.
enum ObjectEvent
{
    EVENT_INGAME    = 1,    //!< Signals that editor is switching into the game mode.
    EVENT_OUTOFGAME,        //!< Signals that editor is switching out of the game mode.
    EVENT_REFRESH,          //!< Signals that editor is refreshing level.
    EVENT_DBLCLICK,         //!< Signals that object have been double clicked.
    EVENT_RELOAD_ENTITY,//!< Signals that entities scripts must be reloaded.
    EVENT_RELOAD_GEOM,  //!< Signals that all possible geometries should be reloaded.
    EVENT_UNLOAD_GEOM,  //!< Signals that all possible geometries should be unloaded.
    EVENT_MISSION_CHANGE,   //!< Signals that mission have been changed.
    EVENT_ALIGN_TOGRID, //!< Object should align itself to the grid.

    EVENT_PHYSICS_GETSTATE,//!< Signals that entities should accept their physical state from game.
    EVENT_PHYSICS_RESETSTATE,//!< Signals that physics state must be reseted on objects.
    EVENT_PHYSICS_APPLYSTATE,//!< Signals that the stored physics state must be applied to objects.

    EVENT_FREE_GAME_DATA,//!< Object should free game data that its holding.
    EVENT_CONFIG_SPEC_CHANGE,   //!< Called when config spec changed.
    EVENT_HIDE_HELPER, //!< Signals that happens when Helper mode switches to be hidden.
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_OBJECTEVENT_H

