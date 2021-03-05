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
