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

    class CCommentContext;

    class CCommentNode : public CAnimNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CCommentNode, AZ::SystemAllocator);
        AZ_RTTI(CCommentNode, "{9FCBF56F-B7B3-4519-B3D2-9B7E5F7E6210}", CAnimNode);

        CCommentNode(const int id);
        CCommentNode();

        static void Initialize();

        //-----------------------------------------------------------------------------
        //! Overrides from CAnimNode
        void Animate(SAnimContext& ac) override;

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
    };

} // namespace Maestro
