/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/algorithm.h>

#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RHI.Edit/Utils.h>

#include "Common/ShaderBuilderTestFixture.h"
#include <ShaderBuildArgumentsManager.h>

namespace UnitTest
{
    class ShaderBuildArgumentsTests : public ShaderBuilderTestFixture
    {
    protected:

        AZ::RHI::ShaderBuildArguments CreateInitializedShaderBuildArguments()
        {
            AZ::RHI::ShaderBuildArguments arguments(
                true,
                { "-cpp1", "-cpp2", "-DMACRO1=1", "-DMACRO2=2" },
                { "--azslc1", "--azslc2", "--azslc3" },
                { "--dxc1" },
                { "--spirv1", "--spirv2", "--spirv3", "--spirv4" },
                { "--metalair1", "--metalair2" },
                { "--metallib1", "--metallib2", "--metallib3" });

            return arguments;
        }

        AZ::ShaderBuilder::ShaderBuildArgumentsManager CreateInitializedManager(
            AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> && removeBuildArgumentsMap,
            AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> && addBuildArgumentsMap)
        {
            AZ::ShaderBuilder::ShaderBuildArgumentsManager argsManager;
            argsManager.Init(AZStd::move(removeBuildArgumentsMap), AZStd::move(addBuildArgumentsMap));
            return argsManager;
        }

        // Because macros are evil, this short named function allows to write easy
        // EXPECT_EQ() expressions with string literal vector.
        // Example:
        // EXPECT_EQ(someVector, VSTR({"str1", "str2"}))
        static AZStd::vector<AZStd::string> VSTR(AZStd::vector<AZStd::string> args)
        {
            return args;
        }


    }; // class ShaderBuildArgumentsTests


    TEST_F(ShaderBuildArgumentsTests, CreateShaderBuildArguments_AllArraysMustBeEmpty)
    {
        AZ::RHI::ShaderBuildArguments arguments;

        EXPECT_FALSE(arguments.m_generateDebugInfo);
        EXPECT_TRUE(arguments.m_preprocessorArguments.empty());
        EXPECT_TRUE(arguments.m_azslcArguments.empty());
        EXPECT_TRUE(arguments.m_dxcArguments.empty());
        EXPECT_TRUE(arguments.m_spirvCrossArguments.empty());
        EXPECT_TRUE(arguments.m_metalAirArguments.empty());
        EXPECT_TRUE(arguments.m_metalLibArguments.empty());
    }

    TEST_F(ShaderBuildArgumentsTests, AddEmptyShaderBuildArguments_AllArraysMustBeEmpty)
    {
        AZ::RHI::ShaderBuildArguments lhs;
        AZ::RHI::ShaderBuildArguments rhs;
        AZ::RHI::ShaderBuildArguments arguments = lhs + rhs;

        EXPECT_FALSE(arguments.m_generateDebugInfo);
        EXPECT_TRUE(arguments.m_preprocessorArguments.empty());
        EXPECT_TRUE(arguments.m_azslcArguments.empty());
        EXPECT_TRUE(arguments.m_dxcArguments.empty());
        EXPECT_TRUE(arguments.m_spirvCrossArguments.empty());
        EXPECT_TRUE(arguments.m_metalAirArguments.empty());
        EXPECT_TRUE(arguments.m_metalLibArguments.empty());
    }

    TEST_F(ShaderBuildArgumentsTests, SubtractEmptyShaderBuildArguments_AllArraysMustBeEmpty)
    {
        AZ::RHI::ShaderBuildArguments lhs;
        AZ::RHI::ShaderBuildArguments rhs;
        AZ::RHI::ShaderBuildArguments arguments = lhs - rhs;

        EXPECT_FALSE(arguments.m_generateDebugInfo);
        EXPECT_TRUE(arguments.m_preprocessorArguments.empty());
        EXPECT_TRUE(arguments.m_azslcArguments.empty());
        EXPECT_TRUE(arguments.m_dxcArguments.empty());
        EXPECT_TRUE(arguments.m_spirvCrossArguments.empty());
        EXPECT_TRUE(arguments.m_metalAirArguments.empty());
        EXPECT_TRUE(arguments.m_metalLibArguments.empty());
    }

    TEST_F(ShaderBuildArgumentsTests, AccumulateEmptyShaderBuildArguments_AllArraysMustBeEmpty)
    {
        AZ::RHI::ShaderBuildArguments arguments;
        AZ::RHI::ShaderBuildArguments rhs;

        arguments += rhs;

        EXPECT_FALSE(arguments.m_generateDebugInfo);
        EXPECT_TRUE(arguments.m_preprocessorArguments.empty());
        EXPECT_TRUE(arguments.m_azslcArguments.empty());
        EXPECT_TRUE(arguments.m_dxcArguments.empty());
        EXPECT_TRUE(arguments.m_spirvCrossArguments.empty());
        EXPECT_TRUE(arguments.m_metalAirArguments.empty());
        EXPECT_TRUE(arguments.m_metalLibArguments.empty());
    }

    TEST_F(ShaderBuildArgumentsTests, SubtractEqualEmptyShaderBuildArguments_AllArraysMustBeEmpty)
    {
        AZ::RHI::ShaderBuildArguments arguments;
        AZ::RHI::ShaderBuildArguments rhs;

        arguments -= rhs;

        EXPECT_FALSE(arguments.m_generateDebugInfo);
        EXPECT_TRUE(arguments.m_preprocessorArguments.empty());
        EXPECT_TRUE(arguments.m_azslcArguments.empty());
        EXPECT_TRUE(arguments.m_dxcArguments.empty());
        EXPECT_TRUE(arguments.m_spirvCrossArguments.empty());
        EXPECT_TRUE(arguments.m_metalAirArguments.empty());
        EXPECT_TRUE(arguments.m_metalLibArguments.empty());
    }

    TEST_F(ShaderBuildArgumentsTests, InitializeShaderBuildArguments_AddEmpty_RemainsUnchanged)
    {
        AZ::RHI::ShaderBuildArguments arguments = CreateInitializedShaderBuildArguments();
        const auto generateDebugInfo =       arguments.m_generateDebugInfo;
        const auto preprocessorArgumentsSize =        arguments.m_preprocessorArguments.size();
        const auto azslcArgumentsSize =      arguments.m_azslcArguments.size();
        const auto dxcArgumentsSize =        arguments.m_dxcArguments.size();
        const auto spirvCrossArgumentsSize = arguments.m_spirvCrossArguments.size();
        const auto metalAirArgumentsSize =   arguments.m_metalAirArguments.size();
        const auto metalLibArgumentsSize =   arguments.m_metalLibArguments.size();

        AZ::RHI::ShaderBuildArguments rhs;

        arguments += rhs;

        EXPECT_EQ(generateDebugInfo,       arguments.m_generateDebugInfo);
        EXPECT_EQ(preprocessorArgumentsSize,        arguments.m_preprocessorArguments.size());
        EXPECT_EQ(azslcArgumentsSize,      arguments.m_azslcArguments.size());
        EXPECT_EQ(dxcArgumentsSize,        arguments.m_dxcArguments.size());
        EXPECT_EQ(spirvCrossArgumentsSize, arguments.m_spirvCrossArguments.size());
        EXPECT_EQ(metalAirArgumentsSize,   arguments.m_metalAirArguments.size());
        EXPECT_EQ(metalLibArgumentsSize,   arguments.m_metalLibArguments.size());
    }

    TEST_F(ShaderBuildArgumentsTests, InitializeShaderBuildArguments_SubtractEmpty_RemainsUnchanged)
    {
        AZ::RHI::ShaderBuildArguments arguments = CreateInitializedShaderBuildArguments();
        const auto generateDebugInfo =       arguments.m_generateDebugInfo;
        const auto preprocessorArgumentsSize =        arguments.m_preprocessorArguments.size();
        const auto azslcArgumentsSize =      arguments.m_azslcArguments.size();
        const auto dxcArgumentsSize =        arguments.m_dxcArguments.size();
        const auto spirvCrossArgumentsSize = arguments.m_spirvCrossArguments.size();
        const auto metalAirArgumentsSize =   arguments.m_metalAirArguments.size();
        const auto metalLibArgumentsSize =   arguments.m_metalLibArguments.size();

        AZ::RHI::ShaderBuildArguments rhs;

        arguments -= rhs;

        EXPECT_EQ(generateDebugInfo,       arguments.m_generateDebugInfo);
        EXPECT_EQ(preprocessorArgumentsSize,        arguments.m_preprocessorArguments.size());
        EXPECT_EQ(azslcArgumentsSize,      arguments.m_azslcArguments.size());
        EXPECT_EQ(dxcArgumentsSize,        arguments.m_dxcArguments.size());
        EXPECT_EQ(spirvCrossArgumentsSize, arguments.m_spirvCrossArguments.size());
        EXPECT_EQ(metalAirArgumentsSize,   arguments.m_metalAirArguments.size());
        EXPECT_EQ(metalLibArgumentsSize,   arguments.m_metalLibArguments.size());
    }

    TEST_F(ShaderBuildArgumentsTests, InitializeShaderBuildArguments_AddSameArguments_RemainsUnchanged)
    {
        AZ::RHI::ShaderBuildArguments arguments = CreateInitializedShaderBuildArguments();
        AZ::RHI::ShaderBuildArguments sameArguments = CreateInitializedShaderBuildArguments();

        arguments += sameArguments;

        EXPECT_EQ(arguments.m_generateDebugInfo,   sameArguments.m_generateDebugInfo);
        EXPECT_EQ(arguments.m_preprocessorArguments,        sameArguments.m_preprocessorArguments);
        EXPECT_EQ(arguments.m_azslcArguments,      sameArguments.m_azslcArguments);
        EXPECT_EQ(arguments.m_dxcArguments,        sameArguments.m_dxcArguments);
        EXPECT_EQ(arguments.m_spirvCrossArguments, sameArguments.m_spirvCrossArguments);
        EXPECT_EQ(arguments.m_metalAirArguments,   sameArguments.m_metalAirArguments);
        EXPECT_EQ(arguments.m_metalLibArguments,   sameArguments.m_metalLibArguments);
    }

    TEST_F(ShaderBuildArgumentsTests, InitializeShaderBuildArguments_SubtractSameArguments_ChangesToEmpty)
    {
        AZ::RHI::ShaderBuildArguments arguments = CreateInitializedShaderBuildArguments();
        AZ::RHI::ShaderBuildArguments sameArguments = CreateInitializedShaderBuildArguments();

        arguments -= sameArguments;

        EXPECT_FALSE(arguments.m_generateDebugInfo);
        EXPECT_TRUE(arguments.m_preprocessorArguments.empty());
        EXPECT_TRUE(arguments.m_azslcArguments.empty());
        EXPECT_TRUE(arguments.m_dxcArguments.empty());
        EXPECT_TRUE(arguments.m_spirvCrossArguments.empty());
        EXPECT_TRUE(arguments.m_metalAirArguments.empty());
        EXPECT_TRUE(arguments.m_metalLibArguments.empty());
    }

    TEST_F(ShaderBuildArgumentsTests, CreateEmtpyShaderBuildArguments_AppendDefinitionsSomeWithSpaces_AppendSuccessful)
    {
        AZ::RHI::ShaderBuildArguments arguments;

        auto definitions = VSTR({"MACRO1", "MACRO2=VALUE2", "  MACRO3", "MACRO4   ", "  MACRO5=VALUE5  "});
        arguments.AppendDefinitions(definitions);

        EXPECT_EQ(arguments.m_preprocessorArguments, VSTR({ "-DMACRO1", "-DMACRO2=VALUE2", "-DMACRO3", "-DMACRO4", "-DMACRO5=VALUE5" }));
    }


    TEST_F(ShaderBuildArgumentsTests, InitializeShaderBuildArguments_AppendDefinitionsWithTypos_ExpectError)
    {
        AZ::RHI::ShaderBuildArguments arguments = CreateInitializedShaderBuildArguments();
        auto definitions = VSTR({"-DMACRO1", "MACRO2=VALUE2"}); // Not allowed to start with '-'

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(arguments.AppendDefinitions(definitions) >= 0);
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;

        definitions = VSTR({"MACRO2 = VALUE2", "MACRO1"}); // Not allowed to have spaces in between.

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(arguments.AppendDefinitions(definitions) >= 0);
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
    }

    TEST_F(ShaderBuildArgumentsTests, InitializeShaderBuildArgumentsManager_ValidateAllOperations)
    {
        // Let's initialize the arguments.
        AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> removeBuildArgumentsMap;
        AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> addBuildArgumentsMap;

        // Globals. The name of the global arguments is always the empty string.
        // This will be level 1.
        addBuildArgumentsMap.emplace("", AZ::RHI::ShaderBuildArguments(false
            , { "--cpp1" }
            , { "--azslc1" }
            , { "--dxc1" }
            , { "--spirv1" }
            , { "--metalair1" }
            , { "--metallib1" })
        );
        removeBuildArgumentsMap.emplace("", AZ::RHI::ShaderBuildArguments());

        // Simulates common arguments for all RHIs under the "Windows" platform.
        addBuildArgumentsMap.emplace("Windows", AZ::RHI::ShaderBuildArguments(false
            , { "--cpp2w" }
            , { "--azslc2w" }
            , { "--dxc2w" }
            , { "--spirv2w" }
            , { "--metalair2w" }
            , { "--metallib2w" })
        );
        removeBuildArgumentsMap.emplace("Windows", AZ::RHI::ShaderBuildArguments());

        // Simulates "dx12" arguments for "Windows"
        addBuildArgumentsMap.emplace("Windows.dx12", AZ::RHI::ShaderBuildArguments(false
            , { "--cpp3w" }
            , { "--azslc3w" }
            , { "--dxc3w" }
            , { "--spirv3w" }
            , { "--metalair3w" }
            , { "--metallib3w" })
        );
        removeBuildArgumentsMap.emplace("Windows.dx12", AZ::RHI::ShaderBuildArguments());

        // Simulates "vulkan" arguments for "Windows"
        addBuildArgumentsMap.emplace("Windows.vulkan", AZ::RHI::ShaderBuildArguments(true
            , { "--cpp4w" }
            , { "--azslc4w" }
            , { "--dxc4w" }
            , { "--spirv4w" }
            , { "--metalair4w" }
            , { "--metallib4w" })
        );
        removeBuildArgumentsMap.emplace("Windows.vulkan", AZ::RHI::ShaderBuildArguments());

        // Simulates "vulkan" arguments for "Linux"
        addBuildArgumentsMap.emplace("Linux.vulkan", AZ::RHI::ShaderBuildArguments(true
            , { "--cpp3l" }
            , { "--azslc3l" }
            , { "--dxc3l" }
            , { "--spirv3l" }
            , { "--metalair3l" }
            , { "--metallib3l" })
        );
        removeBuildArgumentsMap.emplace("Linux.vulkan", AZ::RHI::ShaderBuildArguments());

        // Simulates some arguments customized by a .shader.
        AZ::RHI::ShaderBuildArguments addShaderArgs(true
            , { "--cpp5s" }
            , { "--azslc5s" }
            , { "--dxc5s" }
            , { "--spirv5s" }
            , { "--metalair5s" }
            , { "--metallib5s" });
        AZ::RHI::ShaderBuildArguments removeShaderArgs;

        // Simulates some arguments customized by a supervariant inside the .shader.
        AZ::RHI::ShaderBuildArguments addSuperVariantArgs(true
            , { "--cpp6sv" }
            , { "--azslc6sv" }
            , { "--dxc6sv" }
            , { "--spirv6sv" }
            , { "--metalair6sv" }
            , { "--metallib6sv" });
         AZ::RHI::ShaderBuildArguments removeSuperVariantArgs(false
            , { }
            , { }
            , { }
            , { "--spirv5s" }
            , { "--metalair5s" }
            , { "--metallib5s" });

        AZ::ShaderBuilder::ShaderBuildArgumentsManager argsManager = CreateInitializedManager(AZStd::move(removeBuildArgumentsMap), AZStd::move(addBuildArgumentsMap));

        // We have a fully initialized ShaderBuildArgumentsManager. The "" (global) set of arguments is the starting set (or scope) of
        // arguments
        auto buildArgs = argsManager.GetCurrentArguments();

        EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1" }));
        EXPECT_EQ(buildArgs.m_azslcArguments, VSTR({ "--azslc1" }));

        // Simulate for loop across all platforms and across RHIs per platform.
        {
            buildArgs = argsManager.PushArgumentScope("Windows");
            // Now the current set of arguments at the top of the stack are the addition of the global arguments, and "Windows" arguments.
            EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w" }));
            EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w" }));

            // Simulate looping across RHIs for Windows.
            {
                buildArgs = argsManager.PushArgumentScope("dx12");
                EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w", "--cpp3w" }));
                EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w", "--dxc3w" }));

                {
                    // Simulate shader arguments.
                    buildArgs = argsManager.PushArgumentScope(removeShaderArgs, addShaderArgs, {"MACRO1  ", "  MACRO2=VALUE2"}); // Spaces in MACROxx added on purpose.
                    EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w", "--cpp3w", "-DMACRO1", "-DMACRO2=VALUE2", "--cpp5s"}));
                    EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w", "--dxc3w", "--dxc5s" }));
                    EXPECT_EQ(buildArgs.m_spirvCrossArguments, VSTR({ "--spirv1", "--spirv2w", "--spirv3w", "--spirv5s" }));
                    EXPECT_EQ(buildArgs.m_metalAirArguments, VSTR({ "--metalair1", "--metalair2w", "--metalair3w", "--metalair5s" }));
                    EXPECT_EQ(buildArgs.m_metalLibArguments, VSTR({ "--metallib1", "--metallib2w", "--metallib3w", "--metallib5s" }));

                    // Simulate supervariant arguments.
                    buildArgs = argsManager.PushArgumentScope(removeSuperVariantArgs, addSuperVariantArgs, VSTR({" MACRO3  ", " MACRO4=VALUE4 "})); // Spaces in MACROxx added on purpose.
                    EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w", "--cpp3w", "-DMACRO1", "-DMACRO2=VALUE2", "--cpp5s", "-DMACRO3", "-DMACRO4=VALUE4", "--cpp6sv"}));
                    EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w", "--dxc3w", "--dxc5s", "--dxc6sv" }));
                    // Notice that in this case the supervariant removes the shader arguments "--spirv5s", "--metalair5s" & "--metallib5s"
                    EXPECT_EQ(buildArgs.m_spirvCrossArguments, VSTR({ "--spirv1", "--spirv2w", "--spirv3w", "--spirv6sv" }));
                    EXPECT_EQ(buildArgs.m_metalAirArguments, VSTR({ "--metalair1", "--metalair2w", "--metalair3w", "--metalair6sv" }));
                    EXPECT_EQ(buildArgs.m_metalLibArguments, VSTR({ "--metallib1", "--metallib2w", "--metallib3w", "--metallib6sv" }));

                    // Pop the supervariant arguments.
                    argsManager.PopArgumentScope();
                    buildArgs = argsManager.GetCurrentArguments();
                    EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w", "--cpp3w", "-DMACRO1", "-DMACRO2=VALUE2", "--cpp5s"}));
                    EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w", "--dxc3w", "--dxc5s" }));
                    EXPECT_EQ(buildArgs.m_spirvCrossArguments, VSTR({ "--spirv1", "--spirv2w", "--spirv3w", "--spirv5s" }));
                    EXPECT_EQ(buildArgs.m_metalAirArguments, VSTR({ "--metalair1", "--metalair2w", "--metalair3w", "--metalair5s" }));
                    EXPECT_EQ(buildArgs.m_metalLibArguments, VSTR({ "--metallib1", "--metallib2w", "--metallib3w", "--metallib5s" }));

                    // Pop the shader arguments.
                    argsManager.PopArgumentScope();
                    buildArgs = argsManager.GetCurrentArguments();
                    EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w", "--cpp3w" }));
                    EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w", "--dxc3w" }));
                }


                // Pop the rhi before pushing the next rhi.
                argsManager.PopArgumentScope();
                buildArgs = argsManager.GetCurrentArguments();
                EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w" }));
                EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w" }));

                // Push the "vulkan" arguments.
                buildArgs = argsManager.PushArgumentScope("vulkan");
                EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w", "--cpp4w" }));
                EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w", "--dxc4w" }));

                {
                    // Simulate shader arguments.
                    buildArgs = argsManager.PushArgumentScope(removeShaderArgs, addShaderArgs, {});
                    EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w", "--cpp4w", "--cpp5s" }));
                    EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w", "--dxc4w", "--dxc5s" }));
                    EXPECT_EQ(buildArgs.m_spirvCrossArguments, VSTR({ "--spirv1", "--spirv2w", "--spirv4w", "--spirv5s" }));
                    EXPECT_EQ(buildArgs.m_metalAirArguments, VSTR({ "--metalair1", "--metalair2w", "--metalair4w", "--metalair5s" }));
                    EXPECT_EQ(buildArgs.m_metalLibArguments, VSTR({ "--metallib1", "--metallib2w", "--metallib4w", "--metallib5s" }));

                    // Simulate supervariant arguments.
                    buildArgs = argsManager.PushArgumentScope(removeSuperVariantArgs, addSuperVariantArgs, {});
                    EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w", "--cpp4w", "--cpp5s", "--cpp6sv" }));
                    EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w", "--dxc4w", "--dxc5s", "--dxc6sv" }));
                    // Notice that in this case the supervariant removes the shader arguments "--spirv5s", "--metalair5s" & "--metallib5s"
                    EXPECT_EQ(buildArgs.m_spirvCrossArguments, VSTR({ "--spirv1", "--spirv2w", "--spirv4w", "--spirv6sv" }));
                    EXPECT_EQ(buildArgs.m_metalAirArguments, VSTR({ "--metalair1", "--metalair2w", "--metalair4w", "--metalair6sv" }));
                    EXPECT_EQ(buildArgs.m_metalLibArguments, VSTR({ "--metallib1", "--metallib2w", "--metallib4w", "--metallib6sv" }));

                    // Pop the supervariant arguments.
                    argsManager.PopArgumentScope();
                    buildArgs = argsManager.GetCurrentArguments();
                    EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w", "--cpp4w", "--cpp5s" }));
                    EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w", "--dxc4w", "--dxc5s" }));
                    EXPECT_EQ(buildArgs.m_spirvCrossArguments, VSTR({ "--spirv1", "--spirv2w", "--spirv4w", "--spirv5s" }));
                    EXPECT_EQ(buildArgs.m_metalAirArguments, VSTR({ "--metalair1", "--metalair2w", "--metalair4w", "--metalair5s" }));
                    EXPECT_EQ(buildArgs.m_metalLibArguments, VSTR({ "--metallib1", "--metallib2w", "--metallib4w", "--metallib5s" }));

                    // Pop the shader arguments.
                    argsManager.PopArgumentScope();
                    buildArgs = argsManager.GetCurrentArguments();
                    EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w", "--cpp4w" }));
                    EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w", "--dxc4w" }));
                }

                // Pop the rhi before pushing the next rhi.
                argsManager.PopArgumentScope();
                buildArgs = argsManager.GetCurrentArguments();
                EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp2w" }));
                EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc2w" }));
            }

            // Pop the platform before changing Platforms.
            argsManager.PopArgumentScope();
            buildArgs = argsManager.GetCurrentArguments();
            EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1" }));
            EXPECT_EQ(buildArgs.m_azslcArguments, VSTR({ "--azslc1" }));

            buildArgs = argsManager.PushArgumentScope("Linux");
            // In this test case, the platform "Linux" does not customize the arguments.
            // We expect the same arguments as before.
            EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1" }));
            EXPECT_EQ(buildArgs.m_azslcArguments, VSTR({ "--azslc1" }));

            // Simulate looping across RHIs for Linux.
            {
                buildArgs = argsManager.PushArgumentScope("dx12");
                // Linux doesn't work with dx12. Expect arguments to be unchanged.
                EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1" }));
                EXPECT_EQ(buildArgs.m_azslcArguments, VSTR({ "--azslc1" }));

                // Pop the rhi before pushing the next rhi.
                argsManager.PopArgumentScope();
                buildArgs = argsManager.GetCurrentArguments();
                EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1" }));
                EXPECT_EQ(buildArgs.m_azslcArguments, VSTR({ "--azslc1" }));

                // Push the "vulkan" arguments.
                buildArgs = argsManager.PushArgumentScope("vulkan");
                EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1", "--cpp3l" }));
                EXPECT_EQ(buildArgs.m_dxcArguments, VSTR({ "--dxc1", "--dxc3l" }));

                // Pop the rhi before pushing the next rhi.
                argsManager.PopArgumentScope();
                buildArgs = argsManager.GetCurrentArguments();
                EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1" }));
                EXPECT_EQ(buildArgs.m_azslcArguments, VSTR({ "--azslc1" }));
            }

            // Pop the platform before changing Platforms.
            argsManager.PopArgumentScope();
            buildArgs = argsManager.GetCurrentArguments();
            EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1" }));
            EXPECT_EQ(buildArgs.m_azslcArguments, VSTR({ "--azslc1" }));
        }

        // At the moment the current scope is the global scope, named "".
        buildArgs = argsManager.GetCurrentArguments();
        EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1" }));
        EXPECT_EQ(buildArgs.m_azslcArguments, VSTR({ "--azslc1" }));
 
        // No matter how many times We pop, the global set of arguments is never removed.
        argsManager.PopArgumentScope();
        argsManager.PopArgumentScope();
        argsManager.PopArgumentScope();
        buildArgs = argsManager.GetCurrentArguments();
        EXPECT_EQ(buildArgs.m_preprocessorArguments, VSTR({ "--cpp1" }));
        EXPECT_EQ(buildArgs.m_azslcArguments, VSTR({ "--azslc1" }));

    }

} //namespace UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

