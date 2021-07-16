#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IAnimationGroup
                : public IGroup
            {
            public:
                AZ_RTTI(IAnimationGroup, "{B07FB380-6A1F-40BE-B3CE-41258985DF36}", IGroup);

                struct PerBoneCompression
                {
                public:

                    AZ_TYPE_INFO(PerBoneCompression, "{23DC875D-42E8-40AF-AB0E-30BDBB6427D8}");

                    AZStd::string m_boneNamePattern = "*";
                    float m_compressionStrength = 0.1f;

                    static void Reflect(ReflectContext* context);
                };
                typedef AZStd::vector<PerBoneCompression> PerBoneCompressionList;

                ~IAnimationGroup() override = default;

                virtual const AZStd::string& GetSelectedRootBone() const = 0;
                virtual uint32_t GetStartFrame() const = 0;
                virtual uint32_t GetEndFrame() const = 0;

                virtual void SetSelectedRootBone(const AZStd::string& selectedRootBone) = 0;
                virtual void SetStartFrame(uint32_t frame) = 0;
                virtual void SetEndFrame(uint32_t frame) = 0;

                virtual const float GetDefaultCompressionStrength() const = 0;
                virtual const PerBoneCompressionList& GetPerBoneCompression() const = 0;
            };
        }
    }
}
