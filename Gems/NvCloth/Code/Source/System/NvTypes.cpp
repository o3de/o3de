/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <System/NvTypes.h>

// NvCloth library includes
#include <NvCloth/Factory.h>
#include <NvCloth/Solver.h>
#include <NvCloth/Fabric.h>
#include <NvCloth/Cloth.h>

namespace NvCloth
{
    void NvClothTypesDeleter::operator()(nv::cloth::Factory* factory) const
    {
        NvClothDestroyFactory(factory);
    }

    void NvClothTypesDeleter::operator()(nv::cloth::Solver* solver) const
    {
        // Any cloth instance remaining in the solver must be removed before deleting it.
        while (solver->getNumCloths() > 0)
        {
            solver->removeCloth(*solver->getClothList());
        }
        NV_CLOTH_DELETE(solver);
    }

    void NvClothTypesDeleter::operator()(nv::cloth::Fabric* fabric) const
    {
        fabric->decRefCount();
    }

    void NvClothTypesDeleter::operator()(nv::cloth::Cloth* cloth) const
    {
        NV_CLOTH_DELETE(cloth);
    }
} // namespace NvCloth
