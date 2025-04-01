/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class Skeleton;
    class Node;

    class PrefabLeftArmSkeleton
    {
    public:
        enum JointIndexes
        {
            leftShoulderIndex = 0,
            leftElbowIndex = 1,
            leftWristIndex = 2,
            leftHandIndex = 3,
            leftThumb1Index = 4,
            leftThumb2Index = 5,
            leftThumb3Index = 6,
            leftIndex1Index = 7,
            leftIndex2Index = 8,
            leftIndex3Index = 9,
            leftPinky1Index = 10,
            leftPinky2Index = 11,
            leftPinky3Index = 12,
            numJoints = 13,
            INVALID = InvalidIndex
        };

        PrefabLeftArmSkeleton()
        {
            using testing::Return;

            AddJoint(&m_leftShoulder, leftShoulderIndex, nullptr, INVALID, "leftShoulder", leftElbowIndex);

            AddJoint(&m_leftElbow, leftElbowIndex, &m_leftShoulder, leftShoulderIndex, "leftElbow", leftWristIndex);

            AddJoint(&m_leftWrist, leftWristIndex, &m_leftElbow, leftElbowIndex, "leftWrist", leftHandIndex);

            AddJoint(&m_leftHand, leftHandIndex, &m_leftWrist, leftWristIndex, "leftHand", leftThumb1Index, leftIndex1Index, leftPinky1Index);

            AddJoint(&m_leftThumb1, leftThumb1Index, &m_leftHand, leftHandIndex, "leftThumb1", leftThumb2Index);
            AddJoint(&m_leftThumb2, leftThumb2Index, &m_leftThumb1, leftThumb1Index, "leftThumb2", leftThumb3Index);
            AddJoint(&m_leftThumb3, leftThumb3Index, &m_leftThumb2, leftThumb2Index, "leftThumb3");

            AddJoint(&m_leftIndex1, leftIndex1Index, &m_leftHand, leftHandIndex, "leftIndex1", leftIndex2Index);
            AddJoint(&m_leftIndex2, leftIndex2Index, &m_leftIndex1, leftIndex1Index, "leftIndex2", leftIndex3Index);
            AddJoint(&m_leftIndex3, leftIndex3Index, &m_leftIndex2, leftIndex2Index, "leftIndex3");

            AddJoint(&m_leftPinky1, leftPinky1Index, &m_leftHand, leftHandIndex, "leftPinky1", leftPinky2Index);
            AddJoint(&m_leftPinky2, leftPinky2Index, &m_leftPinky1, leftPinky1Index, "leftPinky2", leftPinky3Index);
            AddJoint(&m_leftPinky3, leftPinky3Index, &m_leftPinky2, leftPinky2Index, "leftPinky3");

            EXPECT_CALL(m_skeleton, GetNumNodes())
                .WillRepeatedly(Return(numJoints));
        }

        template<class... T>
        void AddJoint(Node* node, const JointIndexes index, Node* parentNode, const JointIndexes parentIndex, const char* nodeName, T... children)
        {
            using testing::Return;

            EXPECT_CALL(m_skeleton, GetNode(index))
                .WillRepeatedly(Return(node));
            EXPECT_CALL(*node, GetNumChildNodes())
                .WillRepeatedly(Return(static_cast<AZ::u32>(sizeof...(T))));
            EXPECT_CALL(*node, GetNodeIndex())
                .WillRepeatedly(Return(index));
            EXPECT_CALL(*node, GetParentIndex())
                .WillRepeatedly(Return(parentIndex));
            EXPECT_CALL(*node, GetParentNode())
                .WillRepeatedly(Return(parentNode));
            EXPECT_CALL(*node, GetName())
                .WillRepeatedly(Return(nodeName));

            [[maybe_unused]] AZ::u32 i = 0;
            [[maybe_unused]] std::initializer_list<int> dummy = {(([&]() {
                EXPECT_CALL(*node, GetChildIndex(i))
                    .WillRepeatedly(Return(children));
                ++i;
            })(), 0)...};
        }

        Node m_leftShoulder;
        Node m_leftElbow;
        Node m_leftWrist;
        Node m_leftHand;
        Node m_leftThumb1;
        Node m_leftThumb2;
        Node m_leftThumb3;
        Node m_leftIndex1;
        Node m_leftIndex2;
        Node m_leftIndex3;
        Node m_leftPinky1;
        Node m_leftPinky2;
        Node m_leftPinky3;

        Skeleton m_skeleton;
    };
};
