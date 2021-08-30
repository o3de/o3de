/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "IStatObj.h"

#include <IRenderer.h>
#include <limits>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/algorithm.h>


namespace AZ
{
    class Vector2;
}

struct IMaterial;
struct IRenderNode;
struct IVisArea;
struct SRenderingPassInfo;
struct SRendItemSorter;
struct SFrameLodInfo;
struct pe_params_area;
struct pe_articgeomparams;

struct IRenderNode
{
    // Gives access to object components.
    struct IStatObj* GetEntityStatObj(unsigned int  = 0, unsigned int  = 0, Matrix34A*  = NULL, bool  = false) {
        return nullptr;
    }

    int GetSlotCount() const { return 1; }

    // Max view distance settings.
    static constexpr int VIEW_DISTANCE_MULTIPLIER_MAX = 100;

};
