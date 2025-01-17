/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CryMovie animation node for shadow settings

#pragma once

#include "AnimNode.h"

namespace Maestro
{

    class CShadowsSetupNode : public CAnimNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CShadowsSetupNode, AZ::SystemAllocator);
        AZ_RTTI(CShadowsSetupNode, "{419F9F77-FC64-43D1-ABCF-E78E90889DF8}", CAnimNode);

        CShadowsSetupNode();
        explicit CShadowsSetupNode(const int id);

        static void Initialize();

        //-----------------------------------------------------------------------------
        //! Overrides from CAnimNode
        void Animate(SAnimContext& ac) override;

        void CreateDefaultTracks() override;

        void OnReset() override;

        //-----------------------------------------------------------------------------
        //! Overrides from IAnimNode
        unsigned int GetParamCount() const override;
        CAnimParamType GetParamType(unsigned int nIndex) const override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const override;
    };

} // namespace Maestro
