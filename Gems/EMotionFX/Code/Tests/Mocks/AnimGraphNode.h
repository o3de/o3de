/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class AnimGraphNode
        : public AnimGraphObject
    {
    public:
        AZ_RTTI(AnimGraphNode, "{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}", AnimGraphObject)

        virtual ~AnimGraphNode() = default;

        MOCK_CONST_METHOD1(CollectOutgoingConnections, void(AZStd::vector<AZStd::pair<BlendTreeConnection*, AnimGraphNode*>>& outConnections));
        MOCK_CONST_METHOD2(CollectOutgoingConnections, void(AZStd::vector<AZStd::pair<BlendTreeConnection*, AnimGraphNode*>>& outConnections, const size_t portIndex));

        MOCK_CONST_METHOD1(FindOutputPortIndex, size_t(const AZStd::string& name));
    };
} // namespace EMotionFX
