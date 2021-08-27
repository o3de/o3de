/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class EMFX_API BlendTreeParameterNode
        : public AnimGraphNode
        , public ObjectAffectedByParameterChanges
    {
    public:
        AZ_RTTI(BlendTreeParameterNode, "{4510529A-323F-40F6-B773-9FA8FC4DE53D}", AnimGraphNode, ObjectAffectedByParameterChanges)
    };
} // namespace EMotionFX
