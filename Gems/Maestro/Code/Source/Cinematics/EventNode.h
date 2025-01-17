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

    class CAnimEventNode : public CAnimNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CAnimEventNode, AZ::SystemAllocator);
        AZ_RTTI(CAnimEventNode, "{F9F306E0-FF9C-4FF4-B1CC-5A96746364FE}", CAnimNode);

        CAnimEventNode();
        explicit CAnimEventNode(const int id);

        //////////////////////////////////////////////////////////////////////////
        // Overrides from CAnimNode
        //////////////////////////////////////////////////////////////////////////
        void Animate(SAnimContext& ec) override;
        void CreateDefaultTracks() override;
        void OnReset() override;

        //////////////////////////////////////////////////////////////////////////
        // Supported tracks description.
        //////////////////////////////////////////////////////////////////////////
        unsigned int GetParamCount() const override;
        CAnimParamType GetParamType(unsigned int nIndex) const override;
        bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Last animated key in track.
        int m_lastEventKey;
    };

} // namespace Maestro
