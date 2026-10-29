/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/unordered_map.h>
#include <SceneColorView/Configuration.h>

namespace SceneColorView
{
    class SceneColorViewFeatureProcessor final : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(SceneColorViewFeatureProcessor, "{7E00B56E-AC46-46AF-95C9-862DCE4F635A}", AZ::RPI::FeatureProcessor);
        AZ_CLASS_ALLOCATOR(SceneColorViewFeatureProcessor, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);
        void Activate() override;
        void Deactivate() override;
        void AddRenderPasses(AZ::RPI::RenderPipeline* pipeline) override;
        void OnEndCulling(const RenderPacket& packet) override;

        bool SetConfiguration(AZ::EntityId owner, const Configuration& configuration);
        void ReleaseConfiguration(AZ::EntityId owner);
        const Configuration& GetConfiguration() const { return m_configuration; }

        struct CaptureRecord
        {
            AZ::Data::Instance<AZ::RPI::AttachmentImage> m_image;
            AZ::RPI::Ptr<AZ::RPI::PassAttachment> m_attachment;
        };
        const CaptureRecord* FindCapture(const AZ::RPI::View* view) const;
        void PublishCapture(const AZ::RPI::View* view, CaptureRecord record);

    private:
        Configuration m_configuration;
        AZ::EntityId m_owner;
        AZStd::unordered_map<const AZ::RPI::View*, CaptureRecord> m_captures;
        AZStd::vector<AZ::RPI::Ptr<AZ::RPI::Pass>> m_passes;
        bool m_reportedMissingLayout = false;
    };
}
