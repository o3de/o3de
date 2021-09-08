/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <CryCommon/Cry_Matrix34.h>

struct IStatObj;

struct IRenderNode
{
    // Gives access to object components.
    IStatObj* GetEntityStatObj(unsigned int  = 0, unsigned int  = 0, Matrix34* = nullptr, bool  = false) {
        return nullptr;
    }

    int GetSlotCount() const { return 1; }

    // Max view distance settings.
    static constexpr int VIEW_DISTANCE_MULTIPLIER_MAX = 100;

};
