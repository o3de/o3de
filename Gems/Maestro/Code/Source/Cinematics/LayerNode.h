/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Header of layer node to control entities properties in the
//               specific layer.

#pragma once

#include "AnimNode.h"

namespace Maestro
{

    class CLayerNode : public CAnimNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CLayerNode, AZ::SystemAllocator);
        AZ_RTTI(CLayerNode, "{C2E65C31-D469-4DE0-8F67-B5B00DE96E52}", CAnimNode);

        //-----------------------------------------------------------------------------
        //!
        CLayerNode();
        CLayerNode(const int id);
        static void Initialize();

        //-----------------------------------------------------------------------------
        //! Overrides from CAnimNode
        void Animate(SAnimContext& ec) override;

        void CreateDefaultTracks() override;

        void OnReset() override;

        void Activate(bool bActivate) override;

        void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;

        //-----------------------------------------------------------------------------
        //! Overrides from IAnimNode
        unsigned int GetParamCount() const override;
        CAnimParamType GetParamType(unsigned int nIndex) const override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const override;

    private:
        bool m_bInit;
        bool m_bPreVisibility;
    };

} // namespace Maestro
