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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYMOVIE_ANIMAZENTITYNODE_H
#define CRYINCLUDE_CRYMOVIE_ANIMAZENTITYNODE_H

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>

#include <set>

#include "AnimNode.h"

class CAnimComponentNode;

/**
 * CAnimAzEntityNode
 *
 * AZEntities are containers for Components. All the animation is keyed to Components, so the CAnimAzEntityNode
 * only exists to support 'Add Selected Node' functionality in TrackView and to marshall TrackView messages/events
 * to contained components.
*/
class CAnimAzEntityNode
    : public CAnimNode
{
    struct SScriptPropertyParamInfo;
    struct SAnimState;

public:
    AZ_CLASS_ALLOCATOR(CAnimAzEntityNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimAzEntityNode, "{28C02702-3498-488C-BF93-B5FC3FECC9F1}", CAnimNode);

    CAnimAzEntityNode(const int id);
    CAnimAzEntityNode();
    ~CAnimAzEntityNode();

    void         SetAzEntityId(const AZ::EntityId& id) override { m_entityId = id; }
    AZ::EntityId GetAzEntityId() const override { return m_entityId; }

    //////////////////////////////////////////////////////////////////////////
    // Overrides from IAnimNode
    // AzEntityNodes don't have any animatable params - they are all handled by their children components
    // return AnimParamType::Invalid for this pure virtual for the legacy system
    CAnimParamType GetParamType(unsigned int nIndex) const override;

    void SetPos(float time, const Vec3& pos) override;
    void SetRotate(float time, const Quat& quat) override;
    void SetScale(float time, const Vec3& scale) override;

    Vec3 GetOffsetPosition(const Vec3& position) override;

    Vec3 GetPos() override;
    Quat GetRotate() override;
    Quat GetRotate(float time) override;
    Vec3 GetScale() override;
    //////////////////////////////////////////////////////////////////////////

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    // this is an unfortunate hold-over from legacy entities - used when a SceneNode overrides the camera animation so
    // we must disable the transform and camera components from updating animation on this entity because the SceneNode
    // will be animating these components during interpolation.
    void SetSkipInterpolatedCameraNode(const bool skipNodeCameraAnimation) override;

    static void Reflect(AZ::ReflectContext* context);

private:

    // searches children nodes for a component matching the given typeId and returns a pointer to it or nullptr if one is not found
    CAnimComponentNode* GetComponentNodeForComponentWithTypeId(const AZ::Uuid& componentTypeId) const;

    // searches children nodes for a transform component and returns a pointer to it or nullptr if one is not found
    CAnimComponentNode* GetTransformComponentNode() const;

    //! Reference to game entity.
    AZ::EntityId                                m_entityId;
};
#endif // CRYINCLUDE_CRYMOVIE_ANIMAZENTITYNODE_H
