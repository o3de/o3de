/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Node.h>
#include <Tests/TestAssetCode/JackActor.h>

namespace EMotionFX
{
    JackNoMeshesActor::JackNoMeshesActor(const char* name)
        : Actor(name)
    {
        size_t nodeId = 0;
        auto root = AddNode(nodeId++, "jack_root");
        auto Bip01__pelvis = AddNode(nodeId++, "Bip01__pelvis", root->GetNodeIndex());
        auto l_upLeg = AddNode(nodeId++, "l_upLeg", Bip01__pelvis->GetNodeIndex());
        auto l_upLegRoll = AddNode(nodeId++, "l_upLegRoll", l_upLeg->GetNodeIndex());
        auto l_loLeg = AddNode(nodeId++, "l_loLeg", l_upLeg->GetNodeIndex());
        auto l_ankle = AddNode(nodeId++, "l_ankle", l_loLeg->GetNodeIndex());
        auto l_ball = AddNode(nodeId++, "l_ball", l_ankle->GetNodeIndex());
        /*auto Bip01__L_Heel =*/ AddNode(nodeId++, "Bip01__L_Heel", l_ankle->GetNodeIndex());
        /*auto Bip01__planeTargetLeft =*/ AddNode(nodeId++, "Bip01__planeTargetLeft", l_ankle->GetNodeIndex());
        /*auto Bip01__planeWeightLeft =*/ AddNode(nodeId++, "Bip01__planeWeightLeft", l_ankle->GetNodeIndex());
        auto r_upLeg = AddNode(nodeId++, "r_upLeg", Bip01__pelvis->GetNodeIndex());
        auto r_upLegRoll = AddNode(nodeId++, "r_upLegRoll", r_upLeg->GetNodeIndex());
        auto r_loLeg = AddNode(nodeId++, "r_loLeg", r_upLeg->GetNodeIndex());
        auto r_ankle = AddNode(nodeId++, "r_ankle", r_loLeg->GetNodeIndex());
        auto r_ball = AddNode(nodeId++, "r_ball", r_ankle->GetNodeIndex());
        /*auto Bip01__R_Heel =*/ AddNode(nodeId++, "Bip01__R_Heel", r_ankle->GetNodeIndex());
        /*auto Bip01__planeTargetRight =*/ AddNode(nodeId++, "Bip01__planeTargetRight", r_ankle->GetNodeIndex());
        /*auto Bip01__planeWeightRight =*/ AddNode(nodeId++, "Bip01__planeWeightRight", r_ankle->GetNodeIndex());
        auto spine1 = AddNode(nodeId++, "spine1", Bip01__pelvis->GetNodeIndex());
        auto spine2 = AddNode(nodeId++, "spine2", spine1->GetNodeIndex());
        auto spine3 = AddNode(nodeId++, "spine3", spine2->GetNodeIndex());
        auto neck = AddNode(nodeId++, "neck", spine3->GetNodeIndex());
        auto head = AddNode(nodeId++, "head", neck->GetNodeIndex());
        auto l_shldr = AddNode(nodeId++, "l_shldr", spine3->GetNodeIndex());
        auto l_upArm = AddNode(nodeId++, "l_upArm", l_shldr->GetNodeIndex());
        auto l_upArmRoll = AddNode(nodeId++, "l_upArmRoll", l_upArm->GetNodeIndex());
        auto l_loArm = AddNode(nodeId++, "l_loArm", l_upArm->GetNodeIndex());
        auto l_loArmRoll = AddNode(nodeId++, "l_loArmRoll", l_loArm->GetNodeIndex());
        auto l_hand = AddNode(nodeId++, "l_hand", l_loArm->GetNodeIndex());
        auto l_thumb1 = AddNode(nodeId++, "l_thumb1", l_hand->GetNodeIndex());
        auto l_thumb2 = AddNode(nodeId++, "l_thumb2", l_thumb1->GetNodeIndex());
        auto l_thumb3 = AddNode(nodeId++, "l_thumb3", l_thumb2->GetNodeIndex());
        auto l_index1 = AddNode(nodeId++, "l_index1", l_hand->GetNodeIndex());
        auto l_index2 = AddNode(nodeId++, "l_index2", l_index1->GetNodeIndex());
        auto l_index3 = AddNode(nodeId++, "l_index3", l_index2->GetNodeIndex());
        auto l_mid1 = AddNode(nodeId++, "l_mid1", l_hand->GetNodeIndex());
        auto l_mid2 = AddNode(nodeId++, "l_mid2", l_mid1->GetNodeIndex());
        auto l_mid3 = AddNode(nodeId++, "l_mid3", l_mid2->GetNodeIndex());
        auto l_metacarpal = AddNode(nodeId++, "l_metacarpal", l_hand->GetNodeIndex());
        auto l_ring1 = AddNode(nodeId++, "l_ring1", l_metacarpal->GetNodeIndex());
        auto l_ring2 = AddNode(nodeId++, "l_ring2", l_ring1->GetNodeIndex());
        auto l_ring3 = AddNode(nodeId++, "l_ring3", l_ring2->GetNodeIndex());
        auto l_pinky1 = AddNode(nodeId++, "l_pinky1", l_metacarpal->GetNodeIndex());
        auto l_pinky2 = AddNode(nodeId++, "l_pinky2", l_pinky1->GetNodeIndex());
        auto l_pinky3 = AddNode(nodeId++, "l_pinky3", l_pinky2->GetNodeIndex());
        auto l_handProp = AddNode(nodeId++, "l_handProp", l_hand->GetNodeIndex());
        /*auto Bip01__RHand2Weapon_IKBlend =*/ AddNode(nodeId++, "Bip01__RHand2Weapon_IKBlend", l_handProp->GetNodeIndex());
        /*auto Bip01__RHand2Weapon_IKTarget =*/ AddNode(nodeId++, "Bip01__RHand2Weapon_IKTarget", l_handProp->GetNodeIndex());
        auto r_shldr = AddNode(nodeId++, "r_shldr", spine3->GetNodeIndex());
        auto r_upArm = AddNode(nodeId++, "r_upArm", r_shldr->GetNodeIndex());
        auto r_upArmRoll = AddNode(nodeId++, "r_upArmRoll", r_upArm->GetNodeIndex());
        auto r_loArm = AddNode(nodeId++, "r_loArm", r_upArm->GetNodeIndex());
        auto r_loArmRoll = AddNode(nodeId++, "r_loArmRoll", r_loArm->GetNodeIndex());
        auto r_hand = AddNode(nodeId++, "r_hand", r_loArm->GetNodeIndex());
        auto r_thumb1 = AddNode(nodeId++, "r_thumb1", r_hand->GetNodeIndex());
        auto r_thumb2 = AddNode(nodeId++, "r_thumb2", r_thumb1->GetNodeIndex());
        auto r_thumb3 = AddNode(nodeId++, "r_thumb3", r_thumb2->GetNodeIndex());
        auto r_index1 = AddNode(nodeId++, "r_index1", r_hand->GetNodeIndex());
        auto r_index2 = AddNode(nodeId++, "r_index2", r_index1->GetNodeIndex());
        auto r_index3 = AddNode(nodeId++, "r_index3", r_index2->GetNodeIndex());
        auto r_mid1 = AddNode(nodeId++, "r_mid1", r_hand->GetNodeIndex());
        auto r_mid2 = AddNode(nodeId++, "r_mid2", r_mid1->GetNodeIndex());
        auto r_mid3 = AddNode(nodeId++, "r_mid3", r_mid2->GetNodeIndex());
        auto r_metacarpal = AddNode(nodeId++, "r_metacarpal", r_hand->GetNodeIndex());
        auto r_ring1 = AddNode(nodeId++, "r_ring1", r_metacarpal->GetNodeIndex());
        auto r_ring2 = AddNode(nodeId++, "r_ring2", r_ring1->GetNodeIndex());
        auto r_ring3 = AddNode(nodeId++, "r_ring3", r_ring2->GetNodeIndex());
        auto r_pinky1 = AddNode(nodeId++, "r_pinky1", r_metacarpal->GetNodeIndex());
        auto r_pinky2 = AddNode(nodeId++, "r_pinky2", r_pinky1->GetNodeIndex());
        auto r_pinky3 = AddNode(nodeId++, "r_pinky3", r_pinky2->GetNodeIndex());
        auto r_handProp = AddNode(nodeId++, "r_handProp", r_hand->GetNodeIndex());
        /*auto Bip01__LHand2Weapon_IKBlend =*/ AddNode(nodeId++, "Bip01__LHand2Weapon_IKBlend", r_handProp->GetNodeIndex());
        /*auto Bip01__LHand2Weapon_IKTarget =*/ AddNode(nodeId++, "Bip01__LHand2Weapon_IKTarget", r_handProp->GetNodeIndex());
        /*auto Bip01__CustomStart =*/ AddNode(nodeId++, "Bip01__CustomStart", spine3->GetNodeIndex());
        /*auto Bip01__CustomAim =*/ AddNode(nodeId++, "Bip01__CustomAim", spine2->GetNodeIndex());
        /*auto Bip01__RHand2Aim_IKBlend =*/ AddNode(nodeId++, "RHand2Aim_IKBlend", spine2->GetNodeIndex());

        Pose* bindPose = GetBindPose();
        bindPose->SetModelSpaceTransform(Bip01__pelvis->GetNodeIndex(), {AZ::Vector3(2.52997e-16f, 7.26991e-16f, 1.02314f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0f)});
        bindPose->SetModelSpaceTransform(l_upLeg->GetNodeIndex(), {AZ::Vector3(0.100606f, -0.0114117f, 0.990395f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0f)});
        bindPose->SetModelSpaceTransform(r_upLeg->GetNodeIndex(), {AZ::Vector3(-0.10061f, -0.0114117f, 0.990395f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0f)});
        bindPose->SetModelSpaceTransform(spine1->GetNodeIndex(), {AZ::Vector3(1.2988e-16f, -0.018f, 1.09f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-4.21496e-17f, 0.688355f, -0.725374f), 3.14159f)});
        bindPose->SetModelSpaceTransform(l_upLegRoll->GetNodeIndex(), {AZ::Vector3(0.100606f, -0.011412f, 0.767395f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0f)});
        bindPose->SetModelSpaceTransform(l_loLeg->GetNodeIndex(), {AZ::Vector3(0.100606f, -0.0114122f, 0.544395f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0f)});
        bindPose->SetModelSpaceTransform(r_upLegRoll->GetNodeIndex(), {AZ::Vector3(-0.10061f, -0.011412f, 0.767395f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0f)});
        bindPose->SetModelSpaceTransform(r_loLeg->GetNodeIndex(), {AZ::Vector3(-0.10061f, -0.0114122f, 0.544395f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0f)});
        bindPose->SetModelSpaceTransform(spine2->GetNodeIndex(), {AZ::Vector3(5.34234e-17f, -0.0101496f, 1.23979f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-4.09724e-17f, 0.669131f, -0.743145f), 3.14159f)});
        bindPose->SetModelSpaceTransform(l_ankle->GetNodeIndex(), {AZ::Vector3(0.100606f, -0.0114128f, 0.0932541f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0f)});
        bindPose->SetModelSpaceTransform(r_ankle->GetNodeIndex(), {AZ::Vector3(-0.10061f, -0.0114128f, 0.0932541f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0f)});
        bindPose->SetModelSpaceTransform(spine3->GetNodeIndex(), {AZ::Vector3(2.65345e-18f, 0.00552972f, 1.38897f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-3.97672e-17f, 0.649448f, -0.760406f), 3.14159f)});
        bindPose->SetModelSpaceTransform(l_ball->GetNodeIndex(), {AZ::Vector3(0.100606f, -0.161586f, 0.0260659f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0f)});
        bindPose->SetModelSpaceTransform(r_ball->GetNodeIndex(), {AZ::Vector3(-0.10061f, -0.161586f, 0.0260659f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0f)});
        bindPose->SetModelSpaceTransform(neck->GetNodeIndex(), {AZ::Vector3(-8.98044e-17f, 0.0352523f, 1.57663f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-4.32978e-17f, 0.707107f, -0.707107f), 3.14159f)});
        bindPose->SetModelSpaceTransform(l_shldr->GetNodeIndex(), {AZ::Vector3(0.0819695f, 0.03399f, 1.50163f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(8.42937e-08f, -8.42937e-08f, -1.0f), 4.71239f)});
        bindPose->SetModelSpaceTransform(r_shldr->GetNodeIndex(), {AZ::Vector3(-0.0819695f, 0.03399f, 1.50163f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.707107f, 0.707107f, -5.96046e-08f), 3.14159f)});
        bindPose->SetModelSpaceTransform(head->GetNodeIndex(), {AZ::Vector3(-8.66743e-17f, 0.0183504f, 1.67076f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-4.32978e-17f, 0.707107f, -0.707107f), 3.14159f)});
        bindPose->SetModelSpaceTransform(l_upArm->GetNodeIndex(), {AZ::Vector3(0.197941f, 0.0456615f, 1.45918f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.678598f, -0.678598f, -0.281085f), 3.68962f)});
        bindPose->SetModelSpaceTransform(r_upArm->GetNodeIndex(), {AZ::Vector3(-0.197941f, 0.0456615f, 1.45918f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.678598f, 0.678598f, 0.281085f), 3.68962f)});
        bindPose->SetModelSpaceTransform(l_upArmRoll->GetNodeIndex(), {AZ::Vector3(0.287744f, 0.0456615f, 1.36937f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.678598f, -0.678598f, -0.281085f), 3.68962f)});
        bindPose->SetModelSpaceTransform(l_loArm->GetNodeIndex(), {AZ::Vector3(0.377546f, 0.0456614f, 1.27957f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.746307f, -0.644711f, -0.165452f), 3.92103f)});
        bindPose->SetModelSpaceTransform(r_upArmRoll->GetNodeIndex(), {AZ::Vector3(-0.287744f, 0.0456615f, 1.36937f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.678598f, 0.678598f, 0.281085f), 3.68962f)});
        bindPose->SetModelSpaceTransform(r_loArm->GetNodeIndex(), {AZ::Vector3(-0.377546f, 0.0456614f, 1.27957f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.746307f, 0.644711f, 0.165452f), 3.92103f)});
        bindPose->SetModelSpaceTransform(l_loArmRoll->GetNodeIndex(), {AZ::Vector3(0.469411f, -0.00162434f, 1.18771f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.746307f, -0.644711f, -0.165452f), 3.92103f)});
        bindPose->SetModelSpaceTransform(l_hand->GetNodeIndex(), {AZ::Vector3(0.561276f, -0.0489101f, 1.09584f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.759761f, -0.641015f, -0.108917f), 3.79613f)});
        bindPose->SetModelSpaceTransform(r_loArmRoll->GetNodeIndex(), {AZ::Vector3(-0.469411f, -0.00162434f, 1.18771f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.746307f, 0.644711f, 0.165452f), 3.92103f)});
        bindPose->SetModelSpaceTransform(r_hand->GetNodeIndex(), {AZ::Vector3(-0.561276f, -0.0489101f, 1.09584f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.759761f, 0.641015f, 0.108917f), 3.79613f)});
        bindPose->SetModelSpaceTransform(l_thumb1->GetNodeIndex(), {AZ::Vector3(0.569857f, -0.104531f, 1.07793f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.84792f, -0.397456f, -0.350802f), 5.75265f)});
        bindPose->SetModelSpaceTransform(l_index1->GetNodeIndex(), {AZ::Vector3(0.617146f, -0.105998f, 1.03483f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.350114f, -0.615319f, -0.70626f), 4.82869f)});
        bindPose->SetModelSpaceTransform(l_mid1->GetNodeIndex(), {AZ::Vector3(0.618331f, -0.0877503f, 1.0272f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.347031f, -0.552723f, -0.757672f), 4.65153f)});
        bindPose->SetModelSpaceTransform(l_metacarpal->GetNodeIndex(), {AZ::Vector3(0.586811f, -0.0484658f, 1.05823f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.432044f, -0.44393f, -0.785025f), 4.63799f)});
        bindPose->SetModelSpaceTransform(l_handProp->GetNodeIndex(), {AZ::Vector3(0.581079f, -0.070114f, 1.04127f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.415703f, -0.526297f, -0.741756f), 4.79594f)});
        bindPose->SetModelSpaceTransform(r_thumb1->GetNodeIndex(), {AZ::Vector3(-0.569857f, -0.104531f, 1.07793f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.989788f, 0.0943283f, -0.106873f), 2.69326f)});
        bindPose->SetModelSpaceTransform(r_index1->GetNodeIndex(), {AZ::Vector3(-0.617146f, -0.105998f, 1.03483f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.768104f, 0.482794f, -0.420627f), 2.67176f)});
        bindPose->SetModelSpaceTransform(r_mid1->GetNodeIndex(), {AZ::Vector3(-0.618332f, -0.0877503f, 1.0272f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.708262f, 0.570322f, -0.416051f), 2.63057f)});
        bindPose->SetModelSpaceTransform(r_metacarpal->GetNodeIndex(), {AZ::Vector3(-0.586811f, -0.0484658f, 1.05823f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.717227f, 0.60657f, -0.343014f), 2.4972f)});
        bindPose->SetModelSpaceTransform(r_handProp->GetNodeIndex(), {AZ::Vector3(-0.581079f, -0.070114f, 1.04127f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.537402f, 0.787713f, 0.301177f), 2.41302f)});
        bindPose->SetModelSpaceTransform(l_thumb2->GetNodeIndex(), {AZ::Vector3(0.57507f, -0.13967f, 1.06012f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.84792f, -0.397456f, -0.350802f), 5.75265f)});
        bindPose->SetModelSpaceTransform(l_index2->GetNodeIndex(), {AZ::Vector3(0.642263f, -0.128149f, 0.998859f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.350114f, -0.615319f, -0.70626f), 4.82869f)});
        bindPose->SetModelSpaceTransform(l_mid2->GetNodeIndex(), {AZ::Vector3(0.64395f, -0.0999512f, 0.990563f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.347031f, -0.552723f, -0.757672f), 4.65153f)});
        bindPose->SetModelSpaceTransform(l_ring1->GetNodeIndex(), {AZ::Vector3(0.617197f, -0.0668877f, 1.02221f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.381704f, -0.480076f, -0.789828f), 4.43895f)});
        bindPose->SetModelSpaceTransform(l_pinky1->GetNodeIndex(), {AZ::Vector3(0.614023f, -0.0461531f, 1.02066f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.3959f, -0.478225f, -0.783942f), 4.24768f)});
        bindPose->SetModelSpaceTransform(r_thumb2->GetNodeIndex(), {AZ::Vector3(-0.57507f, -0.13967f, 1.06012f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.989788f, 0.0943283f, -0.106873f), 2.69326f)});
        bindPose->SetModelSpaceTransform(r_index2->GetNodeIndex(), {AZ::Vector3(-0.642263f, -0.12815f, 0.998856f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.768104f, 0.482794f, -0.420627f), 2.67176f)});
        bindPose->SetModelSpaceTransform(r_mid2->GetNodeIndex(), {AZ::Vector3(-0.643951f, -0.0999511f, 0.99056f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.708262f, 0.570322f, -0.41605f), 2.63057f)});
        bindPose->SetModelSpaceTransform(r_ring1->GetNodeIndex(), {AZ::Vector3(-0.617198f, -0.0668878f, 1.02221f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.634185f, 0.660706f, -0.401593f), 2.52345f)});
        bindPose->SetModelSpaceTransform(r_pinky1->GetNodeIndex(), {AZ::Vector3(-0.614024f, -0.0461531f, 1.02066f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.557891f, 0.708492f, -0.432199f), 2.45438f)});
        bindPose->SetModelSpaceTransform(l_thumb3->GetNodeIndex(), {AZ::Vector3(0.578947f, -0.165802f, 1.04687f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.84792f, -0.397456f, -0.350802f), 5.75265f)});
        bindPose->SetModelSpaceTransform(l_index3->GetNodeIndex(), {AZ::Vector3(0.658928f, -0.142847f, 0.974991f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.350114f, -0.615319f, -0.70626f), 4.82869f)});
        bindPose->SetModelSpaceTransform(l_mid3->GetNodeIndex(), {AZ::Vector3(0.661971f, -0.108533f, 0.964789f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.347031f, -0.552723f, -0.757672f), 4.65153f)});
        bindPose->SetModelSpaceTransform(l_ring2->GetNodeIndex(), {AZ::Vector3(0.638427f, -0.0677996f, 0.988058f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.381704f, -0.480076f, -0.789828f), 4.43895f)});
        bindPose->SetModelSpaceTransform(l_pinky2->GetNodeIndex(), {AZ::Vector3(0.628544f, -0.0421721f, 0.990141f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.3959f, -0.478225f, -0.783942f), 4.24768f)});
        bindPose->SetModelSpaceTransform(r_thumb3->GetNodeIndex(), {AZ::Vector3(-0.578947f, -0.165802f, 1.04687f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.989788f, 0.0943283f, -0.106873f), 2.69326f)});
        bindPose->SetModelSpaceTransform(r_index3->GetNodeIndex(), {AZ::Vector3(-0.658929f, -0.142848f, 0.974988f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.768104f, 0.482794f, -0.420627f), 2.67176f)});
        bindPose->SetModelSpaceTransform(r_mid3->GetNodeIndex(), {AZ::Vector3(-0.661971f, -0.108534f, 0.964785f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.708262f, 0.570322f, -0.416051f), 2.63057f)});
        bindPose->SetModelSpaceTransform(r_ring2->GetNodeIndex(), {AZ::Vector3(-0.638427f, -0.0677997f, 0.988056f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.634185f, 0.660706f, -0.401593f), 2.52345f)});
        bindPose->SetModelSpaceTransform(r_pinky2->GetNodeIndex(), {AZ::Vector3(-0.628544f, -0.0421721f, 0.990139f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.557891f, 0.708492f, -0.432199f), 2.45438f)});
        bindPose->SetModelSpaceTransform(l_ring3->GetNodeIndex(), {AZ::Vector3(0.655923f, -0.0685512f, 0.959908f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.381704f, -0.480076f, -0.789828f), 4.43895f)});
        bindPose->SetModelSpaceTransform(l_pinky3->GetNodeIndex(), {AZ::Vector3(0.638685f, -0.0393917f, 0.968823f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.3959f, -0.478225f, -0.783942f), 4.24768f)});
        bindPose->SetModelSpaceTransform(r_ring3->GetNodeIndex(), {AZ::Vector3(-0.655924f, -0.0685512f, 0.959906f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.634185f, 0.660706f, -0.401593f), 2.52345f)});
        bindPose->SetModelSpaceTransform(r_pinky3->GetNodeIndex(), {AZ::Vector3(-0.638685f, -0.0393917f, 0.968821f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(-0.557891f, 0.708492f, -0.432199f), 2.45438f)});
    }
} // namespace EMotionFX
