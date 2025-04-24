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

    class CFXNodeDescription;

    class CAnimPostFXNode : public CAnimNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CAnimPostFXNode, AZ::SystemAllocator);
        AZ_RTTI(CAnimPostFXNode, "{41FCA8BB-46A8-4F37-87C2-C1D10994854B}", CAnimNode);

        static void Initialize();
        static CFXNodeDescription* GetFXNodeDescription(AnimNodeType nodeType);
        static CAnimNode* CreateNode(const int id, AnimNodeType nodeType);

        CAnimPostFXNode();
        CAnimPostFXNode(const int id, AnimNodeType nodeType, CFXNodeDescription* pDesc);

        void SerializeAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;

        unsigned int GetParamCount() const override;
        CAnimParamType GetParamType(unsigned int nIndex) const override;

        void CreateDefaultTracks() override;

        void OnReset() override;

        void Animate(SAnimContext& ac) override;

        void InitPostLoad(IAnimSequence* sequence) override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const override;

        using FxNodeDescriptionMap = AZStd::map<AnimNodeType, AZStd::unique_ptr<CFXNodeDescription>>;
        static FxNodeDescriptionMap s_fxNodeDescriptions;
        static inline bool s_initialized{};

        CFXNodeDescription* m_pDescription;
    };

} // namespace Maestro
