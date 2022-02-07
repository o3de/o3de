/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/CopyPass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>

#include <Atom/RPI.Public/RenderPipeline.h>

#include <AzCore/UnitTest/TestTypes.h>

#include <Common/RPITestFixture.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    // This class holds and sets up some data for the tests
    // This is it's own class so we can delete it before the teardown phase, otherwise
    // name dictionary fails because we have remaining names in the PassTemplates
    struct PassTestData
    {
        AZ_CLASS_ALLOCATOR(PassTestData, SystemAllocator, 0);

        PassTemplate m_parentPass;

        // Child passes
        PassTemplate m_depthPrePass;
        PassTemplate m_lightCullPass;
        PassTemplate m_forwardPass;
        PassTemplate m_postProcessPass;
        static const size_t NumberOfChildPasses = 4;

        void CreatePassTemplates()
        {
            // Depth Pre Pass Dummy...
            {
                m_depthPrePass.m_name = "DepthPrePass";
                m_depthPrePass.m_passClass = "Pass";

                {
                    PassSlot slot;
                    slot.m_name = "DepthOutput";
                    slot.m_slotType = RPI::PassSlotType::Output;
                    slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::RenderTarget;
                    m_depthPrePass.AddSlot(slot);
                }
                {
                    PassImageAttachmentDesc depthBuffer;
                    depthBuffer.m_name = "DepthBuffer";
                    depthBuffer.m_imageDescriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::DepthStencil | RHI::ImageBindFlags::ShaderReadWrite, 1600, 900, RHI::Format::D32_FLOAT_S8X24_UINT);
                    depthBuffer.m_lifetime = RHI::AttachmentLifetimeType::Transient;
                    m_depthPrePass.AddImageAttachment(depthBuffer);
                }
                {
                    PassConnection connection;
                    connection.m_localSlot = "DepthOutput";
                    connection.m_attachmentRef.m_pass = "This";
                    connection.m_attachmentRef.m_attachment = "DepthBuffer";
                    m_depthPrePass.AddOutputConnection(connection);
                }
            }
            // Light Culling Pass Dummy...
            {
                m_lightCullPass.m_name = "LightCullPass";
                m_lightCullPass.m_passClass = "Pass";

                {
                    PassSlot slot;
                    slot.m_name = "LightListOutput";
                    slot.m_slotType = RPI::PassSlotType::Output;
                    slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
                    m_lightCullPass.AddSlot(slot);
                }
                {
                    PassBufferAttachmentDesc lightListBuffer;
                    lightListBuffer.m_name = "LightList";
                    lightListBuffer.m_lifetime = RHI::AttachmentLifetimeType::Transient;
                    m_lightCullPass.AddBufferAttachment(lightListBuffer);
                }
                {
                    PassConnection connection;
                    connection.m_localSlot = "LightListOutput";
                    connection.m_attachmentRef.m_pass = "This";
                    connection.m_attachmentRef.m_attachment = "LightList";
                    m_lightCullPass.AddOutputConnection(connection);
                }
            }
            // Forward Pass Dummy...
            {
                m_forwardPass.m_name = "ForwardPass";
                m_forwardPass.m_passClass = "Pass";

                {
                    PassSlot slot;
                    slot.m_name = "DepthInputOutput";
                    slot.m_slotType = RPI::PassSlotType::InputOutput;
                    slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::DepthStencil;
                    slot.m_formatFilter.push_back(RHI::Format::D24_UNORM_S8_UINT);
                    slot.m_formatFilter.push_back(RHI::Format::D24_UNORM_S8_UINT);
                    slot.m_formatFilter.push_back(RHI::Format::D32_FLOAT_S8X24_UINT);
                    m_forwardPass.AddSlot(slot);
                }
                {
                    PassSlot slot;
                    slot.m_name = "LightListInput";
                    slot.m_slotType = RPI::PassSlotType::Input;
                    m_forwardPass.AddSlot(slot);
                }
                {
                    PassSlot slot;
                    slot.m_name = "LightingOutput";
                    slot.m_slotType = RPI::PassSlotType::Output;
                    slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::RenderTarget;
                    m_forwardPass.AddSlot(slot);
                }
                {
                    PassImageAttachmentDesc lightingOutput;
                    lightingOutput.m_name = "LightingBuffer";
                    lightingOutput.m_imageDescriptor.m_format = RHI::Format::R16G16B16A16_FLOAT;
                    lightingOutput.m_lifetime = RHI::AttachmentLifetimeType::Transient;
                    lightingOutput.m_sizeSource.m_source.m_pass = "This";
                    lightingOutput.m_sizeSource.m_source.m_attachment = "DepthInputOutput";
                    lightingOutput.m_sizeSource.m_multipliers.m_widthMultiplier = 1.0f;
                    lightingOutput.m_sizeSource.m_multipliers.m_heightMultiplier = 1.0f;
                    m_forwardPass.AddImageAttachment(lightingOutput);
                }
                {
                    PassConnection connection;
                    connection.m_localSlot = "LightingOutput";
                    connection.m_attachmentRef.m_pass = "This";
                    connection.m_attachmentRef.m_attachment = "LightingBuffer";
                    m_forwardPass.AddOutputConnection(connection);
                }
            }
            // Post Process Pass Dummy...
            {
                m_postProcessPass.m_name = "PostProcessPass";
                m_postProcessPass.m_passClass = "Pass";

                {
                    PassSlot slot;
                    slot.m_name = "DepthInput";
                    slot.m_slotType = RPI::PassSlotType::Input;
                    slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
                    slot.m_formatFilter.push_back(RHI::Format::D24_UNORM_S8_UINT);
                    slot.m_formatFilter.push_back(RHI::Format::D24_UNORM_S8_UINT);
                    slot.m_formatFilter.push_back(RHI::Format::D32_FLOAT_S8X24_UINT);
                    m_postProcessPass.AddSlot(slot);
                }
                {
                    PassSlot slot;
                    slot.m_name = "LightingInput";
                    slot.m_slotType = RPI::PassSlotType::Input;
                    slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
                    slot.m_formatFilter.push_back(RHI::Format::R16G16B16A16_FLOAT);
                    m_postProcessPass.AddSlot(slot);
                }
                {
                    PassSlot slot;
                    slot.m_name = "FinalOutput";
                    slot.m_slotType = RPI::PassSlotType::Output;
                    slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
                    m_postProcessPass.AddSlot(slot);
                }
                {
                    PassImageAttachmentDesc finalImage;
                    finalImage.m_name = "FinalImage";
                    finalImage.m_lifetime = RHI::AttachmentLifetimeType::Transient;
                    finalImage.m_formatSource.m_pass = "This";
                    finalImage.m_formatSource.m_attachment = "LightingInput";
                    finalImage.m_sizeSource.m_source.m_pass = "This";
                    finalImage.m_sizeSource.m_source.m_attachment = "LightingInput";
                    finalImage.m_sizeSource.m_multipliers.m_widthMultiplier = 0.5f;
                    finalImage.m_sizeSource.m_multipliers.m_heightMultiplier = 0.5f;
                    m_postProcessPass.AddImageAttachment(finalImage);
                }
                {
                    PassConnection connection;
                    connection.m_localSlot = "FinalOutput";
                    connection.m_attachmentRef.m_pass = "This";
                    connection.m_attachmentRef.m_attachment = "FinalImage";
                    m_postProcessPass.AddOutputConnection(connection);
                }
            }
            // Parent Pass Dummy...
            {
                m_parentPass.m_name = "ParentPass";
                m_parentPass.m_passClass = "ParentPass";

                {
                    PassSlot slot;
                    slot.m_name = "Output";
                    slot.m_slotType = RPI::PassSlotType::Output;
                    m_parentPass.AddSlot(slot);
                }
                {
                    PassRequest request;
                    request.m_passName = "DepthPrePass";
                    request.m_templateName = "DepthPrePass";
                    m_parentPass.AddPassRequest(request);
                }
                {
                    PassRequest request;
                    request.m_passName = "LightCullPass";
                    request.m_templateName = "LightCullPass";
                    m_parentPass.AddPassRequest(request);
                }
                {
                    PassRequest request;
                    request.m_passName = "ForwardPass";
                    request.m_templateName = "ForwardPass";

                    PassConnection connection;
                    connection.m_localSlot = "DepthInputOutput";
                    connection.m_attachmentRef.m_pass = "DepthPrePass";
                    connection.m_attachmentRef.m_attachment = "DepthOutput";
                    request.AddInputConnection(connection);

                    connection.m_localSlot = "LightListInput";
                    connection.m_attachmentRef.m_pass = "LightCullPass";
                    connection.m_attachmentRef.m_attachment = "LightListOutput";
                    request.AddInputConnection(connection);

                    m_parentPass.AddPassRequest(request);
                }
                {
                    PassRequest request;
                    request.m_passName = "PostProcessPass";
                    request.m_templateName = "PostProcessPass";

                    PassConnection connection;
                    connection.m_localSlot = "DepthInput";
                    connection.m_attachmentRef.m_pass = "ForwardPass";
                    connection.m_attachmentRef.m_attachment = "DepthInputOutput";
                    request.AddInputConnection(connection);

                    connection.m_localSlot = "LightingInput";
                    connection.m_attachmentRef.m_pass = "ForwardPass";
                    connection.m_attachmentRef.m_attachment = "LightingOutput";
                    request.AddInputConnection(connection);

                    m_parentPass.AddPassRequest(request);
                }
                {
                    PassConnection connection;
                    connection.m_localSlot = "Output";
                    connection.m_attachmentRef.m_pass = "PostProcessPass";
                    connection.m_attachmentRef.m_attachment = "FinalOutput";
                    m_parentPass.AddOutputConnection(connection);
                }
            }
        }

        void AddPassTemplatesToLibrary()
        {
            PassSystemInterface* passSystem = PassSystemInterface::Get();
            passSystem->AddPassTemplate(Name("DepthPrePass"), m_depthPrePass.Clone());
            passSystem->AddPassTemplate(Name("LightCullPass"), m_lightCullPass.Clone());
            passSystem->AddPassTemplate(Name("ForwardPass"), m_forwardPass.Clone());
            passSystem->AddPassTemplate(Name("PostProcessPass"), m_postProcessPass.Clone());
            passSystem->AddPassTemplate(Name("ParentPass"), m_parentPass.Clone());
        }
    };

    class PassTests
        : public RPITestFixture
    {
    protected:

        static Ptr<Pass> Create(const PassDescriptor& descriptor)
        {
            Ptr<Pass> pass = aznew Pass(descriptor);
            return pass;
        }

        // Setup functions...

        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_passSystem = azrtti_cast<PassSystem*>(PassSystemInterface::Get());

            m_data = aznew PassTestData();

            // We don't ever instantiate the base Pass class in the runtime, however we use it here to facilitate testing
            m_passSystem->AddPassCreator(Name("Pass"), &PassTests::Create);

            m_data->CreatePassTemplates();
        }

        void TearDown() override
        {
            // Delete PassTemplates so Names are released
            delete m_data;

            RPITestFixture::TearDown();
        }

        // Members...

        PassSystem* m_passSystem;
        PassTestData* m_data;

        // Tests...

        // Tests that we can build the passes outlined in PassTestData and that the validation of the pass hierarchy returns no errors
        void TestPassConstructionAndValidation()
        {
            m_data->AddPassTemplatesToLibrary();

            Ptr<Pass> parentPass = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("ParentPass"));
            parentPass->Reset();
            parentPass->Build();

            PassValidationResults validationResults;
            parentPass->Validate(validationResults);

            EXPECT_TRUE(validationResults.IsValid());
            EXPECT_TRUE(parentPass->m_name == Name("ParentPass"));
            EXPECT_TRUE(parentPass->AsParent()->GetChildren().size() == PassTestData::NumberOfChildPasses);
        }

        // Tests that validation correctly fails when a connected slot does not match any of the format filters
        void TestFormatFilterFailure()
        {
            AZ_TEST_START_TRACE_SUPPRESSION;

            // Set the format filter to block the connected attachment
            m_data->m_postProcessPass.m_slots[1].m_formatFilter.clear();
            m_data->m_postProcessPass.m_slots[1].m_formatFilter.push_back(RHI::Format::R8G8B8A8_SNORM);
            m_data->AddPassTemplatesToLibrary();

            Ptr<Pass> parentPass = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("ParentPass"));
            parentPass->Reset();
            parentPass->Build();

            PassValidationResults validationResults;
            parentPass->Validate(validationResults);

            EXPECT_FALSE(validationResults.IsValid());
            EXPECT_EQ(1, validationResults.m_passesWithErrors.size());

            AZ_TEST_STOP_TRACE_SUPPRESSION(2);
        }

        // Tests that validation correctly fails when connection's local slot name is set to a garbage value
        void TestInvalidLocalSlotName()
        {
            AZ_TEST_START_TRACE_SUPPRESSION;

            // Set the connection's local slot name to a garbage value
            m_data->m_parentPass.m_passRequests[3].m_connections[1].m_localSlot = "NonExistantName";
            m_data->AddPassTemplatesToLibrary();

            Ptr<Pass> parentPass = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("ParentPass"));
            parentPass->Reset();
            parentPass->Build();

            PassValidationResults validationResults;
            parentPass->Validate(validationResults);

            EXPECT_FALSE(validationResults.IsValid());
            EXPECT_EQ(1, validationResults.m_passesWithErrors.size());

            AZ_TEST_STOP_TRACE_SUPPRESSION(2);
        }

        // Tests that validation correctly fails when connection's target slot name is set to a garbage value
        void TestInvalidConnectedSlotName()
        {
            AZ_TEST_START_TRACE_SUPPRESSION;

            // Set the connection's target slot name to a garbage value
            m_data->m_parentPass.m_passRequests[3].m_connections[1].m_attachmentRef.m_attachment = "NonExistantName";
            m_data->AddPassTemplatesToLibrary();

            Ptr<Pass> parentPass = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("ParentPass"));
            parentPass->m_flags.m_partOfHierarchy = true;
            parentPass->OnHierarchyChange();
            parentPass->Reset();
            parentPass->Build();

            PassValidationResults validationResults;
            parentPass->Validate(validationResults);

            EXPECT_FALSE(validationResults.IsValid());
            EXPECT_EQ(1, validationResults.m_passesWithErrors.size());

            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Tests that validation correctly fails when connection's target slot name is set to a garbage value
        void TestInvalidConnectedPassName()
        {
            AZ_TEST_START_TRACE_SUPPRESSION;

            // Set the connection's target pass name to a garbage value
            m_data->m_parentPass.m_passRequests[3].m_connections[1].m_attachmentRef.m_pass = "NonExistantName";
            m_data->AddPassTemplatesToLibrary();

            Ptr<Pass> parentPass = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("ParentPass"));
            parentPass->m_flags.m_partOfHierarchy = true;
            parentPass->OnHierarchyChange();
            parentPass->Reset();
            parentPass->Build();

            PassValidationResults validationResults;
            parentPass->Validate(validationResults);

            EXPECT_FALSE(validationResults.IsValid());
            EXPECT_EQ(1, validationResults.m_passesWithErrors.size());

            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Tests that validation correctly fails when connection's slots are mismatched
        void TestSlotTypeMismatch()
        {
            AZ_TEST_START_TRACE_SUPPRESSION;

            // Set one of the inputs to be connected to another input, which is invalid
            m_data->m_parentPass.m_passRequests[3].m_connections[1].m_attachmentRef.m_attachment = "LightListInput";
            m_data->AddPassTemplatesToLibrary();

            Ptr<Pass> parentPass = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("ParentPass"));
            parentPass->m_flags.m_partOfHierarchy = true;
            parentPass->OnHierarchyChange();
            parentPass->Reset();
            parentPass->Build();

            PassValidationResults validationResults;
            parentPass->Validate(validationResults);

            EXPECT_FALSE(validationResults.IsValid());
            EXPECT_EQ(1, validationResults.m_passesWithErrors.size());

            AZ_TEST_STOP_TRACE_SUPPRESSION(2);
        }

        // Tests that validation correctly fails when parent-child connection's slots are mismatched (mismatches are different for parent to child connections)
        void TestParentChildSlotTypeMismatch()
        {
            AZ_TEST_START_TRACE_SUPPRESSION;

            // Set parent output to be connect to child input, which is invalid
            m_data->m_parentPass.m_connections[0].m_attachmentRef.m_attachment = "LightingInput";
            m_data->AddPassTemplatesToLibrary();

            Ptr<Pass> parentPass = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("ParentPass"));
            parentPass->m_flags.m_partOfHierarchy = true;
            parentPass->OnHierarchyChange();
            parentPass->Reset();
            parentPass->Build();

            PassValidationResults validationResults;
            parentPass->Validate(validationResults);

            EXPECT_FALSE(validationResults.IsValid());
            EXPECT_EQ(1, validationResults.m_passesWithErrors.size());

            AZ_TEST_STOP_TRACE_SUPPRESSION(2);
        }
    };

    TEST_F(PassTests, ConstructionAndValidation)
    {
        TestPassConstructionAndValidation();
    }

    TEST_F(PassTests, FormatFilterFailure)
    {
        TestFormatFilterFailure();
    }

    TEST_F(PassTests, InvalidLocalSlotName)
    {
        TestInvalidLocalSlotName();
    }

    TEST_F(PassTests, InvalidConnectedSlotName)
    {
        TestInvalidConnectedSlotName();
    }

    TEST_F(PassTests, InvalidConnectedPassName)
    {
        TestInvalidConnectedPassName();
    }

    TEST_F(PassTests, SlotTypeMismatch)
    {
        TestSlotTypeMismatch();
    }

    TEST_F(PassTests, ParentChildSlotTypeMismatch)
    {
        TestParentChildSlotTypeMismatch();
    }

    // Checks that pass factory has all the default name
    TEST_F(PassTests, FactoryDefaultCreators)
    {
        EXPECT_TRUE(m_passSystem->HasCreatorForClass(Name("ParentPass"))             );
        EXPECT_TRUE(m_passSystem->HasCreatorForClass(Name("RasterPass"))             );
        EXPECT_TRUE(m_passSystem->HasCreatorForClass(Name("CopyPass"))               );
        EXPECT_TRUE(m_passSystem->HasCreatorForClass(Name("FullScreenTriangle"))     );
        EXPECT_TRUE(m_passSystem->HasCreatorForClass(Name("ComputePass"))            );
        EXPECT_TRUE(m_passSystem->HasCreatorForClass(Name("MSAAResolvePass"))        );
        EXPECT_TRUE(m_passSystem->HasCreatorForClass(Name("DownsampleMipChainPass")) );
    }

    // Tests that all creation methods return null with invalid arguments
    TEST_F(PassTests, CreationMethodsFailure)
    {
        Name doesNotExist("doesNotExist");
        Ptr<Pass> pass;
        AZStd::shared_ptr<PassTemplate> nullTemplate = nullptr;

        AZ_TEST_START_TRACE_SUPPRESSION;

        pass = m_passSystem->CreatePassFromClass(doesNotExist, doesNotExist);
        EXPECT_TRUE(pass == nullptr);

        pass = m_passSystem->CreatePassFromTemplate(doesNotExist, doesNotExist);
        EXPECT_TRUE(pass == nullptr);

        pass = m_passSystem->CreatePassFromTemplate(nullTemplate, doesNotExist);
        EXPECT_TRUE(pass == nullptr);

        pass = m_passSystem->CreatePassFromRequest(nullptr);
        EXPECT_TRUE(pass == nullptr);

        AZ_TEST_STOP_TRACE_SUPPRESSION(4);
    }

    // Tests that all creation methods successfully create passes with valid arguments
    TEST_F(PassTests, CreationMethodsSuccess)
    {
        m_data->AddPassTemplatesToLibrary();

        Ptr<Pass> pass = m_passSystem->CreatePassFromClass(Name("Pass"), Name("Test01"));
        EXPECT_TRUE(pass != nullptr);

        pass = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("Test02"));
        EXPECT_TRUE(pass != nullptr);

        AZStd::shared_ptr<PassTemplate> parentPassTemplate = m_data->m_parentPass.Clone();
        pass = m_passSystem->CreatePassFromTemplate(parentPassTemplate, Name("Test03"));
        EXPECT_TRUE(pass != nullptr);

        PassRequest request;
        request.m_passName = "Test04";
        request.m_templateName = "ParentPass";
        pass = m_passSystem->CreatePassFromRequest(&request);
        EXPECT_TRUE(pass != nullptr);

        pass = m_passSystem->CreatePass<CopyPass>(Name("Test05"));
        EXPECT_TRUE(pass != nullptr);
    }

    TEST_F(PassTests, PassFilter_PassHierarchy)
    {
        m_data->AddPassTemplatesToLibrary();

        // create a pass tree
        Ptr<Pass> pass = m_passSystem->CreatePassFromClass(Name("Pass"), Name("pass1"));
        Ptr<Pass> parent1 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent1"));
        Ptr<Pass> parent2 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent2"));
        Ptr<Pass> parent3 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent3"));

        parent3->AsParent()->AddChild(parent2);
        parent2->AsParent()->AddChild(parent1);
        parent1->AsParent()->AddChild(pass);

        {
            // Filter with pass hierarchy which has only one element
            PassFilter filter = PassFilter::CreateWithPassHierarchy({Name("pass1")});
            EXPECT_TRUE(filter.Matches(pass.get()));
        }

        {
            // Filter with empty pass hierarchy, triggers one assert
            AZ_TEST_START_TRACE_SUPPRESSION;
            PassFilter filter = PassFilter::CreateWithPassHierarchy(AZStd::vector<Name>{});
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }
        
        {
            // Filters with partial hierarchy by using string vector
            AZStd::vector<AZStd::string> passHierarchy1 = { "parent1", "pass1" };
            PassFilter filter1 = PassFilter::CreateWithPassHierarchy(passHierarchy1);
            EXPECT_TRUE(filter1.Matches(pass.get()));

            AZStd::vector<AZStd::string> passHierarchy2 = { "parent2", "pass1" };
            PassFilter filter2 = PassFilter::CreateWithPassHierarchy(passHierarchy2);
            EXPECT_TRUE(filter2.Matches(pass.get()));

            AZStd::vector<AZStd::string> passHierarchy3 = { "parent3", "parent2", "pass1" };
            PassFilter filter3 = PassFilter::CreateWithPassHierarchy(passHierarchy3);
            EXPECT_TRUE(filter3.Matches(pass.get()));
        }

        {
            // Filters with partial hierarchy by using Name vector
            AZStd::vector<Name> passHierarchy1 = { Name("parent1"), Name("pass1") };
            PassFilter filter1 = PassFilter::CreateWithPassHierarchy(passHierarchy1);
            EXPECT_TRUE(filter1.Matches(pass.get()));

            AZStd::vector<Name> passHierarchy2 = { Name("parent2"), Name("pass1")};
            PassFilter filter2 = PassFilter::CreateWithPassHierarchy(passHierarchy2);
            EXPECT_TRUE(filter2.Matches(pass.get()));

            AZStd::vector<Name> passHierarchy3 = { Name("parent3"), Name("parent2"), Name("pass1") };
            PassFilter filter3 = PassFilter::CreateWithPassHierarchy(passHierarchy3);
            EXPECT_TRUE(filter3.Matches(pass.get()));
        }

        {
            // Find non-leaf pass
            PassFilter filter1 = PassFilter::CreateWithPassHierarchy(AZStd::vector<AZStd::string>{"parent3", "parent1"});
            EXPECT_TRUE(filter1.Matches(parent1.get()));
            
            PassFilter filter2 = PassFilter::CreateWithPassHierarchy({ Name("parent1") });
            EXPECT_TRUE(filter2.Matches(parent1.get()));
            EXPECT_FALSE(filter2.Matches(pass.get()));
        }

        {
            // Failed to find pass
            // Mis-matching hierarchy 
            PassFilter filter1 = PassFilter::CreateWithPassHierarchy(AZStd::vector<AZStd::string>{"Parent1", "Parent3", "pass1"});
            EXPECT_FALSE(filter1.Matches(pass.get()));
            // Mis-matching name
            PassFilter filter2 = PassFilter::CreateWithPassHierarchy(AZStd::vector<AZStd::string>{"Parent1", "pass1"});
            EXPECT_FALSE(filter2.Matches(parent1.get()));
        }
    }

    TEST_F(PassTests, PassFilter_Empty_Success)
    {
        m_data->AddPassTemplatesToLibrary();

        // create a pass tree
        Ptr<Pass> pass = m_passSystem->CreatePassFromClass(Name("Pass"), Name("pass1"));
        Ptr<Pass> parent1 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent1"));
        Ptr<Pass> parent2 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent2"));
        Ptr<Pass> parent3 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent3"));

        parent3->AsParent()->AddChild(parent2);
        parent2->AsParent()->AddChild(parent1);
        parent1->AsParent()->AddChild(pass);

        PassFilter filter;

        // Any pass can match an empty filter
        EXPECT_TRUE(filter.Matches(pass.get()));
        EXPECT_TRUE(filter.Matches(parent1.get()));
        EXPECT_TRUE(filter.Matches(parent2.get()));
        EXPECT_TRUE(filter.Matches(parent3.get()));
    }
        
    TEST_F(PassTests, PassFilter_PassClass_Success)
    {
        m_data->AddPassTemplatesToLibrary();

        // create a pass tree
        Ptr<Pass> pass = m_passSystem->CreatePassFromClass(Name("Pass"), Name("pass1"));
        Ptr<Pass> depthPass = m_passSystem->CreatePassFromTemplate(Name("DepthPrePass"), Name("depthPass"));
        Ptr<Pass> parent1 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent1"));

        parent1->AsParent()->AddChild(pass);
        parent1->AsParent()->AddChild(depthPass);

        PassFilter filter1 = PassFilter::CreateWithPassClass<Pass>();

        EXPECT_TRUE(filter1.Matches(pass.get()));
        EXPECT_FALSE(filter1.Matches(parent1.get()));

        PassFilter filter2 = PassFilter::CreateWithPassClass<ParentPass>();
        EXPECT_FALSE(filter2.Matches(pass.get()));
        EXPECT_TRUE(filter2.Matches(parent1.get()));
    }
            
    TEST_F(PassTests, PassFilter_PassTemplate_Success)
    {
        m_data->AddPassTemplatesToLibrary();

        // create a pass tree
        Ptr<Pass> childPass = m_passSystem->CreatePassFromClass(Name("Pass"), Name("pass1"));
        Ptr<Pass> parent1 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent1"));

        PassFilter filter1 = PassFilter::CreateWithTemplateName(Name("Pass"), (Scene*) nullptr);
        // childPass doesn't have a template 
        EXPECT_FALSE(filter1.Matches(childPass.get()));

        PassFilter filter2 = PassFilter::CreateWithTemplateName(Name("ParentPass"), (Scene*) nullptr);
        EXPECT_TRUE(filter2.Matches(parent1.get()));
    }

    TEST_F(PassTests, ForEachPass_PassTemplateFilter_Success)
    {
        m_data->AddPassTemplatesToLibrary();

        // create a pass tree
        Ptr<Pass> pass = m_passSystem->CreatePassFromClass(Name("Pass"), Name("pass1"));
        Ptr<Pass> parent1 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent1"));
        Ptr<Pass> parent2 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent2"));
        Ptr<Pass> parent3 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent3"));

        parent3->AsParent()->AddChild(parent2);
        parent2->AsParent()->AddChild(parent1);
        parent1->AsParent()->AddChild(pass);

        // Create render pipeline
        const RPI::PipelineViewTag viewTag{ "viewTag1" };
        RPI::RenderPipelineDescriptor desc;
        desc.m_mainViewTagName = viewTag.GetStringView();
        desc.m_name = "TestPipeline";
        RPI::RenderPipelinePtr pipeline = RPI::RenderPipeline::CreateRenderPipeline(desc);
        Ptr<Pass> parent4 = m_passSystem->CreatePassFromTemplate(Name("ParentPass"), Name("parent4"));
        pipeline->GetRootPass()->AddChild(parent4);

        Name templateName = Name("ParentPass");
        PassFilter filter1 = PassFilter::CreateWithTemplateName(templateName, (RenderPipeline*)nullptr);

        int count = 0;
        m_passSystem->ForEachPass(filter1,  [&count, templateName](RPI::Pass* pass) -> PassFilterExecutionFlow
            {
                EXPECT_TRUE(pass->GetPassTemplate()->m_name == templateName);
                count++;
                return PassFilterExecutionFlow::ContinueVisitingPasses; 
            });

        // three from CreatePassFromTemplate() calls and one from Render Pipeline.
        EXPECT_TRUE(count == 4);

        count = 0;
        m_passSystem->ForEachPass(filter1,  [&count, templateName](RPI::Pass* pass) -> PassFilterExecutionFlow
            {
                EXPECT_TRUE(pass->GetPassTemplate()->m_name == templateName);
                count++;
                return PassFilterExecutionFlow::StopVisitingPasses;
            });
        EXPECT_TRUE(count == 1);

        PassFilter filter2 = PassFilter::CreateWithTemplateName(templateName, pipeline.get());
        count = 0;
        m_passSystem->ForEachPass(filter2,  [&count]([[maybe_unused]] RPI::Pass* pass) -> PassFilterExecutionFlow
            {
                count++;
                return PassFilterExecutionFlow::ContinueVisitingPasses;
            });

        // only the ParentPass in the render pipeline was found
        EXPECT_TRUE(count == 1);

    }
}
