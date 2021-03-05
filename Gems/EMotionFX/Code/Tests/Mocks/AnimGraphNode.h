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

namespace EMotionFX
{
    class AnimGraphNode
        : public AnimGraphObject
    {
    public:
        AZ_RTTI(AnimGraphNode, "{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}", AnimGraphObject)

        MOCK_CONST_METHOD1(CollectOutgoingConnections, void(AZStd::vector<AZStd::pair<BlendTreeConnection*, AnimGraphNode*>>& outConnections));
        MOCK_CONST_METHOD2(CollectOutgoingConnections, void(AZStd::vector<AZStd::pair<BlendTreeConnection*, AnimGraphNode*>>& outConnections, const uint32 portIndex));

        MOCK_CONST_METHOD1(FindOutputPortIndex, uint32(const AZStd::string& name));
    };
} // namespace EMotionFX
