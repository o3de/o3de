/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Atom/RHI.Reflect/RenderAttachmentLayoutBuilder.h>
#include <AzCore/Name/NameDictionary.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RHI;

    class RenderAttachmentLayoutBuilderTests
        : public RHITestFixture
    {
    protected:

        template<class T>
        void ExpectEqMemory(AZStd::span<const T> expected, AZStd::span<const T> actual)
        {
            EXPECT_EQ(expected.size(), actual.size());
            EXPECT_TRUE(memcmp(expected.data(), actual.data(), expected.size() * sizeof(T)) == 0);
        }

        void ExpectEq(AZStd::span<const SubpassRenderAttachmentLayout> expected, AZStd::span<const SubpassRenderAttachmentLayout> actual)
        {
            EXPECT_EQ(expected.size(), actual.size());
            for (int i = 0; i < expected.size() && i < actual.size(); ++i)
            {
                const auto& expectedLayout = expected[i];
                const auto& actualLayout = actual[i];
                ExpectEqMemory<RenderAttachmentDescriptor>(
                    { expectedLayout.m_rendertargetDescriptors.begin(), expectedLayout.m_rendertargetCount },
                    { actualLayout.m_rendertargetDescriptors.begin(), actualLayout.m_rendertargetCount });

                ExpectEqMemory<SubpassInputDescriptor>(
                    { expectedLayout.m_subpassInputDescriptors.begin(), expectedLayout.m_subpassInputCount },
                    { actualLayout.m_subpassInputDescriptors.begin(), actualLayout.m_subpassInputCount });

                ExpectEqMemory<RenderAttachmentDescriptor>({ &expectedLayout.m_depthStencilDescriptor, 1 }, { &actualLayout.m_depthStencilDescriptor, 1 });
            }
        }

        void ExpectEq(const RenderAttachmentLayout& expected, const RenderAttachmentLayout& actual)
        {
            ExpectEqMemory<Format>({ expected.m_attachmentFormats.begin(), expected.m_attachmentCount }, { actual.m_attachmentFormats.begin(), actual.m_attachmentCount });
            ExpectEq({ expected.m_subpassLayouts.begin(), expected.m_subpassCount }, { actual.m_subpassLayouts.begin(), actual.m_subpassCount });
        }
    };

    TEST_F(RenderAttachmentLayoutBuilderTests, TestDefault)
    {
        RenderAttachmentLayout expected;
        RenderAttachmentLayout actual;
        ResultCode result = RenderAttachmentLayoutBuilder().End(actual);
        EXPECT_EQ(result, ResultCode::Success);
        ExpectEq(expected, actual);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestSingleSubpass)
    {
        RenderAttachmentLayout expected;
        {
            expected.m_subpassCount = 1;
            expected.m_attachmentCount = 3;
            expected.m_attachmentFormats[0] = Format::R16_FLOAT;
            expected.m_attachmentFormats[1] = Format::R8G8B8A8_SINT;
            expected.m_attachmentFormats[2] = Format::D16_UNORM;

            auto& subpassLayout = expected.m_subpassLayouts[0];
            subpassLayout.m_rendertargetCount = 2;
            subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 0;
            subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
            subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
            subpassLayout.m_rendertargetDescriptors[1].m_attachmentIndex = 1;
            subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
            subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
            subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = 2;
            subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
            subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::EarlyFragmentTest | AZ::RHI::ScopeAttachmentStage::LateFragmentTest;
        }
        
        RenderAttachmentLayout actual;
        {
            RHI::RenderAttachmentLayoutBuilder layoutBuilder;

            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Format::R16_FLOAT)
                ->RenderTargetAttachment(Format::R8G8B8A8_SINT)
                ->DepthStencilAttachment(Format::D16_UNORM);

            ResultCode result = layoutBuilder.End(actual);
            EXPECT_EQ(result, ResultCode::Success);
        }

        ExpectEq(expected, actual);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestMultipleSubpasses)
    {
        RenderAttachmentLayout expected;
        {            
            expected.m_subpassCount = 2;
            expected.m_attachmentCount = 4;
            expected.m_attachmentFormats[0] = Format::R10G10B10A2_UNORM;
            expected.m_attachmentFormats[1] = Format::R32_FLOAT;
            expected.m_attachmentFormats[2] = Format::R10G10B10A2_UNORM;
            expected.m_attachmentFormats[3] = Format::D24_UNORM_S8_UINT;

            {
                auto& subpassLayout = expected.m_subpassLayouts[0];
                subpassLayout.m_rendertargetCount = 2;
                subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 0;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_rendertargetDescriptors[1].m_attachmentIndex = 1;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
            }
            {
                auto& subpassLayout = expected.m_subpassLayouts[1];
                subpassLayout.m_rendertargetCount = 1;
                subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 2;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = 3;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::EarlyFragmentTest | AZ::RHI::ScopeAttachmentStage::LateFragmentTest;
            }
        }

        RenderAttachmentLayout actual;
        {  
            RHI::RenderAttachmentLayoutBuilder layoutBuilder;
            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Format::R10G10B10A2_UNORM)
                ->RenderTargetAttachment(Format::R32_FLOAT);
            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Format::R10G10B10A2_UNORM)
                ->DepthStencilAttachment(Format::D24_UNORM_S8_UINT);
            ResultCode result = layoutBuilder.End(actual);
            EXPECT_EQ(result, ResultCode::Success);
        }

        ExpectEq(expected, actual);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestSubpassInputs)
    {
        RenderAttachmentLayout expected;
        {
            expected.m_subpassCount = 3;
            expected.m_attachmentCount = 4;
            expected.m_attachmentFormats[0] = Format::R10G10B10A2_UNORM;
            expected.m_attachmentFormats[1] = Format::R32_FLOAT;
            expected.m_attachmentFormats[2] = Format::R10G10B10A2_UNORM;
            expected.m_attachmentFormats[3] = Format::D24_UNORM_S8_UINT;

            {
                auto& subpassLayout = expected.m_subpassLayouts[0];
                subpassLayout.m_rendertargetCount = 2;
                subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 0;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_rendertargetDescriptors[1].m_attachmentIndex = 1;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
            }
            {
                auto& subpassLayout = expected.m_subpassLayouts[1];
                subpassLayout.m_rendertargetCount = 1;
                subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 2;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = 3;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::EarlyFragmentTest | AZ::RHI::ScopeAttachmentStage::LateFragmentTest;
            }
            {
                auto& subpassLayout = expected.m_subpassLayouts[2];
                subpassLayout.m_rendertargetCount = 1;
                subpassLayout.m_subpassInputCount = 2;
                subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 1;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_subpassInputDescriptors[0] = RHI::SubpassInputDescriptor{ 2, ImageAspectFlags::Color };
                subpassLayout.m_subpassInputDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Read;
                subpassLayout.m_subpassInputDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::FragmentShader;
                subpassLayout.m_subpassInputDescriptors[1] = RHI::SubpassInputDescriptor{ 0, ImageAspectFlags::Color };
                subpassLayout.m_subpassInputDescriptors[1].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Read;
                subpassLayout.m_subpassInputDescriptors[1].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::FragmentShader;
            }
        }

        RenderAttachmentLayout actual;
        {
            RHI::RenderAttachmentLayoutBuilder layoutBuilder;
            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Format::R10G10B10A2_UNORM, Name{ "InputAttachment1" })
                ->RenderTargetAttachment(Format::R32_FLOAT, Name{ "RenderTarget0" });
            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Format::R10G10B10A2_UNORM, Name{ "InputAttachment0" })
                ->DepthStencilAttachment(Format::D24_UNORM_S8_UINT);
            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Name{ "RenderTarget0" })
                ->SubpassInputAttachment(Name{ "InputAttachment0" }, ImageAspectFlags::Color)
                ->SubpassInputAttachment(Name{ "InputAttachment1" }, ImageAspectFlags::Color);

            ResultCode result = layoutBuilder.End(actual);
            EXPECT_EQ(result, ResultCode::Success);
        }

        ExpectEq(expected, actual);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestResolveAttachments)
    {
        RenderAttachmentLayout expected;
        {
            expected.m_subpassCount = 1;
            expected.m_attachmentCount = 4;
            expected.m_attachmentFormats[0] = Format::R16_FLOAT;
            expected.m_attachmentFormats[1] = Format::R16_FLOAT;
            expected.m_attachmentFormats[2] = Format::R8G8B8A8_SINT;
            expected.m_attachmentFormats[3] = Format::D16_UNORM;

            auto& subpassLayout = expected.m_subpassLayouts[0];
            subpassLayout.m_rendertargetCount = 2;
            subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 1;
            subpassLayout.m_rendertargetDescriptors[0].m_resolveAttachmentIndex = 0;
            subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
            subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
            subpassLayout.m_rendertargetDescriptors[1].m_attachmentIndex = 2;
            subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
            subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
            subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = 3;
            subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
            subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::EarlyFragmentTest | AZ::RHI::ScopeAttachmentStage::LateFragmentTest;
        }

        RenderAttachmentLayout actual;
        {
            RHI::RenderAttachmentLayoutBuilder layoutBuilder;

            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Format::R16_FLOAT, true)
                ->RenderTargetAttachment(Format::R8G8B8A8_SINT)
                ->DepthStencilAttachment(Format::D16_UNORM);

            ResultCode result = layoutBuilder.End(actual);
            EXPECT_EQ(result, ResultCode::Success);
        }

        ExpectEq(expected, actual);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestRenderTargetByName)
    {
        RenderAttachmentLayout expected;
        {
            expected.m_subpassCount = 2;
            expected.m_attachmentCount = 3;
            expected.m_attachmentFormats[0] = Format::R16_FLOAT;
            expected.m_attachmentFormats[1] = Format::R8G8B8A8_SINT;
            expected.m_attachmentFormats[2] = Format::D16_UNORM;

            {
                auto& subpassLayout = expected.m_subpassLayouts[0];
                subpassLayout.m_rendertargetCount = 2;
                subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 0;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_rendertargetDescriptors[1].m_attachmentIndex = 1;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = 2;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::EarlyFragmentTest | AZ::RHI::ScopeAttachmentStage::LateFragmentTest;
            }
            {
                auto& subpassLayout = expected.m_subpassLayouts[1];
                subpassLayout.m_rendertargetCount = 2;
                subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 0;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_rendertargetDescriptors[1].m_attachmentIndex = 1;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
            }
        }

        RenderAttachmentLayout actual;
        {
            RHI::RenderAttachmentLayoutBuilder layoutBuilder;

            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Format::R16_FLOAT, Name{ "RenderTaret0" })
                ->RenderTargetAttachment(Format::R8G8B8A8_SINT, Name{ "RenderTaret1" })
                ->DepthStencilAttachment(Format::D16_UNORM);
            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Name{ "RenderTaret0" })
                ->RenderTargetAttachment(Name{ "RenderTaret1" });

            ResultCode result = layoutBuilder.End(actual);
            EXPECT_EQ(result, ResultCode::Success);
        }

        ExpectEq(expected, actual);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestDepthStencil)
    {
        RenderAttachmentLayout expected;
        {
            expected.m_subpassCount = 2;
            expected.m_attachmentCount = 3;
            expected.m_attachmentFormats[0] = Format::R16_FLOAT;
            expected.m_attachmentFormats[1] = Format::R8G8B8A8_SINT;
            expected.m_attachmentFormats[2] = Format::D16_UNORM;

            {
                auto& subpassLayout = expected.m_subpassLayouts[0];
                subpassLayout.m_rendertargetCount = 2;
                subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 0;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_rendertargetDescriptors[1].m_attachmentIndex = 1;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = 2;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::EarlyFragmentTest | AZ::RHI::ScopeAttachmentStage::LateFragmentTest;
            }
            {
                auto& subpassLayout = expected.m_subpassLayouts[1];
                subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = 2;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::EarlyFragmentTest | AZ::RHI::ScopeAttachmentStage::LateFragmentTest;
            }
        }

        RenderAttachmentLayout actual;
        {
            RHI::RenderAttachmentLayoutBuilder layoutBuilder;

            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Format::R16_FLOAT)
                ->RenderTargetAttachment(Format::R8G8B8A8_SINT)
                ->DepthStencilAttachment(Format::D16_UNORM);
            layoutBuilder.AddSubpass()
                ->DepthStencilAttachment();

            ResultCode result = layoutBuilder.End(actual);
            EXPECT_EQ(result, ResultCode::Success);
        }

        ExpectEq(expected, actual);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestResolveByName)
    {
        RenderAttachmentLayout expected;
        {
            expected.m_subpassCount = 1;
            expected.m_attachmentCount = 4;
            expected.m_attachmentFormats[0] = Format::R16_FLOAT;
            expected.m_attachmentFormats[1] = Format::R16_FLOAT;
            expected.m_attachmentFormats[2] = Format::R8G8B8A8_SINT;
            expected.m_attachmentFormats[3] = Format::D16_UNORM;

            auto& subpassLayout = expected.m_subpassLayouts[0];
            subpassLayout.m_rendertargetCount = 2;
            subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 1;
            subpassLayout.m_rendertargetDescriptors[0].m_resolveAttachmentIndex = 0;
            subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
            subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
            subpassLayout.m_rendertargetDescriptors[1].m_attachmentIndex = 2;
            subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
            subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
            subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = 3;
            subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
            subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::EarlyFragmentTest | AZ::RHI::ScopeAttachmentStage::LateFragmentTest;
        }

        RenderAttachmentLayout actual;
        {
            RHI::RenderAttachmentLayoutBuilder layoutBuilder;

            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Format::R16_FLOAT, Name{ "RenderTarget0" })
                ->RenderTargetAttachment(Format::R8G8B8A8_SINT)
                ->DepthStencilAttachment(Format::D16_UNORM)
                ->ResolveAttachment(Name{ "RenderTarget0" });

            ResultCode result = layoutBuilder.End(actual);
            EXPECT_EQ(result, ResultCode::Success);
        }

        ExpectEq(expected, actual);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestResolveAsSubpassInput)
    {
        RenderAttachmentLayout expected;
        {
            expected.m_subpassCount = 2;
            expected.m_attachmentCount = 4;
            expected.m_attachmentFormats[0] = Format::R10G10B10A2_UNORM;
            expected.m_attachmentFormats[1] = Format::R10G10B10A2_UNORM;
            expected.m_attachmentFormats[2] = Format::R32_FLOAT;
            expected.m_attachmentFormats[3] = Format::D24_UNORM_S8_UINT;

            {
                auto& subpassLayout = expected.m_subpassLayouts[0];
                subpassLayout.m_rendertargetCount = 2;
                subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 1;
                subpassLayout.m_rendertargetDescriptors[0].m_resolveAttachmentIndex = 0;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_rendertargetDescriptors[1].m_attachmentIndex = 2;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
            }
            {
                auto& subpassLayout = expected.m_subpassLayouts[1];
                subpassLayout.m_subpassInputCount = 1;
                subpassLayout.m_subpassInputDescriptors[0] = RHI::SubpassInputDescriptor{ 0, ImageAspectFlags::Color };
                subpassLayout.m_subpassInputDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Read;
                subpassLayout.m_subpassInputDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::FragmentShader;
                subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = 3;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::EarlyFragmentTest | AZ::RHI::ScopeAttachmentStage::LateFragmentTest;
            }
        }

        RenderAttachmentLayout actual;
        {
            RHI::RenderAttachmentLayoutBuilder layoutBuilder;
            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Format::R10G10B10A2_UNORM, Name{ "ColorAttachment0" })
                ->RenderTargetAttachment(Format::R32_FLOAT)
                ->ResolveAttachment(Name{ "ColorAttachment0" }, Name{ "Resolve0" });
            layoutBuilder.AddSubpass()
                ->SubpassInputAttachment(Name{ "Resolve0" }, ImageAspectFlags::Color)
                ->DepthStencilAttachment(Format::D24_UNORM_S8_UINT);

            ResultCode result = layoutBuilder.End(actual);
            EXPECT_EQ(result, ResultCode::Success);
        }

        ExpectEq(expected, actual);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestAttachmentLoadStoreAction)
    {
        RenderAttachmentLayout expected;
        AttachmentLoadStoreAction renderTargetLoadStoreAction;
        renderTargetLoadStoreAction.m_storeAction = RHI::AttachmentStoreAction::DontCare;
        AttachmentLoadStoreAction depthStencilLoadStoreAction;
        {
            expected.m_subpassCount = 1;
            expected.m_attachmentCount = 3;
            expected.m_attachmentFormats[0] = Format::R10G10B10A2_UNORM;
            expected.m_attachmentFormats[1] = Format::R32_FLOAT;
            expected.m_attachmentFormats[2] = Format::D24_UNORM_S8_UINT;

            {
                auto& subpassLayout = expected.m_subpassLayouts[0];
                subpassLayout.m_rendertargetCount = 2;
                subpassLayout.m_rendertargetDescriptors[0].m_attachmentIndex = 0;
                subpassLayout.m_rendertargetDescriptors[0].m_loadStoreAction = renderTargetLoadStoreAction;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[0].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_rendertargetDescriptors[1].m_attachmentIndex = 1;
                subpassLayout.m_rendertargetDescriptors[1].m_loadStoreAction = depthStencilLoadStoreAction;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_rendertargetDescriptors[1].m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::ColorAttachmentOutput;
                subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = 2;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Write;
                subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::EarlyFragmentTest | AZ::RHI::ScopeAttachmentStage::LateFragmentTest;
            }
        }

        RenderAttachmentLayout actual;
        {
            RHI::RenderAttachmentLayoutBuilder layoutBuilder;

            depthStencilLoadStoreAction.m_storeActionStencil = RHI::AttachmentStoreAction::Store;
            layoutBuilder.AddSubpass()
                ->RenderTargetAttachment(Format::R10G10B10A2_UNORM, Name{"RenderTarget0"}, renderTargetLoadStoreAction)
                ->RenderTargetAttachment(Format::R32_FLOAT)
                ->DepthStencilAttachment(Format::D24_UNORM_S8_UINT, Name{ "DepthStencil" }, depthStencilLoadStoreAction);

            ResultCode result = layoutBuilder.End(actual);
            EXPECT_EQ(result, ResultCode::Success);
        }

        ExpectEq(expected, actual);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestInvalidRenderTargetFormat)
    {
        RenderAttachmentLayout actual;
        RHI::RenderAttachmentLayoutBuilder layoutBuilder;

        layoutBuilder.AddSubpass()
            ->RenderTargetAttachment(Format::Unknown)
            ->RenderTargetAttachment(Format::R32_FLOAT)
            ->DepthStencilAttachment(Format::D24_UNORM_S8_UINT);

        AZ_TEST_START_TRACE_SUPPRESSION;
        ResultCode result = layoutBuilder.End(actual);
        EXPECT_EQ(result, ResultCode::InvalidArgument);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestInvalidRenderTargetFormatByReference)
    {
        RenderAttachmentLayout actual;
        RHI::RenderAttachmentLayoutBuilder layoutBuilder;

        layoutBuilder.AddSubpass()
            ->RenderTargetAttachment(Format::R10G10B10A2_UNORM, Name{ "RenderAttachment0" })
            ->RenderTargetAttachment(Format::R32_FLOAT)
            ->DepthStencilAttachment(Format::D24_UNORM_S8_UINT);
        layoutBuilder.AddSubpass()
            ->RenderTargetAttachment(Format::R32_FLOAT, Name{ "RenderAttachment0" });

        AZ_TEST_START_TRACE_SUPPRESSION;
        ResultCode result = layoutBuilder.End(actual);
        EXPECT_EQ(result, ResultCode::InvalidArgument);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestInvalidRenderTargetName)
    {
        RenderAttachmentLayout actual;
        RHI::RenderAttachmentLayoutBuilder layoutBuilder;

        layoutBuilder.AddSubpass()
            ->RenderTargetAttachment(Name{ "RenderAttachment0" })
            ->RenderTargetAttachment(Format::R32_FLOAT)
            ->DepthStencilAttachment(Format::D24_UNORM_S8_UINT);

        AZ_TEST_START_TRACE_SUPPRESSION;
        ResultCode result = layoutBuilder.End(actual);
        EXPECT_EQ(result, ResultCode::InvalidArgument);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestInvalidDepthStencilFormat)
    {
        RenderAttachmentLayout actual;
        RHI::RenderAttachmentLayoutBuilder layoutBuilder;

        layoutBuilder.AddSubpass()
            ->RenderTargetAttachment(Format::R10G10B10A2_UNORM)
            ->RenderTargetAttachment(Format::R32_FLOAT)
            ->DepthStencilAttachment(Format::D24_UNORM_S8_UINT);
        layoutBuilder.AddSubpass()
            ->DepthStencilAttachment(Format::D32_FLOAT);

        AZ_TEST_START_TRACE_SUPPRESSION;
        ResultCode result = layoutBuilder.End(actual);
        EXPECT_EQ(result, ResultCode::InvalidArgument);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestInvalidDepthStencilName)
    {
        RenderAttachmentLayout actual;
        RHI::RenderAttachmentLayoutBuilder layoutBuilder;

        layoutBuilder.AddSubpass()
            ->RenderTargetAttachment(Format::R10G10B10A2_UNORM)
            ->RenderTargetAttachment(Format::R32_FLOAT)
            ->DepthStencilAttachment(Format::D24_UNORM_S8_UINT, Name{ "DepthStencil" });
        layoutBuilder.AddSubpass()
            ->DepthStencilAttachment(Name{ "InvalidDepthStencilName" });

        AZ_TEST_START_TRACE_SUPPRESSION;
        ResultCode result = layoutBuilder.End(actual);
        EXPECT_EQ(result, ResultCode::InvalidArgument);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }


    TEST_F(RenderAttachmentLayoutBuilderTests, TestNotDefinedDepthStencilFormat)
    {
        RenderAttachmentLayout actual;
        RHI::RenderAttachmentLayoutBuilder layoutBuilder;

        layoutBuilder.AddSubpass()
            ->RenderTargetAttachment(Format::R10G10B10A2_UNORM)
            ->RenderTargetAttachment(Format::R32_FLOAT)
            ->DepthStencilAttachment();

        AZ_TEST_START_TRACE_SUPPRESSION;
        ResultCode result = layoutBuilder.End(actual);
        EXPECT_EQ(result, ResultCode::InvalidArgument);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestInvalidResolve)
    {
        RenderAttachmentLayout actual;
        RHI::RenderAttachmentLayoutBuilder layoutBuilder;

        AZ_TEST_START_TRACE_SUPPRESSION;
        layoutBuilder.AddSubpass()
            ->RenderTargetAttachment(Format::R10G10B10A2_UNORM)
            ->RenderTargetAttachment(Format::R32_FLOAT)
            ->ResolveAttachment(Name{ "InvalidAttachment" });
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(RenderAttachmentLayoutBuilderTests, TestInvalidSubpassInput)
    {
        RenderAttachmentLayout actual;
        RHI::RenderAttachmentLayoutBuilder layoutBuilder;

        layoutBuilder.AddSubpass()
            ->RenderTargetAttachment(Format::R10G10B10A2_UNORM)
            ->RenderTargetAttachment(Format::R32_FLOAT);
        layoutBuilder.AddSubpass()->SubpassInputAttachment(Name{ "InvalidSubpassInput" }, ImageAspectFlags::Color);

        AZ_TEST_START_TRACE_SUPPRESSION;
        ResultCode result = layoutBuilder.End(actual);
        EXPECT_EQ(result, ResultCode::InvalidArgument);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }
}
