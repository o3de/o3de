/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Memory/OSAllocator.h>

#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
#include <Atom/RHI.Edit/Utils.h>

#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/IO/FileOperations.h> // [GFX TODO] Remove when [ATOM-15472]
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzFramework/Process/ProcessCommunicator.h>
#include <AzFramework/Process/ProcessWatcher.h>

#include <AzslCompiler.h>
#include <CommonFiles/CommonTypes.h>
#include <ShaderBuilder_Traits_Platform.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        AZStd::any AsAny(const rapidjson::Value& value);

        static const char* ShaderCompilerName = "AZSL Compiler";            

        AzslCompiler::AzslCompiler(const AZStd::string& inputFilePath)
            : m_inputFilePath(inputFilePath)
        {
        }

        bool AzslCompiler::Compile(const AZStd::string& compilerParams, const AZStd::string& outputFilePath) const
        {
            // Shader compiler executable
            AZStd::string azslcRelativePath = "Builders/AZSLc/";
            azslcRelativePath += AZ_TRAIT_ATOM_SHADERBUILDER_AZSLC;

            // Compilation parameters
            AZStd::string azslcCommandOptions = AZStd::string::format("\"%s\"", m_inputFilePath.c_str());
            // NOTE: On macOS AZSLc executable fails if there is an extra space in the command line when there are no compiler parameters,
            // checking if there is no compiler parameters to avoid adding the extra space.
            if (!compilerParams.empty())
            {
                AZStd::vector<AZStd::string> tokenizedArguments;
                AzFramework::StringFunc::Tokenize(compilerParams, tokenizedArguments, " ");
                AZStd::string cleanParams;
                AzFramework::StringFunc::Join(cleanParams, tokenizedArguments.begin(), tokenizedArguments.end(), " ");
                azslcCommandOptions += AZStd::string::format(" %s", cleanParams.c_str());
            }

            if (!outputFilePath.empty())
            {
                azslcCommandOptions += AZStd::string::format(" -o \"%s\"", outputFilePath.c_str());
            }

            // Run Shader Compiler
            if (!RHI::ExecuteShaderCompiler(azslcRelativePath, azslcCommandOptions, m_inputFilePath, "AZSLc"))
            {
                return false;
            }

            return true;
        }

        bool AzslCompiler::EmitShader(AZ::IO::GenericStream& outputStream, const AZStd::string& compilerParams) const
        {
            // .azslin for input and .azslout for output (in the same folder)
            AZStd::string hlslOutputFile = m_inputFilePath;
            AzFramework::StringFunc::Path::ReplaceExtension(hlslOutputFile, "azslout");

            // Add the flag for the warning level
            const AZStd::string parameters = compilerParams;
            if (!Compile(parameters, hlslOutputFile))
            {
                return false;
            }

            AZ::IO::FileIOStream readAZSLOutput(hlslOutputFile.data(), AZ::IO::OpenMode::ModeRead);
                
            if (!readAZSLOutput.IsOpen())
            {
                AZ_Error(ShaderCompilerName, false, "Failed because the shader source file \"%s\" could not be opened %s", hlslOutputFile.data());
                return false;
            }

            if (!readAZSLOutput.CanRead())
            {
                AZ_Error(ShaderCompilerName, false, "Failed because the shader source file \"%s\" could not be read %s", hlslOutputFile.data());
                readAZSLOutput.Close();
                AZ::IO::SystemFile::Delete(hlslOutputFile.c_str());
                return false;
            }

            AZStd::string readBuffer;
            readBuffer.resize(readAZSLOutput.GetLength());
            readAZSLOutput.Read(readBuffer.size(), readBuffer.data());
            AZ_Assert(outputStream.CanWrite(), "Failed because the output stream for azslcout is not open for write!");
            outputStream.Write(readBuffer.size(), readBuffer.data());
            readAZSLOutput.Close();

            return true;
        }

        namespace SubProducts = ShaderBuilderUtility::AzslSubProducts;

        Outcome<SubProducts::Paths> AzslCompiler::EmitFullData(const AZStd::string& parameters, const AZStd::string& outputFile /* = ""*/) const
        {
            bool success = Compile("--full " + parameters, outputFile);
            if (!success)
            {
                return Failure();
            }
            // reconstruct the paths that has been created by azslc after a successful --full build, to inform this method's caller of the artifacts to look for:
            SubProducts::Paths productPaths = SubProducts::Paths(SubProducts::Paths::capacity());
            for (auto subProduct : SubProducts::SuffixListMembers)
            {
                AZStd::string subProductFilePath = outputFile.empty() ? m_inputFilePath : outputFile; // that's a reproduction of azslc's behavior (no "-o" = input name is used)
                AzFramework::StringFunc::Path::ReplaceExtension(subProductFilePath, subProduct.m_string.data());
                // append .json if it's one of those subs:
                auto listOfJsons = { SubProducts::ia, SubProducts::om, SubProducts::srg, SubProducts::options, SubProducts::bindingdep };
                subProductFilePath += AZStd::any_of(AZ_BEGIN_END(listOfJsons), [&](auto v) { return v == subProduct.m_value; }) ? ".json" : "";
                productPaths[subProduct.m_value] = subProductFilePath;
            }
            productPaths[SubProducts::azslin] = GetInputFilePath();  // post-fixup this one after the loop, because it's not an output of azslc, it's an output of the builder though.
            return { productPaths };
        }

        bool AzslCompiler::EmitInputAssembler(rapidjson::Document& output) const
        {
            return CompileToFileAndPrepareJsonDocument(output, "--ia", "ia.json") == BuildResult::Success;
        }

        bool AzslCompiler::ParseIaPopulateStructData(const rapidjson::Document& input, const AZStd::string& vertexEntryName, StructData& outStructData) const
        {
            AZStd::vector<StructParameter> structMembers;
            StructParameter inputStructParams;
            bool matchEntry = false;
            const rapidjson::Value& attributesLayout = input["inputLayouts"];
            AZ_Assert(attributesLayout.IsArray(), "Attribute inputLayouts is not an array");

            for (rapidjson::Value::ConstValueIterator itr = attributesLayout.Begin(); itr != attributesLayout.End(); ++itr)
            {
                const rapidjson::Value& attributeEntry = *itr;
                AZ_Assert(attributeEntry.IsObject(), "Attribute is not an object");
                if (attributeEntry.HasMember("entry") && (attributeEntry["entry"].GetString() == vertexEntryName))
                {
                    matchEntry = true;
                }
                if (attributeEntry.HasMember("streams") && matchEntry)
                {
                    const rapidjson::Value& streamArray = attributeEntry["streams"];
                    AZ_Assert(streamArray.IsArray(), "Attribute streams is not an array");

                    for (rapidjson::Value::ConstValueIterator itr3 = streamArray.Begin(); itr3 != streamArray.End(); ++itr3)
                    {
                        const rapidjson::Value& attributeMember = *itr3;
                        AZ_Assert(attributeMember.IsObject(), "Attribute is not an object");
                        for (rapidjson::Value::ConstMemberIterator itr2 = attributeMember.MemberBegin(); itr2 != attributeMember.MemberEnd(); ++itr2)
                        {

                            AZStd::string name = itr2->name.GetString();
                            const rapidjson::Value& value = itr2->value;
                            rapidjson::Type type = value.GetType();

                            switch (type)
                            {
                            case rapidjson::kStringType:
                                if (name == "baseType")
                                {
                                    inputStructParams.m_variable.m_type = StringToBaseType(itr2->value.GetString());
                                }
                                else if (name == "name")
                                {
                                    inputStructParams.m_variable.m_name = itr2->value.GetString();
                                }
                                else if (name == "semanticName")
                                {
                                    inputStructParams.m_semanticText = itr2->value.GetString();
                                }
                                break;

                            case rapidjson::kArrayType:
                                if (name == "dimensions")
                                {
                                    for (rapidjson::SizeType i = 0; i < value.Size(); i++)
                                    {
                                        ArrayItem arrayItem;
                                        arrayItem.m_count = value[i].GetInt();
                                        arrayItem.m_text = "";
                                        inputStructParams.m_variable.m_arrayDefinition.push_back(arrayItem);
                                    }
                                }
                                break;

                            case rapidjson::kNumberType:
                                if (name == "cols")
                                {
                                    inputStructParams.m_variable.m_cols = static_cast<uint8_t>(itr2->value.GetInt());
                                }
                                else if (name == "rows")
                                {
                                    inputStructParams.m_variable.m_rows = static_cast<uint8_t>(itr2->value.GetInt());
                                }
                                else if (name == "semanticIndex")
                                {
                                    inputStructParams.m_semanticIndex = itr2->value.GetInt();
                                    if (inputStructParams.m_semanticIndex > 0)
                                    {
                                        inputStructParams.m_semanticText = inputStructParams.m_semanticText + AZStd::to_string(inputStructParams.m_semanticIndex);
                                    }
                                }
                                break;

                            default:
                                break;
                            }
                        }
                        structMembers.push_back(inputStructParams);
                    }
                }
                if (matchEntry)
                {
                    break;
                }
            }
            if (!matchEntry)
            {
                return false;
            }
            outStructData.m_id = vertexEntryName;
            outStructData.m_members = structMembers;
            return true;
        }

        bool AzslCompiler::EmitOutputMerger(rapidjson::Document& output) const
        {
            return CompileToFileAndPrepareJsonDocument(output, "--om", "om.json") == BuildResult::Success;
        }

        bool AzslCompiler::ParseOmPopulateStructData(const rapidjson::Document& input, const AZStd::string& fragmentShaderName, StructData& outStructData) const
        {
            AZStd::vector<StructParameter> structMembers;
            StructParameter outputStructParams;
            bool matchEntry = false;
            const rapidjson::Value& attributesLayout = input["outputLayouts"];
            AZ_Assert(attributesLayout.IsArray(), "Attribute outputLayouts is not an array");

            for (rapidjson::Value::ConstValueIterator itr = attributesLayout.Begin(); itr != attributesLayout.End(); ++itr)
            {
                const rapidjson::Value& attributeEntry = *itr;
                AZ_Assert(attributeEntry.IsObject(), "Attribute is not an object");
                if (attributeEntry.HasMember("entry") && (attributeEntry["entry"].GetString() == fragmentShaderName))
                {
                    matchEntry = true;
                }
                if (attributeEntry.HasMember("renderTargets") && matchEntry)
                {
                    const rapidjson::Value& streamArray = attributeEntry["renderTargets"];
                    AZ_Assert(streamArray.IsArray(), "Attribute renderTargets is not an array");

                    for (rapidjson::Value::ConstValueIterator itr3 = streamArray.Begin(); itr3 != streamArray.End(); ++itr3)
                    {
                        const rapidjson::Value& attributeMember = *itr3;
                        AZ_Assert(attributeMember.IsObject(), "Attribute is not an object");
                        for (rapidjson::Value::ConstMemberIterator itr2 = attributeMember.MemberBegin(); itr2 != attributeMember.MemberEnd(); ++itr2)
                        {
                            AZStd::string name = itr2->name.GetString();
                            const rapidjson::Value& value = itr2->value;
                            rapidjson::Type type = value.GetType();

                            switch (type)
                            {
                            case rapidjson::kStringType:
                                if (name == "baseType")
                                {
                                    outputStructParams.m_variable.m_type = StringToBaseType(itr2->value.GetString());
                                }
                                else if (name == "semanticName")
                                {
                                    outputStructParams.m_semanticText = itr2->value.GetString();
                                }
                                else if (name == "format")
                                {
                                    outputStructParams.m_format = StringToFormat(itr2->value.GetString());
                                }
                                break;

                            case rapidjson::kNumberType:
                                if (name == "cols")
                                {
                                    outputStructParams.m_variable.m_cols = static_cast<uint8_t>(itr2->value.GetInt());
                                }
                                else if (name == "semanticIndex")
                                {
                                    outputStructParams.m_semanticIndex = itr2->value.GetInt();
                                    if (outputStructParams.m_semanticIndex > 0)
                                    {
                                        outputStructParams.m_semanticText = outputStructParams.m_semanticText + AZStd::to_string(outputStructParams.m_semanticIndex);
                                    }
                                }
                                break;

                            default:
                                break;
                            }
                        }
                        structMembers.push_back(outputStructParams);
                    }
                }
                if (matchEntry)
                {
                    break;
                }
            }
            if (!matchEntry)
            {
                return false;
            }
            outStructData.m_id = fragmentShaderName;
            outStructData.m_members = structMembers;
            return true;
        }

        bool AzslCompiler::ParseIaPopulateFunctionData(const rapidjson::Document& input, AzslFunctions& functionData) const
        {
            AZStd::vector<StructParameter> structMembers;
            StructParameter inputStructParams;
            const rapidjson::Value& attributesLayout = input["inputLayouts"];
            AZ_Assert(attributesLayout.IsArray(), "Attribute inputLayouts renderTargets is not an array");

            for (rapidjson::Value::ConstValueIterator itr = attributesLayout.Begin(); itr != attributesLayout.End(); ++itr)
            {
                const rapidjson::Value& attributeEntry = *itr;
                AZ_Assert(attributeEntry.IsObject(), "Attribute is not an object");
                
                FunctionData functionEntry;

                if (!attributeEntry.HasMember("entry"))
                {
                    continue;
                }
                functionEntry.m_name = attributeEntry["entry"].GetString();

                if (attributeEntry.HasMember("streams"))
                {
                    const rapidjson::Value& streamArray = attributeEntry["streams"];
                    AZ_Assert(streamArray.IsArray(), "Attribute streams is not an array");
                    if (streamArray.Size() > 0)
                    {
                        functionEntry.m_hasShaderStageVaryings = true;
                    }                    
                }

                // Note: switch to using range-based for loop in RapidJSON v1.1.0 - for (const auto& attr : attributeEntry)
                for (auto attr = attributeEntry.MemberBegin(); attr != attributeEntry.MemberEnd(); ++attr)
                {
                    const auto& attrName = attr->name.GetString();
                    if (std::strcmp(attrName, "entry") == 0 || std::strcmp(attrName, "streams") == 0)
                    {
                        continue;
                    }

                    RHI::ShaderStageAttributeArguments argList;

                    if (attr->value.IsArray())
                    {
                        for (rapidjson::Value::ConstValueIterator argItr = attr->value.Begin(); argItr != attr->value.End(); ++argItr)
                        {
                            argList.push_back(AsAny(*argItr));
                        }
                    }

                    functionEntry.attributesList[Name{attrName}] = argList;
                }

                functionData.push_back(functionEntry);
            }

            if (functionData.empty())
            {
                AZ_Error(ShaderCompilerName, false, "AzslCompiler::EmitFunctionData The number of valid shader entry functions in %s was 0!", m_inputFilePath.c_str());
                return false;
            }

            return true;
        }

        bool AzslCompiler::ParseSrgPopulateRootConstantData(const rapidjson::Document& input, RootConstantData& rootConstantData) const
        {
            if (input.HasMember("RootConstantBuffer"))
            {
                const rapidjson::Value& rootConstantBufferValue = input["RootConstantBuffer"];
                AZ_Assert(rootConstantBufferValue.IsObject(), "RootConstantBuffer is not an object");
                for (rapidjson::Value::ConstMemberIterator itr = rootConstantBufferValue.MemberBegin(); itr != rootConstantBufferValue.MemberEnd(); ++itr)
                {
                    AZStd::string_view rootConstantBufferMemberName = itr->name.GetString();
                    const rapidjson::Value& rootConstantBufferMemberValue = itr->value;

                    if (rootConstantBufferMemberName == "bufferForRootConstants")
                    {
                        AZ_Assert(rootConstantBufferMemberValue.IsObject(), "bufferForRootConstants is not an object");

                        for (rapidjson::Value::ConstMemberIterator itr2 = rootConstantBufferMemberValue.MemberBegin(); itr2 != rootConstantBufferMemberValue.MemberEnd(); ++itr2)
                        {
                            AZStd::string_view bufferForRootConstantsName = itr2->name.GetString();
                            const rapidjson::Value& bufferForRootConstantsValue = itr2->value;

                            if (bufferForRootConstantsName == "id")
                            {
                                AZStd::string nameId = bufferForRootConstantsValue.GetString();
                                rootConstantData.m_bindingInfo.m_nameId = nameId;
                            }
                            else if (bufferForRootConstantsName == "index")
                            {
                                rootConstantData.m_bindingInfo.m_registerId = bufferForRootConstantsValue.GetInt();
                            }
                            else if (bufferForRootConstantsName == "space")
                            {
                                rootConstantData.m_bindingInfo.m_space = bufferForRootConstantsValue.GetInt();
                            }
                            else if (bufferForRootConstantsName == "sizeInBytes")
                            {
                                rootConstantData.m_bindingInfo.m_sizeInBytes = bufferForRootConstantsValue.GetInt();
                                AZ_Assert(rootConstantData.m_bindingInfo.m_sizeInBytes > 0, "Invalid constant buffer size %d", rootConstantData.m_bindingInfo.m_sizeInBytes);
                            }
                        }
                    }
                    else if (rootConstantBufferMemberName == "inputsForRootConstants")
                    {
                        AZ_Assert(rootConstantBufferMemberValue.IsArray(), "inputsForRootConstants is not an array");

                        for (rapidjson::Value::ConstValueIterator itr2 = rootConstantBufferMemberValue.Begin(); itr2 != rootConstantBufferMemberValue.End(); ++itr2)
                        {
                            const rapidjson::Value& rootConstantBufferValue2 = *itr2;
                            AZ_Assert(rootConstantBufferValue2.IsObject(), "Entry in inputsForRootConstants is not an object");

                            SrgConstantData rootConstantInputs;

                            for (rapidjson::Value::ConstMemberIterator itr3 = rootConstantBufferValue2.MemberBegin(); itr3 != rootConstantBufferValue2.MemberEnd(); ++itr3)
                            {
                                AZStd::string rootConstantBufferMemberName2 = itr3->name.GetString();
                                const rapidjson::Value& rootConstantBufferMemberValue2 = itr3->value;

                                if (rootConstantBufferMemberName2 == "constantId")
                                {
                                    AZStd::string nameId = rootConstantBufferMemberValue2.GetString();
                                    rootConstantInputs.m_nameId = nameId;
                                }
                                else if (rootConstantBufferMemberName2 == "constantByteOffset")
                                {
                                    rootConstantInputs.m_constantByteOffset = rootConstantBufferMemberValue2.GetInt();
                                }
                                else if (rootConstantBufferMemberName2 == "constantByteSize")
                                {
                                    rootConstantInputs.m_constantByteSize = rootConstantBufferMemberValue2.GetInt();
                                }
                                else if (rootConstantBufferMemberName2 == "qualifiedName")
                                {
                                    rootConstantInputs.m_qualifiedName = rootConstantBufferMemberValue2.GetString();
                                }
                                else if (rootConstantBufferMemberName2 == "typeKind")
                                {
                                    rootConstantInputs.m_typeKind = rootConstantBufferMemberValue2.GetString();
                                }
                                else if (rootConstantBufferMemberName2 == "typeName")
                                {
                                    rootConstantInputs.m_typeName = rootConstantBufferMemberValue2.GetString();
                                }
                                else if (rootConstantBufferMemberName2 == "typeDimensions")
                                {
                                    AZ_Assert(rootConstantBufferMemberValue2.IsArray(), "typeDimensions is not an array");

                                    for (rapidjson::Value::ConstValueIterator itr4 = rootConstantBufferMemberValue2.Begin(); itr4 != rootConstantBufferMemberValue2.End(); ++itr4)
                                    {
                                        const rapidjson::Value& typeDimensionsValue = *itr4;

                                        ArrayItem arrayItem;
                                        arrayItem.m_count = typeDimensionsValue.GetUint();
                                        rootConstantInputs.m_typeDimensions.push_back(arrayItem);
                                    }
                                }
                            }

                            rootConstantData.m_constants.push_back(rootConstantInputs);
                        }
                    }
                }
            }

            return true;
        }

        const AZStd::string& AzslCompiler::GetInputFilePath() const
        {
            return m_inputFilePath;
        }

        bool AzslCompiler::EmitSrgData(rapidjson::Document& output, const AZStd::string& extraCompilerParams) const
        {
            AZStd::string parameters = AZStd::string::format("--srg %s", extraCompilerParams.c_str());
            return CompileToFileAndPrepareJsonDocument(output, parameters.c_str(), "srg.json") == BuildResult::Success;
        }

        bool AzslCompiler::ParseSrgPopulateSrgData(const rapidjson::Document& input, SrgDataContainer& outSrgData) const
        {
            const rapidjson::Value& shaderResourceGroups = input["ShaderResourceGroups"];
            AZ_Assert(shaderResourceGroups.IsArray(), "Attribute ShaderResourceGroups is not an array");

            for (rapidjson::Value::ConstValueIterator itr = shaderResourceGroups.Begin(); itr != shaderResourceGroups.End(); ++itr)
            {
                const rapidjson::Value& srgEntry = *itr;
                AZ_Assert(srgEntry.IsObject(), "Value is not an object");

                SrgData srgData;

                for (rapidjson::Value::ConstMemberIterator itr2 = srgEntry.MemberBegin(); itr2 != srgEntry.MemberEnd(); ++itr2)
                {
                    AZStd::string attributeName = itr2->name.GetString();
                    const rapidjson::Value& subArray = itr2->value;

                    if (attributeName == "bindingSlot")
                    {
                        srgData.m_bindingSlot.m_index = itr2->value.GetInt();
                    }
                    else if (attributeName == "id")
                    {
                        srgData.m_name = itr2->value.GetString();
                    }
                    else if (attributeName == "fallbackName")
                    {
                        srgData.m_fallbackName = itr2->value.GetString();
                    }
                    else if (attributeName == "fallbackSize")
                    {
                        srgData.m_fallbackSize = itr2->value.GetInt();
                    }
                    if (attributeName == "originalFileName")
                    {
                        srgData.m_containingFileName = itr2->value.GetString();
                    }
                    else if (attributeName == "inputsForImageViews")
                    {
                        AZ_Assert(subArray.IsArray(), "Object inputsForImageViews is not an array");
                        for (rapidjson::Value::ConstValueIterator itr3 = subArray.Begin(); itr3 != subArray.End(); ++itr3)
                        {
                            const rapidjson::Value& attributeArray = *itr3;
                            AZ_Assert(attributeArray.IsObject(), "Value is not an object");

                            TextureSrgData inputsForImageViews;

                            for (rapidjson::Value::ConstMemberIterator itr4 = attributeArray.MemberBegin(); itr4 != attributeArray.MemberEnd(); ++itr4)
                            {
                                AZStd::string attributeArrayMemberName = itr4->name.GetString();

                                if (attributeArrayMemberName == "count")
                                {
                                    inputsForImageViews.m_count = itr4->value.GetInt();
                                }
                                else if (attributeArrayMemberName == "id")
                                {
                                    inputsForImageViews.m_nameId = itr4->value.GetString();
                                }
                                else if (attributeArrayMemberName == "type")
                                {
                                    inputsForImageViews.m_type = StringToTextureType(itr4->value.GetString());
                                }
                                else if (attributeArrayMemberName == "usage")
                                {
                                    if(AzFramework::StringFunc::Equal(itr4->value.GetString(), "Read"))
                                    {
                                        inputsForImageViews.m_isReadOnlyType = true;
                                    }
                                    else if (AzFramework::StringFunc::Equal(itr4->value.GetString(), "ReadWrite"))
                                    {
                                        inputsForImageViews.m_isReadOnlyType = false;
                                    }
                                    else
                                    {
                                        AZ_Assert(false, "%s is an unexpected usage type", itr4->value.GetString());
                                    }
                                }
                                else if (attributeArrayMemberName == "index")
                                {
                                    inputsForImageViews.m_registerId = itr4->value.GetInt();
                                }
                            }
                            srgData.m_textures.push_back(inputsForImageViews);
                        }
                    }
                    else if (attributeName == "inputsForSamplers")
                    {
                        AZ_Assert(subArray.IsArray(), "Object inputsForSamplers is not an array");
                        for (rapidjson::Value::ConstValueIterator itr3 = subArray.Begin(); itr3 != subArray.End(); ++itr3)
                        {
                            const rapidjson::Value& attributeArray = *itr3;
                            AZ_Assert(attributeArray.IsObject(), "Value is not an object");

                            SamplerSrgData sampler;
                            AZ::RHI::SamplerState samplerStateDesc;

                            for (rapidjson::Value::ConstMemberIterator itr4 = attributeArray.MemberBegin(); itr4 != attributeArray.MemberEnd(); ++itr4)
                            {
                                const char* attributeArrayMemberName = itr4->name.GetString();

                                if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "addressU"))
                                {
                                    samplerStateDesc.m_addressU = StringToTextureAddressMode(itr4->value.GetString());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "addressV"))
                                {
                                    samplerStateDesc.m_addressV = StringToTextureAddressMode(itr4->value.GetString());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "addressW"))
                                {
                                    samplerStateDesc.m_addressW = StringToTextureAddressMode(itr4->value.GetString());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "anisotropyEnable"))
                                {
                                    samplerStateDesc.m_anisotropyEnable = itr4->value.GetBool();
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "anisotropyMax"))
                                {
                                    samplerStateDesc.m_anisotropyMax = itr4->value.GetInt();
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "borderColor"))
                                {
                                    samplerStateDesc.m_borderColor = StringToTextureBorderColor(itr4->value.GetString());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "comparisonFunc"))
                                {
                                    samplerStateDesc.m_comparisonFunc = StringToComparisonFunc(itr4->value.GetString());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "filterMag"))
                                {
                                    samplerStateDesc.m_filterMag = StringToFilterMode(itr4->value.GetString());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "filterMin"))
                                {
                                    samplerStateDesc.m_filterMin = StringToFilterMode(itr4->value.GetString());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "filterMip"))
                                {
                                    samplerStateDesc.m_filterMip = StringToFilterMode(itr4->value.GetString());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "mipLodBias"))
                                {
                                    samplerStateDesc.m_mipLodBias = aznumeric_caster(itr4->value.GetDouble());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "mipLodMax"))
                                {
                                    samplerStateDesc.m_mipLodMax = aznumeric_caster(itr4->value.GetDouble());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "mipLodMin"))
                                {
                                    samplerStateDesc.m_mipLodMin = aznumeric_caster(itr4->value.GetDouble());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "reductionType"))
                                {
                                    samplerStateDesc.m_reductionType = StringToReductionType(itr4->value.GetString());
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "id"))
                                {
                                    sampler.m_nameId = itr4->value.GetString();
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "isDynamic"))
                                {
                                    sampler.m_isDynamic = itr4->value.GetBool();
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "count"))
                                {
                                    sampler.m_count = itr4->value.GetInt();
                                }
                                else if (AzFramework::StringFunc::Equal(attributeArrayMemberName, "index"))
                                {
                                    sampler.m_registerId = itr4->value.GetInt();
                                }
                            }
                            sampler.m_descriptor = samplerStateDesc;
                            srgData.m_samplers.push_back(sampler);
                        }
                    }
                    else if (attributeName == "inputsForBufferViews")
                    {
                        AZ_Assert(subArray.IsArray(), "Object inputsForBufferViews is not an array");
                        for (rapidjson::Value::ConstValueIterator itr3 = subArray.Begin(); itr3 != subArray.End(); ++itr3)
                        {
                            const rapidjson::Value& attributeArray = *itr3;
                            AZ_Assert(attributeArray.IsObject(), "Value is not an object");

                            BufferSrgData buffer;
                            ConstantBufferData constantBuffer;

                            bool isConstantBuffer = false;
                            if(AzFramework::StringFunc::StartsWith(attributeArray["type"].GetString(), "ConstantBuffer"))
                            {
                                isConstantBuffer = true;
                            }

                            if (isConstantBuffer)
                            {
                                for (rapidjson::Value::ConstMemberIterator itr4 = attributeArray.MemberBegin(); itr4 != attributeArray.MemberEnd(); ++itr4)
                                {
                                    AZStd::string attributeArrayMemberName = itr4->name.GetString();

                                    if (attributeArrayMemberName == "count")
                                    {
                                        uint32_t count = itr4->value.GetInt();
                                        AZ_Assert(count == 1, "Invalid constant buffer count %d", count);

                                        constantBuffer.m_count = count;
                                    }
                                    else if (attributeArrayMemberName == "id")
                                    {
                                        constantBuffer.m_nameId = itr4->value.GetString();
                                    }
                                    else if (attributeArrayMemberName == "stride")
                                    {
                                        constantBuffer.m_strideSize = itr4->value.GetInt();
                                    }
                                    else if (attributeArrayMemberName == "index")
                                    {
                                        constantBuffer.m_registerId = itr4->value.GetInt();
                                    }
                                }
                                srgData.m_constantBuffers.push_back(constantBuffer);
                            }
                            else
                            {
                                for (rapidjson::Value::ConstMemberIterator itr4 = attributeArray.MemberBegin(); itr4 != attributeArray.MemberEnd(); ++itr4)
                                {
                                    AZStd::string attributeArrayMemberName = itr4->name.GetString();

                                    if (attributeArrayMemberName == "count")
                                    {
                                        uint32_t count = itr4->value.GetInt();
                                        AZ_Assert(count == 1, "Invalid buffer count %d", count);
                                        buffer.m_count = count;
                                    }
                                    else if (attributeArrayMemberName == "id")
                                    {
                                        buffer.m_nameId = itr4->value.GetString();
                                    }
                                    else if (attributeArrayMemberName == "type")
                                    {
                                        buffer.m_type = StringToBufferType(itr4->value.GetString());
                                    }
                                    else if (attributeArrayMemberName == "usage")
                                    {
                                        if (AzFramework::StringFunc::Equal(itr4->value.GetString(), "Read"))
                                        {
                                            buffer.m_isReadOnlyType = true;
                                        }
                                        else if (AzFramework::StringFunc::Equal(itr4->value.GetString(), "ReadWrite"))
                                        {
                                            buffer.m_isReadOnlyType = false;
                                        }
                                        else
                                        {
                                            AZ_Assert(false, "%s is an unexpected usage type", itr4->value.GetString());
                                        }
                                    }
                                    else if (attributeArrayMemberName == "stride")
                                    {
                                        buffer.m_strideSize = itr4->value.GetInt();
                                    }
                                    else if (attributeArrayMemberName == "index")
                                    {
                                        buffer.m_registerId = itr4->value.GetInt();
                                    }
                                }
                                srgData.m_buffers.push_back(buffer);
                            }
                        }
                    }
                    else if (attributeName == "inputsForSRGConstants")
                    {
                        AZ_Assert(subArray.IsArray(), "Object inputsForSRGConstants is not an array");
                        for (rapidjson::Value::ConstValueIterator itr3 = subArray.Begin(); itr3 != subArray.End(); ++itr3)
                        {
                            const rapidjson::Value& attributeArray = *itr3;
                            AZ_Assert(attributeArray.IsObject(), "Value is not an object");

                            SrgConstantData srgConstants;

                            for (rapidjson::Value::ConstMemberIterator itr4 = attributeArray.MemberBegin(); itr4 != attributeArray.MemberEnd(); ++itr4)
                            {
                                AZStd::string attributeArrayMemberName = itr4->name.GetString();
                                const rapidjson::Value& attributeArrayMemberValue = itr4->value;

                                if (attributeArrayMemberName == "constantByteOffset")
                                {
                                    srgConstants.m_constantByteOffset = itr4->value.GetInt();
                                }
                                else if (attributeArrayMemberName == "constantByteSize")
                                {
                                    srgConstants.m_constantByteSize = itr4->value.GetInt();
                                }
                                else if (attributeArrayMemberName == "constantId")
                                {
                                    srgConstants.m_nameId = itr4->value.GetString();
                                }
                                else if (attributeArrayMemberName == "qualifiedName")   // This is a meta data
                                {
                                    srgConstants.m_qualifiedName = itr4->value.GetString();
                                }
                                else if (attributeArrayMemberName == "typeKind")        // This is a meta data
                                {
                                    srgConstants.m_typeKind = itr4->value.GetString();
                                }
                                else if (attributeArrayMemberName == "typeName")        // This is a meta data
                                {
                                    srgConstants.m_typeName = itr4->value.GetString();
                                }
                                else if (attributeArrayMemberName == "typeDimensions")  // This is a meta data
                                {
                                    for (rapidjson::SizeType i = 0; i < attributeArrayMemberValue.Size(); i++)
                                    {
                                        ArrayItem arrayItem;
                                        arrayItem.m_count = attributeArrayMemberValue[i].GetInt();
                                        arrayItem.m_text = "";
                                        srgConstants.m_typeDimensions.push_back(arrayItem);
                                    }
                                }
                            }
                            srgData.m_srgConstantData.push_back(srgConstants);
                        }
                    }
                    else if (attributeName == "bufferForSRGConstants")
                    {
                        for (rapidjson::Value::ConstMemberIterator itr3 = subArray.MemberBegin(); itr3 != subArray.MemberEnd(); ++itr3)
                        {
                            AZStd::string attributeArrayMemberName = itr3->name.GetString();
                            if (attributeArrayMemberName == "index")
                            {
                                srgData.m_srgConstantDataRegisterId = itr3->value.GetInt();
                            }
                            // The logical space attribute ("space") is also available when using the --use-spaces argument.
                        }
                    }
                }
                outSrgData.push_back(srgData);
            }
            return true;
        }

        bool AzslCompiler::EmitOptionsList(rapidjson::Document& output) const
        {
            return CompileToFileAndPrepareJsonDocument(output, "--options", "options.json") == BuildResult::Success;
        }

        bool AzslCompiler::ParseOptionsPopulateOptionGroupLayout(const rapidjson::Document& input, RPI::Ptr<RPI::ShaderOptionGroupLayout>& shaderOptionGroupLayout) const
        {
            auto totalBitOffset = (uint32_t) 0u;

            auto onSuccess = [&shaderOptionGroupLayout, &totalBitOffset]() 
                {
                    if (totalBitOffset == 0)
                    {
                        AZStd::vector<RPI::ShaderOptionValuePair> idIndexList;
                        idIndexList.push_back( { Name("false"),    RPI::ShaderOptionValue(0) } );
                        idIndexList.push_back( { Name("true"),     RPI::ShaderOptionValue(1) } );

                        RPI::ShaderOptionDescriptor shaderOption(Name("DefaultOption"), 
                                                                 RPI::ShaderOptionType::Boolean,
                                                                 totalBitOffset,
                                                                 0,
                                                                 idIndexList,
                                                                 Name("false"));

                        shaderOptionGroupLayout->AddShaderOption(shaderOption);
                        totalBitOffset += shaderOption.GetBitCount();
                    }

                    shaderOptionGroupLayout->Finalize();
                    return true;
                };

            auto onFail = [&shaderOptionGroupLayout]() 
                {
                    shaderOptionGroupLayout->Finalize();
                    return false;
                };

            const rapidjson::Value& shaderOptions = input["ShaderOptions"];
            AZ_Assert(shaderOptions.IsArray(), "Attribute ShaderOptions must be an array");

            uint32_t explicitlyOrdered = 0;
            uint32_t implicitlyOrdered = 0;
            for (rapidjson::Value::ConstValueIterator optItr = shaderOptions.Begin(); optItr != shaderOptions.End(); ++optItr)
            {
                const rapidjson::Value& optionEntry = *optItr;
                AZ_Assert(optionEntry.IsObject(), "Expected option entry to be an object!");

                Name defaultValueId = optionEntry.HasMember("defaultValue") ? Name(optionEntry["defaultValue"].GetString()) : Name();
                const AZStd::string optionName             = optionEntry.HasMember("name")         ? optionEntry["name"].GetString()                 : "";
                [[maybe_unused]] const bool valuesAreRange = optionEntry.HasMember("range")        ? optionEntry["range"].GetBool()                  : false;
                const bool isPredefinedType                = optionEntry.HasMember("kind")         ? AzFramework::StringFunc::Equal(optionEntry["kind"].GetString(), "predefined") : false;

                auto optionType = RPI::ShaderOptionType::Unknown;
                if (isPredefinedType && optionEntry.HasMember("type"))
                {   // Bool or int
                    const auto typeName = AZStd::string_view(optionEntry["type"].GetString());
                    if (typeName.find("bool") != AZStd::string::npos)
                    {
                        optionType = RPI::ShaderOptionType::Boolean;
                    }
                    else if (typeName.find("int") != AZStd::string::npos)
                    {
                        AZ_Assert(valuesAreRange, "Integer options must have a range!");
                        optionType = RPI::ShaderOptionType::IntegerRange;
                    }
                }
                else
                {   // We don't support complex structures for options yet, so the only user-defined type is an enumeration.
                    optionType = RPI::ShaderOptionType::Enumeration;
                }

                AZStd::vector<RPI::ShaderOptionValuePair> idIndexList;
                if (optionEntry.HasMember("values"))
                {
                    uint32_t optIndex = 0;
                    for (rapidjson::Value::ConstValueIterator valItr = optionEntry["values"].Begin(); valItr != optionEntry["values"].End(); ++valItr)
                    {

                        if (optionType == RPI::ShaderOptionType::IntegerRange)
                        {
                            auto intValue = atoi(valItr->GetString());
                            idIndexList.push_back( { Name(valItr->GetString()), RPI::ShaderOptionValue(intValue)   } );
                        }
                        else
                        {
                            idIndexList.push_back( { Name(valItr->GetString()), RPI::ShaderOptionValue(optIndex++) } );
                        }
                    }
                }

                if (optionName.empty())
                {
                    AZ_Error(ShaderCompilerName, false, "New option in file '%s' must specify an option name!", m_inputFilePath.c_str());
                    return onFail();
                }

                if (idIndexList.empty())
                {
                    AZ_Error(ShaderCompilerName, false, "Option '%s' must have at least one value!", optionName.c_str());
                    return onFail();
                }

                if (optionType == RPI::ShaderOptionType::IntegerRange && idIndexList.size() != 2)
                {
                    AZ_Error(ShaderCompilerName, false, "Option '%s' is marked as integer range and must provide exactly two values [min, max]!", optionName.c_str());
                    return onFail();
                }

                if (defaultValueId.IsEmpty())
                {
                    defaultValueId = idIndexList[0].first;
                    AZ_Printf(ShaderCompilerName, "Option {%s} doesn't provide a default value, using {%s} instead.", optionName.c_str(), defaultValueId.GetCStr());
                }

                {   // Unnamed scope to limit the use of shaderOption
                    if (!optionEntry.HasMember("keyOffset") || !optionEntry.HasMember("keySize"))
                    {
                        AZ_Error(ShaderCompilerName, false, "Option {%s} must specify keyOffset and keySize. You might want to bump your AZSLc to ver.", optionName.c_str());
                        return onFail();
                    }
                    const uint32_t keyOffset = optionEntry["keyOffset"].GetUint();
                    const uint32_t keySize   = optionEntry["keySize"].GetUint();

                    uint32_t order;
                    if (optionEntry.HasMember("order"))
                    {
                        order = optionEntry["order"].GetUint();                        
                        explicitlyOrdered++;
                    }
                    else
                    {
                        order = implicitlyOrdered;                        
                        implicitlyOrdered++;
                    }

                    RPI::ShaderOptionDescriptor shaderOption(Name(optionName), 
                                                             optionType,
                                                             keyOffset,
                                                             order,
                                                             idIndexList,
                                                             defaultValueId);

                    if (!shaderOptionGroupLayout->AddShaderOption(shaderOption))
                    {
                        // AddShaderOption will report error messages
                        return onFail();
                    }

                    totalBitOffset = keyOffset + keySize;

                    if (keySize != shaderOption.GetBitCount())
                    {
                        AZ_Error(ShaderCompilerName, false, "Option {%} specifies a different bit size than calculated!", optionName.c_str());
                        return onFail();
                    }
                }
            }

            if (explicitlyOrdered > 0 && implicitlyOrdered > 0)
            {
                AZ_Error(ShaderCompilerName, false, "Either all or none of the options must have the \"order\" attribute defined. It's not allowed to add it to some options and not to others.");
                return onFail();                
            }

            return onSuccess();
        }

        bool AzslCompiler::EmitBindingDependencies(rapidjson::Document& output) const
        {
            return PrepareJsonDocument(output, "bindingdep.json") == BuildResult::Success;
        }

        bool AzslCompiler::ParseBindingdepPopulateBindingDependencies(const rapidjson::Document& input, BindingDependencies& bindingDependencies) const
        {
            for (rapidjson::Value::ConstMemberIterator itr = input.MemberBegin(); itr != input.MemberEnd(); ++itr)
            {
                AZStd::string_view srgKey = itr->name.GetString();
                const rapidjson::Value& srgValue = itr->value;
                AZ_Assert(srgValue.IsObject(), "Root is not an object");

                BindingDependencies::SrgResources srg;

                for (rapidjson::Value::ConstMemberIterator itr2 = srgValue.MemberBegin(); itr2 != srgValue.MemberEnd(); ++itr2)
                {
                    AZStd::string_view resourceKey = itr2->name.GetString();
                    const rapidjson::Value& srgEntryValue = itr2->value;
                    AZ_Assert(srgEntryValue.IsObject(), "Value in SRG is not an object");

                    BindingDependencies::Resource* binding = nullptr;

                    for (rapidjson::Value::ConstMemberIterator itr3 = srgEntryValue.MemberBegin(); itr3 != srgEntryValue.MemberEnd(); ++itr3)
                    {
                        AZStd::string_view srgMemberName = itr3->name.GetString();
                        const rapidjson::Value& srgMemberValue = itr3->value;

                        if (srgMemberName == "binding")
                        {
                            AZ_Assert(srgMemberValue.IsObject(), "binding is not an object");

                            BindingDependencies::Register registerId = ~static_cast<BindingDependencies::Register>(0x0);
                            BindingDependencies::Register registerSpace = ~static_cast<BindingDependencies::Register>(0x0);
                            uint32_t registerSpan = 0;

                            for (rapidjson::Value::ConstMemberIterator itr4 = srgMemberValue.MemberBegin(); itr4 != srgMemberValue.MemberEnd(); ++itr4)
                            {
                                AZStd::string bindingName = itr4->name.GetString();
                                const rapidjson::Value& bindingValue = itr4->value;

                                if (bindingName == "type")
                                {
                                    AZStd::string type = bindingValue.GetString();

                                    binding = type == "SrgConstantCB" ? &srg.m_srgConstantsDependencies.m_binding
                                        : &srg.m_resources[resourceKey];

                                    binding->m_type = type;
                                    binding->m_selfName = resourceKey;
                                }
                                else if (bindingName == "index-merged")
                                {
                                    registerId = bindingValue.GetUint();
                                }
                                else if (bindingName == "range")
                                {
                                    registerSpan = bindingValue.GetUint();
                                }
                                else if (bindingName == "space-merged")
                                {
                                    registerSpace = bindingValue.GetUint();
                                }
                            }

                            AZ_Assert(binding, "binding type is not defined");

                            binding->m_registerId = registerId;
                            binding->m_registerSpan = registerSpan;
                            // [ATOM-5914] The registerSpace should be at the SRG level not per resource.
                            srg.m_registerSpace = registerSpace;
                        }
                        else if (srgMemberName == "dependentFunctions")
                        {
                            AZ_Assert(srgMemberValue.IsArray(), "dependentFunctions is not an array");
                            AZ_Assert(binding, "binding is not defined");

                            for (rapidjson::Value::ConstValueIterator itr4 = srgMemberValue.Begin(); itr4 != srgMemberValue.End(); ++itr4)
                            {
                                const rapidjson::Value& dependentFunctionsValue = *itr4;

                                binding->m_dependentFunctions.emplace_back(dependentFunctionsValue.GetString());
                            }
                        }
                        else if (srgMemberName == "participantConstants")
                        {
                            AZ_Assert(srgMemberValue.IsArray(), "participantConstants is not an array");

                            for (rapidjson::Value::ConstValueIterator itr4 = srgMemberValue.Begin(); itr4 != srgMemberValue.End(); ++itr4)
                            {
                                const rapidjson::Value& participantConstantsValue = *itr4;

                                srg.m_srgConstantsDependencies.m_partipicantConstants.emplace_back(participantConstantsValue.GetString());
                            }
                        }
                    }
                }

                bindingDependencies.m_orderedSrgs.push_back(srg);
                bindingDependencies.m_srgNameToVectorIndex[srgKey] = aznumeric_cast<int>(bindingDependencies.m_orderedSrgs.size()) - 1;
            }

            return true;
        }
        
        AzslCompiler::BuildResult AzslCompiler::CompileToFileAndPrepareJsonDocument(
            rapidjson::Document& outputJson,
            const char* compilerCommandSwitch,
            const char* outputExtension,
            AfterRead deleteOutputFileAfterReading /*= AfterRead::Keep*/) const
        {
            // Emitted output filename and path is same as input file, but extension is .json
            AZStd::string outputFile = m_inputFilePath;
            AzFramework::StringFunc::Path::ReplaceExtension(outputFile, outputExtension);

            AZ_Error("AzslCompiler", AZ::IO::SystemFile::Exists(outputFile.c_str()), "Destination file %s, exists. will be overwritten", outputFile.c_str());

            if (!Compile(compilerCommandSwitch, outputFile))
            {
                return BuildResult::CompilationFailed;
            }

            auto readJsonResult = JsonSerializationUtils::ReadJsonFile(outputFile, AZ::RPI::JsonUtils::DefaultMaxFileSize);

            if (readJsonResult.IsSuccess())
            {
                outputJson = readJsonResult.TakeValue();
                if (deleteOutputFileAfterReading == AfterRead::Delete)
                {
                    AZ::IO::SystemFile::Delete(outputFile.c_str());  // Delete the .json file after reading
                }
            }
            return BuildResult::Success;
        }

        AzslCompiler::BuildResult AzslCompiler::PrepareJsonDocument(
            rapidjson::Document& outputJson,
            const char* outputExtension) const
        {
            // Emitted output filename and path is same as input file, but extension is .json
            AZStd::string outputFile = m_inputFilePath;
            AzFramework::StringFunc::Path::ReplaceExtension(outputFile, outputExtension);

            auto readJsonResult = JsonSerializationUtils::ReadJsonFile(outputFile, AZ::RPI::JsonUtils::DefaultMaxFileSize);

            if (readJsonResult.IsSuccess())
            {
                outputJson = readJsonResult.TakeValue();
                return BuildResult::Success;
            }

            return BuildResult::JsonReadbackFailed;
        }

        AZStd::any AsAny(const rapidjson::Value& value)
        {
            if (value.IsBool())
            {
                return AZStd::any{ value.GetBool() };
            }
            else if (value.IsInt())
            {
                return AZStd::any{ value.GetInt() };
            }
            else if (value.IsDouble())
            {
                return AZStd::any{ value.GetDouble() };
            }
            else if (value.IsString())
            {
                return AZStd::any{ value.GetString() };
            }

            // Not any type we recognize
            AZ_Error(ShaderBuilder::ShaderCompilerName, false, "Unrecognized argument type!");

            // Return empty element to prevent compiler warnings
            return AZStd::any{ };
        }

    } // namespace ShaderBuilder
} // namespace AZ
