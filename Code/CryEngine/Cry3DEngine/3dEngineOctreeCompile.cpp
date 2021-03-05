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

#include "Cry3DEngine_precompiled.h"

#include "3dEngine.h"
#include "Ocean.h"
#include "StatObj.h"
#include "ObjMan.h"
#include "MatMan.h"
#include "VisAreas.h"
#include "ObjectsTree.h"

#include <CryPath.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <MathConversion.h>
#include <StatObjBus.h>
#include <PakLoadDataUtils.h>
#include <AzCore/Console/IConsole.h>


#define SIGC_HIDEABILITY          BIT(3)
#define SIGC_HIDEABILITYSECONDARY BIT(4)
#define SIGC_PROCEDURALLYANIMATED BIT(6)
#define SIGC_CASTSHADOW           BIT(7) // Deprecated
#define SIGC_RECVSHADOW           BIT(8)
#define SIGC_DYNAMICDISTANCESHADOWS BIT(9)
#define SIGC_USEALPHABLENDING     BIT(10)
#define SIGC_RANDOMROTATION       BIT(12)
#define SIGC_ALLOWINDOOR                    BIT(13)

// Bits 13-14 reserved for player hideability
#define SIGC_PLAYERHIDEABLE_LOWBIT (13)
#define SIGC_PLAYERHIDEABLE_MASK   BIT(13) | BIT(14)

#define SIGC_CASTSHADOW_MINSPEC_SHIFT (15)

// Get the number of bits needed for the maximum spec level
#define SIGC_CASTSHADOW_MINSPEC_MASK_NUM_BITS_NEEDED (IntegerLog2(uint32(END_CONFIG_SPEC_ENUM - 1)) + 1)

// Create a mask based on the number of bits needed
#define SIGC_CASTSHADOW_MINSPEC_MASK_BITS ((1 << SIGC_CASTSHADOW_MINSPEC_MASK_NUM_BITS_NEEDED) - 1)
#define SIGC_CASTSHADOW_MINSPEC_MASK  (SIGC_CASTSHADOW_MINSPEC_MASK_BITS << SIGC_CASTSHADOW_MINSPEC_SHIFT)

AZ_CVAR(float, bg_DefaultMaxOctreeWorldSize, 4096.0f, nullptr, AZ::ConsoleFunctorFlags::NeedsReload, "Default world size to use for the octree when terrain is not present.");

bool C3DEngine::CreateOctree(float maxRootOctreeNodeSize)
{
    float rootOctreeNodeSize = (maxRootOctreeNodeSize > 0.0f) ? maxRootOctreeNodeSize : bg_DefaultMaxOctreeWorldSize;
    COctreeNode* newOctreeNode = COctreeNode::Create(DEFAULT_SID, AABB(Vec3(0), Vec3(rootOctreeNodeSize)), NULL);
    if (!newOctreeNode)
    {
        Error("Failed to create octree with initial world size=%f", rootOctreeNodeSize);
        return false;
    }
    SetObjectTree(newOctreeNode);
    Cry3DEngineBase::GetObjManager()->GetListStaticTypes().PreAllocate(1, 1);
    Cry3DEngineBase::GetObjManager()->GetListStaticTypes()[DEFAULT_SID].Reset();
    return true;
}


void C3DEngine::DestroyOctree()
{
    COctreeNode* objectTree = GetObjectTree();
    if (objectTree)
    {
        delete objectTree;
        SetObjectTree(nullptr);
    }
}

#define RAD2BYTE(x) ((x)*255.0f / float(g_PI2))
#define BYTE2RAD(x) ((x)* float(g_PI2) / 255.0f)

#include "TypeInfo_impl.h"

