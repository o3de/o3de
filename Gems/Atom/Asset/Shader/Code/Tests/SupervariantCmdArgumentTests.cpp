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

#include <CommonFiles/GlobalBuildOptions.h>

#include "Common/ShaderBuilderTestFixture.h"

namespace UnitTest
{
    using namespace AZ;

    struct KeyValueView
    {
        AZStd::string_view m_key;
        AZStd::string_view m_value;
    };

    class SupervariantCmdArgumentTests : public ShaderBuilderTestFixture
    {
    protected:
        static constexpr char MCPP_MACRO1[] = "MACRO1";
        static constexpr char MCPP_VALUE1[] = "VALUE1a";
        static constexpr char MCPP_NEW_VALUE1[] = "VALUE1b"; // Missing A is not a typo

        static constexpr char MCPP_MACRO2[] = "MACRO2";
        static constexpr char MCPP_VALUE2[] = "VALUE2";

        static constexpr char MCPP_MACRO3[] = "MACRO3";
        static constexpr char MCPP_VALUE3[] = "VALUE3a";
        static constexpr char MCPP_NEW_VALUE3[] = "VALUE3b";

        static constexpr char MCPP_MACRO4[] = "MACRO4";

        static constexpr char MCPP_MACRO5[] = "MACRO5";

        static constexpr char MCPP_MACRO6[] = "MACRO6";
        static constexpr char MCPP_VALUE6[] = "VALUE6";

        static constexpr char AZSLC_ARG1[] = "--azsl1";

        static constexpr char AZSLC_ARG2[] = "--azsl2";
        static constexpr char AZSLC_VAL2[] = "open,source";
        static constexpr char AZSLC_NEW_VAL2a[] = "closed,binary";
        static constexpr char AZSLC_NEW_VAL2b[] = "closed,source";

        static constexpr char AZSLC_ARG3[] = "--azsl3";
        static constexpr char AZSLC_VAL3[] = "blue";

        static constexpr char AZSLC_ARG4[] = "-azsl4";

        static constexpr char AZSLC_ARG5[] = "--azsl5";
        static constexpr char AZSLC_VAL5[] = "smith,wick,john,45,-1,-1";
        static constexpr char AZSLC_NEW_VAL5[] = "apple,seed,crisp,-1,2,0";

        static constexpr char AZSLC_ARG6[] = "--azsl6";

        static constexpr char AZSLC_ARG7[] = "--azsl7";

        //! Helper function.
        //! Given an input list of {Key, Value} pairs returns a list of strings where each string is of the form: "Key=Value". 
        AZStd::vector<AZStd::string> CreateListOfStringsFromListOfKeyValues(AZStd::array_view<KeyValueView> listOfKeyValues) const
        {
            AZStd::vector<AZStd::string> listOfStrings;
            for (const auto& keyValue : listOfKeyValues)
            {
                if (keyValue.m_value.empty())
                {
                    listOfStrings.push_back(keyValue.m_key);
                }
                else
                {
                    listOfStrings.push_back(AZStd::string::format("%s=%s", keyValue.m_key.data(), keyValue.m_value.data()));
                }
            }
            return listOfStrings;
        }

        //! Helper function.
        //! Given an input list of {Key, Value} pairs returns a list of strings where each string is of the form: "Key1", "Value1", "Key2", "Value2". 
        AZStd::vector<AZStd::string> CreateListOfSingleStringsFromListOfKeyValues(AZStd::array_view<KeyValueView> listOfKeyValues) const
        {
            AZStd::vector<AZStd::string> listOfStrings;
            for (const auto& keyValue : listOfKeyValues)
            {
                listOfStrings.push_back(keyValue.m_key);
                if (!keyValue.m_value.empty())
                {
                    listOfStrings.push_back(keyValue.m_value);
                }
            }
            return listOfStrings;
        }

        //! Helper function.
        //! @param outputString: [out] The string " @argName" gets appended to it (The space is intentional).
        //!                      Alternatively, if @argValue is NOT empty, then the string " @argName=@argValue" is
        //!                      appended to it.
        //! @param argName: A typical command line argument. "-p" or "--some".
        //! @param argValue: A string representing the value that should be appended to @argName.
        void AppendCmdLineArgument(AZStd::string& outputString, AZStd::string_view argName, AZStd::string_view argValue) const
        {
            if (argValue.empty())
            {
                outputString += AZStd::string::format(" %s", argName.data());
            }
            else
            {
                outputString += AZStd::string::format(" %s=%s", argName.data(), argValue.data());
            }
        }

        //! Helper function.
        //! Similar to above, but assumes that @argName refers to just the name of a macro definition so the appended string will always start
        //! with "-D". 
        void AppendMacroDefinitionArgument(AZStd::string& outputString, AZStd::string_view argName, AZStd::string_view argValue) const
        {
            AppendCmdLineArgument(outputString, AZStd::string::format("-D%s", argName.data()), argValue);
        }

        //! A helper made of helpers.
        //! Returns a command line string that results of concatenating the input list of {Key, Value} pairs (with '=').
        //! Example of a returned string:
        //! "key1=value1 key2 key3 key4=value" 
        AZStd::string CreateCmdLineStringFromListOfKeyValues(AZStd::array_view<KeyValueView> listOfKeyValues) const
        {
            AZStd::string cmdLineString;
            for (const auto& keyValueView : listOfKeyValues)
            {
                AppendCmdLineArgument(cmdLineString, keyValueView.m_key, keyValueView.m_value);
            }
            return cmdLineString;
        }

        //! A helper made of helpers.
        //! Returns a command line string of macro definitions that results of concatenating the input list of {Key, Value} pairs.
        //! Example of a returned string:
        //! "-Dkey1=value1 -Dkey2 -Dkey3 -Dkey4=value"
        AZStd::string CreateMacroDefinitionCmdLineStringFromListOfKeyValues(AZStd::array_view<KeyValueView> listOfKeyValues) const
        {
            AZStd::string cmdLineString;
            for (const auto& keyValueView : listOfKeyValues)
            {
                AppendMacroDefinitionArgument(cmdLineString, keyValueView.m_key, keyValueView.m_value);
            }
            return cmdLineString;
        }

        //! @param includePaths A List of folder paths
        //! @param predefinedMacros A List of strings with format: "name[=value]"
        ShaderBuilder::PreprocessorOptions CreatePreprocessorOptions(
            AZStd::array_view<AZStd::string> includePaths, AZStd::array_view<AZStd::string> predefinedMacros) const
        {
            ShaderBuilder::PreprocessorOptions preprocessorOptions;

            preprocessorOptions.m_projectIncludePaths.reserve(includePaths.size());
            for (const auto& path : includePaths)
            {
                preprocessorOptions.m_projectIncludePaths.push_back(path);
            }

            preprocessorOptions.m_predefinedMacros.reserve(predefinedMacros.size());
            for (const auto& macro : predefinedMacros)
            {
                preprocessorOptions.m_predefinedMacros.push_back(macro);
            }

            return preprocessorOptions;
        }

        //! @param azslcAdditionalFreeArguments: A string representing series of command line arguments for AZSLc.
        //! @param dxcAdditionalFreeArguments: A string representing series of command line arguments for DXC.
        RHI::ShaderCompilerArguments CreateShaderCompilerArguments(
            AZStd::string_view azslcAdditionalFreeArguments, AZStd::string_view dxcAdditionalFreeArguments) const
        {
            RHI::ShaderCompilerArguments shaderCompilerArguments;
            shaderCompilerArguments.m_azslcWarningLevel = 1;
            shaderCompilerArguments.m_azslcAdditionalFreeArguments = azslcAdditionalFreeArguments;
            shaderCompilerArguments.m_defaultMatrixOrder = RHI::MatrixOrder::Row;
            shaderCompilerArguments.m_dxcAdditionalFreeArguments = dxcAdditionalFreeArguments;

            return shaderCompilerArguments;
        }


        //! @param includePaths A List of folder paths
        //! @param predefinedMacros A List of strings with format: "name[=value]"
        //! @param azslcAdditionalFreeArguments A string representing series of command line arguments for AZSLc.
        //! @param dxcAdditionalFreeArguments: A string representing series of command line arguments for DXC.
        ShaderBuilder::GlobalBuildOptions CreateGlobalBuildOptions(
            AZStd::array_view<AZStd::string> includePaths,
            AZStd::array_view<AZStd::string> predefinedMacros,
            AZStd::string_view azslcAdditionalFreeArguments,
            AZStd::string_view dxcAdditionalFreeArguments) const
        {
            ShaderBuilder::GlobalBuildOptions globalBuildOptions;
            globalBuildOptions.m_preprocessorSettings = CreatePreprocessorOptions(includePaths, predefinedMacros);
            globalBuildOptions.m_compilerArguments =
                CreateShaderCompilerArguments(azslcAdditionalFreeArguments, dxcAdditionalFreeArguments);
            return globalBuildOptions;
        }

        //! @param name Name of the supervariant.
        //! @param plusArguments A string with command line arguments that contains both C-preprocessor macro definitions
        //!        and other command line arguments for AZSLc.
        //! @param minusArguments A string with command line arguments that should be removed from the finalized command line arguments.
        //!        it can contain both, C-preprocessor macro definitions and other command line arguments for AZSLc.
        RPI::ShaderSourceData::SupervariantInfo CreateSupervariantInfo(
            AZStd::string_view name, AZStd::string_view plusArguments, AZStd::string_view minusArguments) const
        {
            RPI::ShaderSourceData::SupervariantInfo supervariantInfo;
            supervariantInfo.m_name = name;
            supervariantInfo.m_plusArguments = plusArguments;
            supervariantInfo.m_minusArguments = minusArguments;
            return supervariantInfo;
        }

        bool StringContainsAllSubstrings(AZStd::string_view haystack, AZStd::array_view<AZStd::string> substrings)
        {
            return AZStd::all_of(AZ_BEGIN_END(substrings),
                [&](AZStd::string_view needle) -> bool
                {
                    return (haystack.find(needle) != AZStd::string::npos);
                }
            );
        }

        bool StringDoesNotContainAnyOneOfTheSubstrings(AZStd::string_view haystack, AZStd::array_view<AZStd::string> substrings)
        {
            return AZStd::all_of(AZ_BEGIN_END(substrings), [&](AZStd::string_view needle) -> bool {
                return (haystack.find(needle) == AZStd::string::npos);
            });
        }

        //! @returns: True if all strings in @substring appear in @vectorOfString.
        //! @remark: Keep in mind that this is not the same as saying that all strings in @vectorOfStrings appear in @substrings.
        bool VectorContainsAllSubstrings(
            AZStd::array_view<AZStd::string> vectorOfStrings, AZStd::array_view<AZStd::string> substrings)
        {
            return AZStd::all_of(
                AZ_BEGIN_END(substrings),
                [&](AZStd::string_view needle) -> bool {
                bool res = AZStd::any_of(AZ_BEGIN_END(vectorOfStrings),
                        [&](AZStd::string_view haystack) -> bool
                        {
                            return haystack.find(needle) != AZStd::string::npos;
                        }
                    );
                    return res;
                }
            );
        }

        //! @returns: True only if None of the strings in @vectorOfStrings contains any of the strings in @substrings.
        bool VectorDoesNotContainAnyOneOfTheSubstrings(AZStd::array_view<AZStd::string> vectorOfStrings, AZStd::array_view<AZStd::string> substrings)
        {
            return AZStd::all_of(AZ_BEGIN_END(vectorOfStrings), [&](AZStd::string_view haystack) -> bool {
                return StringDoesNotContainAnyOneOfTheSubstrings(haystack, substrings);
            });
        }

    }; // class SupervariantCmdArgumentTests


    TEST_F(SupervariantCmdArgumentTests, CommandLineArgumentUtils_ValidateHelperFunctions)
    {
        // In this test the idea is to validate the static helper functions in AZ::RHI::ShaderCompilerArguments class
        // that are useful for command line argument manipulation, etc.
        AZStd::vector<KeyValueView> argumentList = {
            {AZSLC_ARG1, ""}, {AZSLC_ARG2, AZSLC_VAL2}, {AZSLC_ARG3, AZSLC_VAL3}, {AZSLC_ARG4, ""}, {AZSLC_ARG5, AZSLC_VAL5},
        };

        auto argumentsAsString = CreateCmdLineStringFromListOfKeyValues(argumentList);
        auto listOfArgumentNames = AZ::RHI::CommandLineArgumentUtils::GetListOfArgumentNames(argumentsAsString);

        EXPECT_TRUE(AZStd::all_of(AZ_BEGIN_END(argumentList), [&](const KeyValueView& needle) -> bool {
            return (AZStd::find(AZ_BEGIN_END(listOfArgumentNames), needle.m_key) != listOfArgumentNames.end()) &&
                   // Make sure the values did not make into the expected list of keys.
                   (AZStd::find(AZ_BEGIN_END(listOfArgumentNames), needle.m_value) == listOfArgumentNames.end());
        }));

        AZStd::vector<AZStd::string> listOfArgumentsToRemove = { AZSLC_ARG4, AZSLC_ARG2 };
        auto stringWithRemovedArguments =
            AZ::RHI::CommandLineArgumentUtils::RemoveArgumentsFromCommandLineString(listOfArgumentsToRemove, argumentsAsString);
        EXPECT_TRUE(AZStd::all_of(AZ_BEGIN_END(listOfArgumentsToRemove), [&](const AZStd::string& needle) -> bool {
            return stringWithRemovedArguments.find(needle) == AZStd::string::npos;
        }));

        AZStd::vector<AZStd::string> listOfSurvivingArguments = {AZSLC_ARG1, AZSLC_ARG3, AZSLC_ARG5};
        EXPECT_TRUE(AZStd::all_of(AZ_BEGIN_END(listOfSurvivingArguments), [&](const AZStd::string& needle) -> bool {
            return stringWithRemovedArguments.find(needle) != AZStd::string::npos;
        }));

        auto stringWithoutExtraSpaces =
            AZ::RHI::CommandLineArgumentUtils::RemoveExtraSpaces("  --arg1   -arg2     --arg3=foo --arg4=bar  ");
        EXPECT_EQ(stringWithoutExtraSpaces, AZStd::string("--arg1 -arg2 --arg3=foo --arg4=bar"));

        auto stringAsMergedArguments =
            AZ::RHI::CommandLineArgumentUtils::MergeCommandLineArguments("--arg1 -arg2 --arg3=foo", "--arg3=bar --arg4");
        EXPECT_EQ(stringAsMergedArguments, AZStd::string("--arg1 -arg2 --arg3=bar --arg4"));

        EXPECT_TRUE(AZ::RHI::CommandLineArgumentUtils::HasMacroDefinitions("-DMACRO"));
        EXPECT_TRUE(AZ::RHI::CommandLineArgumentUtils::HasMacroDefinitions("-D MACRO"));
        EXPECT_TRUE(AZ::RHI::CommandLineArgumentUtils::HasMacroDefinitions("--help -D MACRO"));
        EXPECT_TRUE(AZ::RHI::CommandLineArgumentUtils::HasMacroDefinitions("--help -p -DMACRO --more"));
        EXPECT_TRUE(AZ::RHI::CommandLineArgumentUtils::HasMacroDefinitions("--help -p -D MACRO=VALUE --more"));
        EXPECT_FALSE(AZ::RHI::CommandLineArgumentUtils::HasMacroDefinitions("--help -p --more"));
        EXPECT_FALSE(AZ::RHI::CommandLineArgumentUtils::HasMacroDefinitions("--help -p --more --DFAKE"));
        EXPECT_FALSE(AZ::RHI::CommandLineArgumentUtils::HasMacroDefinitions("--DFAKE1 --help -p --more --D FAKE2"));
    }

    TEST_F(SupervariantCmdArgumentTests, ShaderCompilerArguments_ValidateCommandLineArgumentsMerge)
    {
        // In this test we validate that AZ::RHI::ShaderCompilerArguments::Merge() works as expected
        // by merging AZSLC & DXC arguments giving higher priority to the arguments in the "right".

        auto shaderCompilerArgumentsLeft = CreateShaderCompilerArguments(
            "--azsl1 --azsl2=avalue2a -azsl3 --azsl4=avalue4a",
            "--dxc1=dvalue1a -dxc2 --dxc3=dvalue3a --dxc4");
        auto shaderCompilerArgumentsRight = CreateShaderCompilerArguments(
            "--azsl1 --azsl2=avalue2b -azsl3 --azsl4=avalue4a --azsl5",
            "--dxc1=dvalue1a -dxc2 --dxc3=dvalue3b --dxc4 --dxc5=dvalue5a");

        shaderCompilerArgumentsLeft.Merge(shaderCompilerArgumentsRight);
        EXPECT_EQ(shaderCompilerArgumentsLeft.m_azslcAdditionalFreeArguments, "--azsl1 --azsl2=avalue2b -azsl3 --azsl4=avalue4a --azsl5");
        EXPECT_EQ(shaderCompilerArgumentsLeft.m_dxcAdditionalFreeArguments, "--dxc1=dvalue1a -dxc2 --dxc3=dvalue3b --dxc4 --dxc5=dvalue5a");
    }


    TEST_F(SupervariantCmdArgumentTests, SupervariantInfo_ValidateMemberFunctions)
    {
        // In this test all member functions of the ShaderSourceData::SupervariantInfo class
        // are validated.

        AZStd::vector<KeyValueView> mcppMacrosList = {
            {MCPP_MACRO1, MCPP_VALUE1},
            {MCPP_MACRO2, MCPP_VALUE2},
            {MCPP_MACRO3, MCPP_VALUE3},
            {MCPP_MACRO4, ""},
        };

        AZStd::string argumentsToAddOrReplace;
        AppendMacroDefinitionArgument(argumentsToAddOrReplace, MCPP_MACRO3, MCPP_NEW_VALUE3);
        AppendCmdLineArgument(argumentsToAddOrReplace, AZSLC_ARG2, AZSLC_NEW_VAL2a);
        AppendMacroDefinitionArgument(argumentsToAddOrReplace, MCPP_MACRO1, MCPP_NEW_VALUE1);
        AppendCmdLineArgument(argumentsToAddOrReplace, AZSLC_ARG5, AZSLC_NEW_VAL5);
        AppendMacroDefinitionArgument(argumentsToAddOrReplace, MCPP_MACRO5, "");
        AppendCmdLineArgument(argumentsToAddOrReplace, AZSLC_ARG6, "");

        AZStd::string argumentsToRemove;
        AppendCmdLineArgument(argumentsToRemove, AZSLC_ARG3, "");
        AppendMacroDefinitionArgument(argumentsToRemove, MCPP_MACRO2, "");
        AppendCmdLineArgument(argumentsToRemove, AZSLC_ARG4, "");
        AppendMacroDefinitionArgument(argumentsToRemove, MCPP_MACRO4, "");

        auto supervariantInfo = CreateSupervariantInfo("Dummy", argumentsToAddOrReplace, argumentsToRemove);

        auto macroListToRemove = supervariantInfo.GetCombinedListOfMacroDefinitionNamesToRemove();
        AZStd::vector<AZStd::string> macroNamesToRemoveThatMustBePresent = { MCPP_MACRO1, MCPP_MACRO2, MCPP_MACRO3, MCPP_MACRO4, MCPP_MACRO5 };
        EXPECT_EQ(macroListToRemove.size(), macroNamesToRemoveThatMustBePresent.size());
        EXPECT_TRUE(
            VectorContainsAllSubstrings(macroListToRemove, macroNamesToRemoveThatMustBePresent)
        );

        auto macroListToAdd = supervariantInfo.GetMacroDefinitionsToAdd();
        AZStd::vector<AZStd::string> macroNamesToAddThatMustBePresent = {MCPP_MACRO1, MCPP_MACRO3, MCPP_MACRO5};
        EXPECT_EQ(macroListToAdd.size(), macroNamesToAddThatMustBePresent.size());
        EXPECT_TRUE(VectorContainsAllSubstrings(macroListToAdd, macroNamesToAddThatMustBePresent));

        // The result of GetCustomizedArgumentsForAzslc() is the most important value to test
        AZStd::vector<KeyValueView> freeAzslcArgumentList = {
            {AZSLC_ARG1, ""}, {AZSLC_ARG2, AZSLC_VAL2}, {AZSLC_ARG3, AZSLC_VAL3}, {AZSLC_ARG4, ""}, {AZSLC_ARG5, AZSLC_VAL5},
        };
        AZStd::string azslcArgs = CreateCmdLineStringFromListOfKeyValues(freeAzslcArgumentList);
        AZStd::string customizedAzslcArgs = supervariantInfo.GetCustomizedArgumentsForAzslc(azslcArgs);

        AZStd::vector<AZStd::string> stringsThatMustBePresent = {
            AZSLC_ARG1, AZSLC_ARG2, AZSLC_NEW_VAL2a, AZSLC_ARG5, AZSLC_NEW_VAL5, AZSLC_ARG6};
        EXPECT_TRUE(StringContainsAllSubstrings(customizedAzslcArgs, stringsThatMustBePresent));

        AZStd::vector<AZStd::string> stringsThatCanNotBePresent = { AZSLC_ARG3, AZSLC_VAL3, AZSLC_ARG4,
            // Because GetCustomizedArgumentsForAzslc() only returns arguments for AZSLc, none of the macro related
            // arguments can be present
            MCPP_MACRO1, MCPP_VALUE1, MCPP_NEW_VALUE1,
            MCPP_MACRO2, MCPP_VALUE2,
            MCPP_MACRO3, MCPP_VALUE3, MCPP_NEW_VALUE3,
            MCPP_MACRO4,
            MCPP_MACRO5
        };

        EXPECT_TRUE(
            StringDoesNotContainAnyOneOfTheSubstrings(customizedAzslcArgs, stringsThatCanNotBePresent)
        );
    }


    TEST_F(SupervariantCmdArgumentTests, ShaderAssetBuilder_ValidateInfluenceOfSupervariantInfoOnGlobalBuildOptions)
    {
        // In this test we validate how the ShaderAssetBuilder configure the commmand line arguments it passes
        // to MCPP, AZSLc & DXC. It basically starts with a GlobalBuildOptions, that gets further customized by
        // the ShaderCompilerArguments from ShaderSourceData(.shader file) and later further customized
        // by each SupervariantInfo in ShaderSourceData.

        // The first step is to define the initial values of the GlobalBuildOptions.
        AZStd::vector<KeyValueView> globalMcppMacrosList = {
            {MCPP_MACRO1, MCPP_VALUE1},
            {MCPP_MACRO2, MCPP_VALUE2},
            {MCPP_MACRO3, MCPP_VALUE3},
            {MCPP_MACRO4, ""},
        };

        AZStd::vector<KeyValueView> globalAzslArguments = {
            {AZSLC_ARG1, ""},
            {AZSLC_ARG2, AZSLC_VAL2},
            {AZSLC_ARG3, AZSLC_VAL3},
            {AZSLC_ARG4, ""},
            {AZSLC_ARG5, AZSLC_VAL5},
        };

        auto globalBuildOptions = CreateGlobalBuildOptions(
            AZStd::vector<AZStd::string>(), CreateListOfStringsFromListOfKeyValues(globalMcppMacrosList),
            CreateCmdLineStringFromListOfKeyValues(globalAzslArguments),
            "" /* Don't care about DXC in this test */);

        // The second step is to load the Shader Compiler Arguments from the .shader file.
        // These arguments will be merged in @globalBuildOptions, but the .shader arguments have
        // higher priority.
        AZStd::vector<KeyValueView> shaderAzslArguments = {
            {AZSLC_ARG2, AZSLC_NEW_VAL2a},
            {AZSLC_ARG6, ""},
        };
        auto shaderCompilerArguments = CreateShaderCompilerArguments(
            CreateCmdLineStringFromListOfKeyValues(shaderAzslArguments), "" /* Don't care about DXC in this test */);
        globalBuildOptions.m_compilerArguments.Merge(shaderCompilerArguments);

        // Let's create the dummy supervariant. It will have some MCPP & AZSLc arguments to be added/replaced AND other MCPP & AZSLc arguments to be removed.
        AZStd::vector<KeyValueView> supervariantAzslArgumentsToAdd = {
            {AZSLC_ARG2, AZSLC_NEW_VAL2b},
            {AZSLC_ARG7, ""},
        };
        AZStd::vector<KeyValueView> supervariantMacroDefinitionsToAdd = {
            {MCPP_MACRO1, MCPP_NEW_VALUE1},
            {MCPP_MACRO3, MCPP_NEW_VALUE3},
            {MCPP_MACRO5, ""},
        };
        auto supervariantArgumentsToAdd = CreateCmdLineStringFromListOfKeyValues(supervariantAzslArgumentsToAdd) +
            CreateMacroDefinitionCmdLineStringFromListOfKeyValues(supervariantMacroDefinitionsToAdd);

        AZStd::vector<KeyValueView> supervariantAzslArgumentsToRemove = {
            {AZSLC_ARG4, ""},
            {AZSLC_ARG1, ""},
        };
        AZStd::vector<KeyValueView> supervariantMacrosToRemove = { 
            {MCPP_MACRO2, ""},
            {MCPP_MACRO4, ""},
        };
        auto supervariantArgumentsToRemove = CreateCmdLineStringFromListOfKeyValues(supervariantAzslArgumentsToRemove) +
            CreateMacroDefinitionCmdLineStringFromListOfKeyValues(supervariantMacrosToRemove);

        //CreateMacroDefinitionCmdLineStringFromListOfKeyValues
        auto supervariantInfo = CreateSupervariantInfo("Dummy",
            supervariantArgumentsToAdd, // These arguments will be added or replace existing ones.
            supervariantArgumentsToRemove); // These arguments must be removed.

        AZStd::vector<AZStd::string> macroDefinitionNamesToRemove = supervariantInfo.GetCombinedListOfMacroDefinitionNamesToRemove();
        globalBuildOptions.m_preprocessorSettings.RemovePredefinedMacros(macroDefinitionNamesToRemove);
        AZStd::vector<AZStd::string> macroDefinitionsToAdd = supervariantInfo.GetMacroDefinitionsToAdd();
        globalBuildOptions.m_preprocessorSettings.m_predefinedMacros.insert(
            globalBuildOptions.m_preprocessorSettings.m_predefinedMacros.end(), macroDefinitionsToAdd.begin(), macroDefinitionsToAdd.end());

        // Validate macro definitions that must be present.
        EXPECT_TRUE(
            VectorContainsAllSubstrings(
                globalBuildOptions.m_preprocessorSettings.m_predefinedMacros,
                AZStd::vector<AZStd::string>({MCPP_MACRO1, MCPP_NEW_VALUE1, MCPP_MACRO3, MCPP_NEW_VALUE3, MCPP_MACRO5}))
        );

        // Validate macro definitions that can't be present.
        EXPECT_TRUE(
            VectorDoesNotContainAnyOneOfTheSubstrings(
                globalBuildOptions.m_preprocessorSettings.m_predefinedMacros,
                AZStd::vector<AZStd::string>({MCPP_MACRO2, MCPP_VALUE3, MCPP_MACRO4}))
        );

        AZStd::string azslcArgsFromGlobalBuildOptions = globalBuildOptions.m_compilerArguments.MakeAdditionalAzslcCommandLineString();

        // The result of GetCustomizedArgumentsForAzslc() is the most important value to test
        AZStd::string customizedAzslcArgs = supervariantInfo.GetCustomizedArgumentsForAzslc(azslcArgsFromGlobalBuildOptions);

        EXPECT_TRUE(
            StringContainsAllSubstrings(customizedAzslcArgs, CreateListOfSingleStringsFromListOfKeyValues(supervariantAzslArgumentsToAdd))
        );

        EXPECT_TRUE(
            StringDoesNotContainAnyOneOfTheSubstrings(customizedAzslcArgs, CreateListOfSingleStringsFromListOfKeyValues(supervariantAzslArgumentsToRemove))
        );

        EXPECT_TRUE(
            StringContainsAllSubstrings(customizedAzslcArgs, AZStd::vector<AZStd::string>({AZSLC_ARG3, AZSLC_VAL3, AZSLC_ARG5, AZSLC_VAL5}))
        );
    }


} //namespace UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

