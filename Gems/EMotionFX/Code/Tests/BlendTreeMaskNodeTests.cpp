/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/AnimGraphFixture.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeMaskNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>

#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>

namespace EMotionFX
{
    class BlendTreeTestInputNode
        : public AnimGraphNode
    {
    public:
        AZ_CLASS_ALLOCATOR(BlendTreeTestInputNode, AnimGraphAllocator)
        AZ_RTTI(BlendTreeTestInputNode, "{72595B5C-045C-4DB1-88A4-40BC4560D7AF}", AnimGraphNode)

        enum
        {
            OUTPUTPORT_RESULT = 0
        };

        BlendTreeTestInputNode(float value)
            : AnimGraphNode()
            , m_identificationValue(value)
        {
            InitOutputPorts(1);
            SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, OUTPUTPORT_RESULT);
        }

        AZ::Color GetVisualColor() const override { return AZ::Color(1.0f, 1.0f, 0.0f, 1.0f); }
        bool GetHasOutputPose() const override { return true; }
        const char* GetPaletteName() const override { return "BlendTreeTestInputNode"; }
        AnimGraphObject::ECategory GetPaletteCategory() const override { return AnimGraphObject::CATEGORY_SOURCES; }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        bool InitAfterLoading(AnimGraph* animGraph) override
        {
            if (!AnimGraphNode::InitAfterLoading(animGraph))
            {
                return false;
            }

            InitInternalAttributesForAllInstances();

            Reinit();
            return true;
        }

        void Output(AnimGraphInstance* animGraphInstance) override
        {
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputAnimGraphPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputAnimGraphPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            Pose& outputPose = outputAnimGraphPose->GetPose();

            // Output the assigned value of the node for each joint so that we can identify from which input each joint is coming from.
            const size_t numJoints = outputPose.GetNumTransforms();
            for (size_t i = 0; i < numJoints; ++i)
            {
                Transform transform = outputPose.GetLocalSpaceTransform(i);
                transform.m_position = AZ::Vector3(m_identificationValue, m_identificationValue, m_identificationValue);
                outputPose.SetLocalSpaceTransform(i, transform);
            }
        }

    private:
        float m_identificationValue;
    };


    using MaskNodeTestParam = std::vector<std::vector<std::string>>;

    /*
     * The general idea is to identify the origin of the joints by embedding identification values into the joint transform
     * and inside the test extract that value and thus know from which mask input it belongs to.
     * We create a blend tree with a mask node having several input nodes. The first one representing the base pose and three
     * input mask nodes with a customizable mask which comes in by the test parameter.
     * We run several tests with different variations of masks and check if the output transforms for each joint corresponds with
     * the set masks and if the mask node picked and overwrote the correct transforms.
    */
    class BlendTreeMaskNodeTestFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<MaskNodeTestParam>
    {
    public:
        void ConstructActor() override
        {
            m_actor = ActorFactory::CreateAndInit<AllRootJointsActor>(5);
        }

        AZStd::vector<AZStd::string> ConstructMask(const std::vector<std::string>& in)
        {
            AZStd::vector<AZStd::string> result;
            result.reserve(in.size());
            for (const std::string& str : in)
            {
                result.emplace_back(AZStd::string(str.c_str(), str.size()));
            }
            return result;
        }

        AZ::Outcome<size_t> FindMaskIndexForJoint(size_t jointIndex) const
        {
            const MaskNodeTestParam& param = GetParam();
            Skeleton* skeleton = m_actor->GetSkeleton();
            const size_t numMasks = param.size();

            for (size_t maskIndex = 0; maskIndex < numMasks; ++maskIndex)
            {
                const std::vector<std::string>& mask = param[maskIndex];

                const Node* joint = skeleton->GetNode(jointIndex);
                const char* jointName = joint->GetName();

                // Is joint in the current mask? Return the index in this case.
                if (std::find(mask.begin(), mask.end(), jointName) != mask.end())
                {
                    return AZ::Success(maskIndex);
                }
            }

            return AZ::Failure();
        }

        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            const MaskNodeTestParam& param = GetParam();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            /*
            +-----------+
            | Base Pose +----------+
            +-----------+          |
                                   |
            +----------+           >+-----------+               +-------+
            | Mask 0   +----------->| Pose Mask +-------------->+ Final |
            +----------+     ------>|           |               +-------+
                             |     >+-----------+
            +----------+     |     |
            | Mask 1   +-----+     |
            +----------+           |
                                   |
            +-------------+        |
            | Mask 3      +--------+
            +-------------+
            */

            m_maskNode = aznew BlendTreeMaskNode();
            m_blendTree->AddChildNode(m_maskNode);

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_blendTree->AddChildNode(finalNode);
            finalNode->AddConnection(m_maskNode, BlendTreeMaskNode::OUTPUTPORT_RESULT, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_basePoseNode = aznew BlendTreeTestInputNode(static_cast<float>(m_basePosePosValue));
            m_blendTree->AddChildNode(m_basePoseNode);
            m_maskNode->AddConnection(m_basePoseNode, BlendTreeTestInputNode::OUTPUTPORT_RESULT, BlendTreeMaskNode::INPUTPORT_BASEPOSE);

            for (uint16 i = 0; i < m_numMaskInputNodes; ++i)
            {
                BlendTreeTestInputNode* inputNode = aznew BlendTreeTestInputNode(static_cast<float>(i));
                m_blendTree->AddChildNode(inputNode);
                m_maskNode->AddConnection(inputNode, BlendTreeTestInputNode::OUTPUTPORT_RESULT, BlendTreeMaskNode::INPUTPORT_START + i);
                m_maskInputNodes.push_back(inputNode);
            }

            const size_t numMasks = param.size();
            ASSERT_EQ(numMasks, m_numMaskInputNodes)
                << "The number of provides masks in the parameter (" << numMasks << ") should match the number of created "
                << "input mask nodes (" << m_numMaskInputNodes << ").";

            for (size_t i = 0; i < numMasks; ++i)
            {
                m_maskNode->SetMask(i, ConstructMask(param[i]));
            }

            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

    public:
        BlendTreeMaskNode* m_maskNode = nullptr;
        BlendTreeTestInputNode* m_basePoseNode = nullptr;
        const size_t m_basePosePosValue = 100; // Special identification value for the base pose to easily distinguish it from the mask indices.
        std::vector<BlendTreeTestInputNode*> m_maskInputNodes;
        size_t m_numMaskInputNodes = 3;
        BlendTree* m_blendTree = nullptr;
    };

    TEST_P(BlendTreeMaskNodeTestFixture, MaskTests)
    {
        GetEMotionFX().Update(0.0f);

        Skeleton* skeleton = m_actor->GetSkeleton();
        const size_t numJoints = skeleton->GetNumNodes();
        TransformData* transformData = m_actorInstance->GetTransformData();
        Pose* pose = transformData->GetCurrentPose();

        // Iterate through the joints and make sure their transforms originate according to the mask setup.
        for (size_t jointIndex = 0; jointIndex < numJoints; jointIndex++)
        {
            const Node* joint = skeleton->GetNode(jointIndex);
            const char* jointName = joint->GetName();
            const Transform& transform = pose->GetModelSpaceTransform(jointIndex);

            // The components of the position embed the origin.
            // If the compareValue equals m_basePosePosValue, it originates from the base pose input.
            // In case the joint is part of any of the masks and got overwriten by them, the compareValue represents the mask index.
            const size_t compareValue = static_cast<size_t>(transform.m_position.GetX());

            AZ::Outcome<size_t> maskIndex = FindMaskIndexForJoint(jointIndex);
            if (maskIndex.IsSuccess())
            {
                EXPECT_EQ(compareValue, maskIndex.GetValue())
                    << "Joint '" << jointName << "' is part of mask " << maskIndex.GetValue()
                    << " while the transform originated from input number " << compareValue
                    << ".";
            }
            else
            {
                EXPECT_EQ(compareValue, m_basePosePosValue)
                    << "Joint '" << jointName << "' is not part of any mask while the transform "
                    << "originated from input number " << compareValue << ". It should originate "
                    << "from the base pose input.";
            }
        }
    }

    std::vector<MaskNodeTestParam> maskNodeTestData
    {
        {
            {},
            {},
            {},
        },
        {
            { "rootJoint" },
            {},
            {},
        },
        {
            { "rootJoint", "joint2" },
            {},
            {},
        },
        {
            { "rootJoint", "joint1", "joint2" },
            {},
            {},
        },
        {
            { "rootJoint", "joint1", "joint2", "joint3", "joint4" },
            {},
            {},
        },
        {
            {},
            { "joint1", "joint3" },
            {},
        },
        {
            {},
            {},
            { "joint2", "joint4" },
        },
        {
            { "rootJoint", "joint1" },
            { "joint3", "joint4" },
            {},
        },
        {
            { "rootJoint", "joint1" },
            {},
            { "joint3", "joint4" },
        },
        {
            {},
            { "rootJoint", "joint1" },
            { "joint3", "joint4" },
        },
        {
            { "rootJoint" },
            { "joint1", "joint2" },
            { "joint3", "joint4" },
        },
    };

    INSTANTIATE_TEST_CASE_P(BlendTreeMaskNode,
        BlendTreeMaskNodeTestFixture,
            ::testing::ValuesIn(maskNodeTestData));
} // namespace EMotionFX
