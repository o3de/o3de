/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/Component/EntityId.h>

namespace AzToolsFramework
{
    //! Builds a working terrain setup in one action.
    //!
    //! TODO: this belongs in Gems/Terrain, not in AzToolsFramework. Every component it names is
    //! supplied by the Terrain, GradientSignal, SurfaceData and Vegetation gems, and the framework
    //! should not know they exist. It compiles here only because components are named by display
    //! string, and it does the right thing at runtime only because the lookups fail gracefully when
    //! those gems are disabled. Moving it out means giving the gem somewhere to register an action
    //! into EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier, which it can already do.
    //!
    //! Terrain is the one setup that does not fit the ordinary preset model, and it misses by
    //! three separate margins rather than one:
    //!
    //! * Some of its components go on the **level entity**, not on a new entity. Nothing else in
    //!   the preset library touches the level.
    //! * It needs **two entities in a parent/child relationship** - a spawner defining the region,
    //!   and a child supplying the height data.
    //! * They have to **refer to each other**: the child's Shape Reference points back at the
    //!   parent's box, and the parent's gradient lists are *containers* of entity ids that have to
    //!   include the child. Presets only carry scalar property values.
    //!
    //! So this is written out by hand rather than expressed as data. The trade is deliberate: it
    //! cannot be edited in the preset editor or duplicated as a starting point, and the next setup
    //! needing a child entity will need the same treatment.
    namespace TerrainPreset
    {
        //! How much of the stack to build.
        //!
        //! These are a progression rather than alternatives - each is the one before it plus a
        //! layer, so starting simple and rebuilding bigger costs nothing.
        enum class Variant
        {
            //! Height only: a region, and noise driving its elevation. Enough to have ground to
            //! stand on, and the fastest to understand when something goes wrong.
            Simple,

            //! The same terrain, parented under an entity carrying the Landscape Canvas component
            //! so the whole setup opens as a node graph. A Landscape Canvas graph is not a file -
            //! it is an entity whose descendants' components are its nodes - so this is a matter of
            //! where the entities sit, and needs no dependency on that gem.
            Landscape,

            //! Landscape plus a vegetation layer planting on it, and the level's vegetation
            //! settings. The asset list is left empty - there is no sensible default plant to
            //! choose - so nothing appears until a mesh or prefab is added to it.
            LandscapeWithVegetation
        };

        //! Create the terrain.
        //!
        //! Adds whatever level components the variant needs if the level lacks them, builds the
        //! spawner and its height provider, then wires them together. All in a single undo batch,
        //! so one Ctrl+Z removes the whole setup.
        //!
        //! @return The spawner entity, or an invalid id if it could not be built.
        AZTF_API AZ::EntityId Create(Variant variant);
    } // namespace TerrainPreset
} // namespace AzToolsFramework
