/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "RenderDll_precompiled.h"
#include "OpticsFactory.h"

#include "RootOpticsElement.h"
#include "OpticsElement.h"
#include "Ghost.h"
#include "Glow.h"
#include "ChromaticRing.h"
#include "CameraOrbs.h"
#include "IrisShafts.h"
#include "Streaks.h"
#include "ImageSpaceShafts.h"
#include "OpticsReference.h"
#include "OpticsProxy.h"
#include "OpticsPredef.hpp"

IOpticsElementBase*  COpticsFactory::Create(EFlareType type) const
{
    switch (type)
    {
    case eFT_Root:
        return new RootOpticsElement;
    case eFT_Group:
        return new COpticsGroup("[Group]");
    case eFT_Ghost:
        return new CLensGhost("Ghost");
    case eFT_MultiGhosts:
        return new CMultipleGhost("Multi Ghost");
    case eFT_Glow:
        return new Glow("Glow");
    case eFT_IrisShafts:
        return new IrisShafts("Iris Shafts");
    case eFT_ChromaticRing:
        return new ChromaticRing("Chromatic Ring");
    case eFT_CameraOrbs:
        return new CameraOrbs("Orbs");
    case eFT_ImageSpaceShafts:
        return new ImageSpaceShafts("Vol Shafts");
    case eFT_Streaks:
        return new Streaks("Streaks");
    case eFT_Reference:
        return new COpticsReference("Reference");
    case eFT_Proxy:
        return new COpticsProxy("Proxy");
    default:
        return NULL;
    }
}