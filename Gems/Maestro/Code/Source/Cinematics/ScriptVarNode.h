/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AnimNode.h"

namespace Maestro
{

    class CAnimScriptVarNode : public CAnimNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CAnimScriptVarNode, AZ::SystemAllocator);
        AZ_RTTI(CAnimScriptVarNode, "{D93FC866-A158-4C00-AB03-29DC7D3CCCFF}", CAnimNode);

        CAnimScriptVarNode(const int id);
        CAnimScriptVarNode();

        //////////////////////////////////////////////////////////////////////////
        // Overrides from CAnimNode
        //////////////////////////////////////////////////////////////////////////
        void Animate(SAnimContext& ec) override;
        void CreateDefaultTracks() override;
        void OnReset() override;
        void OnResume() override;

        //////////////////////////////////////////////////////////////////////////
        unsigned int GetParamCount() const override;
        CAnimParamType GetParamType(unsigned int nIndex) const override;
        bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        float m_value;
    };

} // namespace Maestro
