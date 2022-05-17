/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>

namespace PhysX
{
    //! Material manager specialization for PhysX.
    class MaterialManager
        : public AZ::Interface<Physics::MaterialManager>::Registrar
    {
    public:
        AZ_RTTI(PhysX::MaterialManager, "{4E0CEA41-A289-44F8-B612-43AC7E2AEE06}", Physics::MaterialManager);

        MaterialManager() = default;

    protected:
        AZStd::shared_ptr<Physics::Material> CreateDefaultMaterialInternal() override;
        AZStd::shared_ptr<Physics::Material> CreateMaterialInternal(const Physics::MaterialId& id, const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset) override;
    };
} // namespace PhysX
