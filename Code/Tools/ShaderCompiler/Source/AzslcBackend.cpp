/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzslcBackend.h"
#include "AzslcPlatformEmitter.h"

#include <tuple>
#include <cmath>

namespace AZ::ShaderCompiler
{
    bool IsReadWriteView(string_view viewName)
    {
        return (StartsWith(viewName, "RW") || StartsWith(viewName, "RasterizerOrdered") ||
                StartsWith(viewName, "Append") || StartsWith(viewName, "Consume"));
    }

    QualifiedName MakeSrgConstantsStructName(IdentifierUID srg)
    {
        return QualifiedName{srg.m_name + "_SRGConstantsStruct"};
    }

    QualifiedName MakeSrgConstantsCBName(IdentifierUID srg)
    {
        return QualifiedName{srg.m_name + "_SRGConstantBuffer"};
    }

    string UnmangleTrimedName(const QualifiedNameView name)
    {
        return Replace(string{Trim(name.data(), "/")}, "/", "::");
    }

    string JoinAllNestedNamesWithUnderscore(const QualifiedNameView name)
    {
        return Replace(UnmangleTrimedName(name), "::", "_");
    }

    string GetGlobalRootConstantVarName(const QualifiedNameView name)
    {
        return "_g_" + Replace(UnmangleTrimedName(name), "::", Underscore);
    }

    string GetShaderKeyFunctionName(const IdentifierUID& uid)
    {
        return "GetShaderVariantKey_" + JoinAllNestedNamesWithUnderscore(uid.m_name) + "()";
    }

    string GetRootConstFunctionName(const IdentifierUID& uid)
    {
        return "GetShaderRootConst_" + JoinAllNestedNamesWithUnderscore(uid.m_name) + "()";
    }

    // this doesn't go through translation because now it's not as risky as before considering the name is clear
    string UnmangleTrimedName(const ExtendedTypeInfo& extTypeInfo)
    {
        auto fullName = UnmangleTrimedName(extTypeInfo.m_coreType.m_typeId.m_name);
        if (HasGenericParameter(extTypeInfo.m_coreType.m_typeClass))
        {
            fullName += "<" + UnmangleTrimedName(extTypeInfo.m_genericParameter.m_typeId.m_name) + ">";
        }
        return fullName;
    }

    // Define emission for sampler states
    // Reference: https://github.com/Microsoft/DirectXShaderCompiler/blob/master/tools/clang/unittests/HLSL/FunctionTest.cpp

    Streamable& operator << (Streamable& out, const SamplerStateDesc::AddressMode& addressMode)
    {
        return out << ((addressMode == SamplerStateDesc::AddressMode::Wrap)   ? "TEXTURE_ADDRESS_WRAP"
                     : (addressMode == SamplerStateDesc::AddressMode::Clamp)  ? "TEXTURE_ADDRESS_CLAMP"
                     : (addressMode == SamplerStateDesc::AddressMode::Border) ? "TEXTURE_ADDRESS_BORDER"
                     : (addressMode == SamplerStateDesc::AddressMode::Mirror) ? "TEXTURE_ADDRESS_MIRROR"
                     :                                                          "TEXTURE_ADDRESS_MIRROR_ONCE");
    }

    Streamable& operator << (Streamable& out, const SamplerStateDesc::ComparisonFunc& compFunc)
    {
        return out << ((compFunc == SamplerStateDesc::ComparisonFunc::Never)        ? "COMPARISON_NEVER"
                     : (compFunc == SamplerStateDesc::ComparisonFunc::Less)         ? "COMPARISON_LESS"
                     : (compFunc == SamplerStateDesc::ComparisonFunc::Equal)        ? "COMPARISON_EQUAL"
                     : (compFunc == SamplerStateDesc::ComparisonFunc::LessEqual)    ? "COMPARISON_LESS_EQUAL"
                     : (compFunc == SamplerStateDesc::ComparisonFunc::Greater)      ? "COMPARISON_GREATER"
                     : (compFunc == SamplerStateDesc::ComparisonFunc::NotEqual)     ? "COMPARISON_NOT_EQUAL"
                     : (compFunc == SamplerStateDesc::ComparisonFunc::GreaterEqual) ? "COMPARISON_GREATER_EQUAL"
                     :                                                                "COMPARISON_ALWAYS");
    }

    Streamable& operator << (Streamable& out, const SamplerStateDesc::BorderColor& borderColor)
    {
        return out << ((borderColor == SamplerStateDesc::BorderColor::OpaqueBlack) ? "STATIC_BORDER_COLOR_OPAQUE_BLACK"
                     : (borderColor == SamplerStateDesc::BorderColor::OpaqueWhite) ? "STATIC_BORDER_COLOR_OPAQUE_WHITE"
                     :                                                               "STATIC_BORDER_COLOR_TRANSPARENT_BLACK");
    }

    Streamable& operator << (Streamable& out, const SamplerStateDesc& samplerDesc)
    {
        // Resolving the filter is the hardest part of the emission
        out << ", filter = FILTER_";

        out << ((samplerDesc.m_reductionType == SamplerStateDesc::ReductionType::Comparison) ? "COMPARISON_"
              : (samplerDesc.m_reductionType == SamplerStateDesc::ReductionType::Maximum)    ? "MAXIMUM_"
              : (samplerDesc.m_reductionType == SamplerStateDesc::ReductionType::Minimum)    ? "MINIMUM_"
              :                                                                                "");

        // The correct order is MIN -> MAG -> MIP
        // Reference: https://github.com/Microsoft/DirectXShaderCompiler/blob/master/tools/clang/unittests/HLSL/FunctionTest.cpp
        out << "MIN_"
            << (samplerDesc.m_filterMin == samplerDesc.m_filterMag ? "MAG_" : "")
            << (samplerDesc.m_filterMin == samplerDesc.m_filterMag && samplerDesc.m_filterMag == samplerDesc.m_filterMip ? "MIP_" : "")
            << (samplerDesc.m_filterMin == SamplerStateDesc::FilterMode::Point ? "POINT" : "LINEAR");

        if (samplerDesc.m_filterMin != samplerDesc.m_filterMag)
        {
            out << "_MAG_"
                << (samplerDesc.m_filterMag == samplerDesc.m_filterMip ? "MIP_" : "")
                << (samplerDesc.m_filterMag == SamplerStateDesc::FilterMode::Point ? "POINT" : "LINEAR");
        }

        if (samplerDesc.m_filterMag != samplerDesc.m_filterMip)
        {
            out << "_MIP_"
                << (samplerDesc.m_filterMip == SamplerStateDesc::FilterMode::Point ? "POINT" : "LINEAR");
        }
        // ^ The filter should be resolved

        return out << ", addressU = " << samplerDesc.m_addressU
                   << ", addressV = " << samplerDesc.m_addressV
                   << ", addressW = " << samplerDesc.m_addressW
                   << ", minLOD = " << samplerDesc.m_mipLodMin
                   << ", maxLOD = " << samplerDesc.m_mipLodMax
                   << ", mipLODBias = " << samplerDesc.m_mipLodBias
                   << ", BorderColor = " << samplerDesc.m_borderColor
                   << ", maxAnisotropy = " << samplerDesc.m_anisotropyMax
                   << ", ComparisonFunc  = " << samplerDesc.m_comparisonFunc;
    }

    // map root param type to register type
    BindingType RootParamTypeToBindingType(RootParamType paramType)
    {
        // the 2 enum orders are arranged the same for this to work
        return BindingType::EnumType( int(paramType) < int(BindingType::EndEnumeratorSentinel_) ? int(paramType) : BindingType::B );
    }

    RootParamType FindParamType(const ExtendedTypeInfo& typeInfoExt)
    {
        TypeClass coreTypeClass = typeInfoExt.m_coreType.m_typeClass;
        if (coreTypeClass == TypeClass::Sampler)
        {
            return RootParamType::Sampler;
        }
        if (coreTypeClass == TypeClass::ConstantBuffer)
        {
            return RootParamType::CBV;
        }
        if (IsViewType(coreTypeClass))
        {
            return IsReadWriteView(typeInfoExt.m_coreType.m_typeId.GetNameLeaf()) ? RootParamType::UAV : RootParamType::SRV;
        }
        return RootParamType::EndEnumeratorSentinel_;
    }

    BindingType GetBindingType(const ExtendedTypeInfo& extendedTypeInfo)
    {
        RootParamType paramType = FindParamType(extendedTypeInfo);
        BindingType bindingType = RootParamTypeToBindingType(paramType);
        return bindingType;
    }

    void SingleBindingLocationTracker::IncrementSpace()
    {
        for (auto bufferType : BindingType::Enumerate{})
        {
            m_accumulated[bufferType] += m_registerPos[bufferType];
            m_registerPos[bufferType] = 0;
        }
        ++m_space;
    }

    // Equalizes all register indices (important for platforms with no B,T,S,U distinction)
    void SingleBindingLocationTracker::UnifyIndices()
    {
        using Reg = BindingType;
        auto maxValue = std::max(
            std::max(m_registerPos[Reg::B], m_registerPos[Reg::T]),
            std::max(m_registerPos[Reg::S], m_registerPos[Reg::U]));

        for (auto bufferType : Reg::Enumerate{})
        {
            m_accumulatedUnused[bufferType] += maxValue - m_registerPos[bufferType];
            m_registerPos[bufferType] = maxValue;
        }
    }

    int SingleBindingLocationTracker::GetAccumulated(BindingType r) const
    {
        return (m_accumulated[r] - m_accumulatedUnused[r]) + m_registerPos[r];
    }

    void MultiBindingLocationMaker::SignalIncrementSpace(std::function<void(int, int)> warningMessageFunctionForMinDescOvershoot)
    {
        if (m_options.m_minAvailableDescriptors.m_spaces >= 0
            && m_untainted.m_space >= m_options.m_minAvailableDescriptors.m_spaces)
        {
            warningMessageFunctionForMinDescOvershoot(m_untainted.m_space, m_options.m_minAvailableDescriptors.m_spaces);
        }

        m_untainted.IncrementSpace();
        if (m_options.m_maxSpaces > m_merged.m_space + 1) // example: (1 > 0 + 1)  <=>  (1 > 1)  <=>  (false)  --> never increment space. stay at 0. which respects max-spaces=1
        {
            m_merged.IncrementSpace();
        }
    }

    void MultiBindingLocationMaker::SignalUnifyIndices()
    {
        if (m_options.m_useUniqueIndices)
        {
            m_untainted.UnifyIndices();
            m_merged.UnifyIndices();
        }
    }

    void MultiBindingLocationMaker::SignalIncrementRegister(BindingType regType, int count)
    {
        m_untainted.m_registerPos[regType] += count;
        m_merged.m_registerPos[regType] += count;
    }

    BindingPair MultiBindingLocationMaker::GetCurrent(BindingType regType)
    {
        BindingPair pair;
        pair.m_pair[BindingPair::Set::Merged   ].m_logicalSpace = m_merged.m_space;
        pair.m_pair[BindingPair::Set::Untainted].m_logicalSpace = m_untainted.m_space;
        pair.m_pair[BindingPair::Set::Merged   ].m_registerIndex = m_merged.m_registerPos[regType];
        pair.m_pair[BindingPair::Set::Untainted].m_registerIndex = m_untainted.m_registerPos[regType];
        return pair;
    }

    Streamable& operator << (Streamable& out, const SamplerStateDesc::ReductionType& redcType)
    {
        return out << ((redcType == SamplerStateDesc::ReductionType::Comparison)  ? "Comparison"
                     : (redcType == SamplerStateDesc::ReductionType::Filter)      ? "Filter"
                     : (redcType == SamplerStateDesc::ReductionType::Minimum)     ? "Minimum"
                     :                                                              "Maximum");
    }

    const PlatformEmitter& Backend::GetPlatformEmitter() const
    {
        return GetPlatformEmitter(m_ir);
    }

    //! Gets the next and increments tokenIndex. TokenIndex must be in the [misc::Interval.a, misc::Interval.b] range. Token cannot be nullptr.
    antlr4::Token* Backend::GetNextToken(ssize_t& tokenIndex, size_t channel) const
    {
        auto* token = m_tokens->get(tokenIndex++);
        assert(token); // Higher level logic must ensure tokens are in the [interval.a, interval.b] range. They cannot be null, this is a flaw in the code.

        while (token->getChannel() != channel)
        {
            token = m_tokens->get(tokenIndex++);
            assert(token); // same
        }

        return token;
    }

    void Backend::EmitTranspiledTokens(misc::Interval interval, Streamable& output) const
    {
        ssize_t ii = interval.a;
        while (ii <= interval.b)
        {
            auto* token = GetNextToken(ii /*inout*/);
            auto str = token->getText();
            output << str << ((str == ";" || str == "{") ? '\n' : ' ');
        }
    }

    string Backend::GetTranspiledTokens(misc::Interval interval) const
    {
        static std::stringstream ss;
        ss.str({});
        ss.clear();
        MakeOStreamStreamable soss(ss);
        EmitTranspiledTokens(interval, soss);
        return ss.str();
    }

    string Backend::GetInitializerClause(const VarInfo* varInfo) const
    {
        if (!varInfo->m_declNode ||
            !varInfo->m_declNode->variableInitializer() ||
            !varInfo->m_declNode->variableInitializer()->standardVariableInitializer())
        {
            return "";
        }

        auto* initClause = varInfo->m_declNode->variableInitializer()->standardVariableInitializer();
        return RemoveWhitespaces(GetTranspiledTokens(initClause->getSourceInterval()));
    }

    void Backend::AppendOptionRange(Json::Value& varOption, const IdentifierUID& varUid, const VarInfo* varInfo, const Options& options) const
    {
        Json::Value optValues(Json::arrayValue);
        bool isRange = false;
        uint32_t numberOfOptions = 0;

        auto& [uid, info] = *m_ir->GetIdAndKindInfo(varInfo->GetTypeId().GetName());

        if (info.IsKindOneOf(Kind::Enum))
        {
            auto isScoped = get<EnumerationInfo>(info.GetSubRefAs<ClassInfo>().m_subInfo).m_isScoped;
            auto prefix = isScoped ? UnmangleTrimedName(uid.m_name) + "::" : "";

            auto& list = info.GetSubRefAs<ClassInfo>().GetMemberFields();
            std::for_each(list.begin(), list.end(),
                          [&optValues, &prefix](const IdentifierUID& member) { optValues.append(prefix + member.GetNameLeaf()); });

            numberOfOptions = optValues.size();
        }
        else if (info.GetKind() == Kind::Type)
        {
            auto typeRef = info.GetSubRefAs<TypeRefInfo>();

            if (typeRef.m_typeId.m_name.find("bool") != string::npos)
            {
                optValues.append("false");
                optValues.append("true");
                numberOfOptions = 2;
            }
            else if (typeRef.m_typeId.m_name.find("int") != string::npos)
            {
                // Not adding anything to optValues means we have no valid options
                // This is intentional, but it means the variable won't be exported as an option
                isRange = true;
                numberOfOptions = 0;

                // Integers should support a range of possible values, for example [0 .. 63] (or any other range)
                // Range should be added as an attribute specifier. In the above example it will be [[range(0, 63]] 
                auto rangeAttribute = m_ir->m_symbols.GetAttribute(varUid, "range");
                if (!rangeAttribute)
                {
                    throw AzslcEmitterException(EMITTER_INTEGER_HAS_NO_RANGE,
                                                none, none, ConcatString("Option (", varUid.m_name, ") must decorate declaration with an attribute [range(minimum value, maximum value)]"));
                }

                if (rangeAttribute->m_argList.size() != 2)
                {
                    throw AzslcEmitterException(EMITTER_INTEGER_RANGE_NEEDS_ATTRIBUTE,
                                                none, none, ConcatString("Option (", varUid.m_name, ") must specify a range with exactly 2 values, min & max - [range(min, max)]"));
                }

                const auto rangeMin = rangeAttribute->m_argList[0];
                if (!holds_alternative<ConstNumericVal>(rangeMin))
                {
                    throw AzslcEmitterException(EMITTER_INTEGER_RANGE_MIN_IS_NOT_CONST,
                                                none, none, ConcatString("Option (", varUid.m_name, ") must specify a numeric const for its range's minimum!"));
                }

                const auto rangeMax = rangeAttribute->m_argList[1];
                if (!holds_alternative<ConstNumericVal>(rangeMax))
                {
                    throw AzslcEmitterException(EMITTER_INTEGER_RANGE_MAX_IS_NOT_CONST,
                                                none, none, ConcatString("Option (", varUid.m_name, ") must specify a numeric const for its range's maximum!"));
                }

                const auto rangeMinValue = ExtractValueAsInt64(get<ConstNumericVal>(rangeMin));
                const auto rangeMaxValue = ExtractValueAsInt64(get<ConstNumericVal>(rangeMax));
                const auto rangeCount = (rangeMaxValue - rangeMinValue + 1);

                if (rangeMinValue > rangeMaxValue)
                {
                    throw AzslcEmitterException(EMITTER_INTEGER_RANGE_MIN_IS_BIGGER_THAN_MAX,
                                                none, none, ConcatString("Option (", varUid.m_name, ") cannot specify a minimum for its range that is greater than its maximum!"));
                }

                if (rangeCount > kIntegerMaxShaderVariantValues)
                {
                    PrintWarning(Warn::W1, none, "Option (", varUid.m_name, ") must specify a range of values smaller than ", kIntegerMaxShaderVariantValues);
                }

                optValues.append(std::to_string(rangeMinValue));
                optValues.append(std::to_string(rangeMaxValue));
                numberOfOptions = static_cast<uint32_t>(rangeMaxValue - rangeMinValue + 1);
            }
            else
            {
                // There is no immediate plan to support floats or more complex structures
                throw AzslcEmitterException(EMITTER_OPTION_HAS_UNSUPPORTED_TYPE,
                                            none, none, ConcatString("Option (", varUid.m_name, ") must be of type bool, int, or enum"));
            }
        }

        if (numberOfOptions > 0)
        {
            uint32_t keySizeInBits = 1;
            uint32_t maxNumberOfOptions = 2;
            while (numberOfOptions > maxNumberOfOptions)
            {
                maxNumberOfOptions *= 2;
                keySizeInBits++;
            }
            varOption["keySize"] = keySizeInBits;
        }

        varOption["values"]  = optValues;
        varOption["range"]   = isRange;
    }


    Json::Value Backend::GetVariantList(const Options& options, bool includeEmpty) const
    {
        Json::Value varRoot(Json::objectValue);
        varRoot["meta"] = "Variant options list exported by AZSLc";

        bool useSpecializationConstants = false;

        Json::Value shaderOptions(Json::arrayValue);
        uint32_t keyOffsetBits = 0;

        uint32_t optionOrder = 0;
        for (auto& [uid, varInfo] : m_ir->m_symbols.GetOrderedSymbolsOfSubType_2<VarInfo>())
        {
            // skip non-option variables
            if (!varInfo->CheckHasStorageFlag(StorageFlag::Option))
            {
                continue;
            }

            Json::Value shaderOption(Json::objectValue);
            shaderOption["name"] = UnmangleTrimedName(uid.m_name);
            shaderOption["type"] = UnmangleTrimedName(varInfo->m_typeInfoExt);
            shaderOption["defaultValue"] = GetInitializerClause(varInfo);

            // The order (or rank) of the option matches the order of declaration
            // We reserve the right to change it in the future so we make it explicit attribute here
            shaderOption["order"] = optionOrder;
            optionOrder++;
            shaderOption["costImpact"] = varInfo->m_estimatedCostImpact;

            bool isUdt = IsUserDefined(varInfo->GetTypeClass());
            assert(isUdt || IsPredefinedType(varInfo->GetTypeClass()));
            shaderOption["kind"] = isUdt ? "user-defined" : "predefined";
            shaderOption["specializationId"] = varInfo->m_specializationId;
            useSpecializationConstants |= varInfo->m_specializationId >= 0;

            AppendOptionRange(shaderOption, uid, varInfo, options);

            if (includeEmpty || (shaderOption["values"].isArray() && shaderOption["values"].size() > 0))
            {   // We only emit variant options which have positive number of valid values
                // Because we use uint on the shader source side no shader option can cross the 32-bit boundary
                uint32_t keySizeInBits = shaderOption["keySize"].asUInt();

                if (keySizeInBits > kShaderVariantKeyElementSize)
                {
                    const string errorMessage = ConcatString("Shader option {", UnmangleTrimedName(uid.m_name), "} uses a bitmask which crosses the ",
                                                             kShaderVariantKeyElementSize, "-bit boundary!");
                    throw AzslcEmitterException(EMITTER_OVERFLOW_BIT_BOUNDARY, errorMessage);
                }

                uint32_t remainingBitsAfterOffset = kShaderVariantKeyElementSize - (keyOffsetBits % kShaderVariantKeyElementSize);
                if (keySizeInBits > remainingBitsAfterOffset)
                {
                    keyOffsetBits += remainingBitsAfterOffset;
                }

                shaderOption["keyOffset"] = keyOffsetBits;
                keyOffsetBits += keySizeInBits;

                shaderOptions.append(shaderOption);
            }
        }

        varRoot["specializationConstants"] = options.m_useSpecializationConstantsForOptions && useSpecializationConstants;
        varRoot["ShaderOptions"] = shaderOptions;

        return varRoot;
    }

    void Backend::SetupOptionsSpecializationId(const Options& options) const
    {
        uint32_t specializationId = 0;
        for (auto& [uid, varInfo, kindInfo] : m_ir->m_symbols.GetOrderedSymbolsOfSubType_3<VarInfo>())
        {
            if (varInfo->CheckHasStorageFlag(StorageFlag::Option) &&
                !m_ir->m_symbols.GetAttribute(uid, "no_specialization"))
            {
                varInfo->m_specializationId = specializationId++;
            }
        }
    }

    // little check utility
    static void CheckHasOneFoldedDimensionOrThrow(const ArrayDimensions& dims, string_view callSite)
    {
        if (!dims.AreAllDimsFullyConstantFolded())
        {
            throw AzslcEmitterException(EMITTER_INVALID_ARRAY_DIMENSIONS,
                                        ConcatString(callSite, " Invalid array dimensions (more than 1 or non constant size)"));
        }
    }

    static void PrintWarningsRelatedToSpecOvershoots(MultiBindingLocationMaker& bindInfo, const Options& options, const SRGInfo* srgInfo, const IdentifierUID& srgUid)
    {
        int numSamplerUsed = bindInfo.m_untainted.GetAccumulated(BindingType::S);
        if (options.m_minAvailableDescriptors.m_samplers >= 0
            && numSamplerUsed > options.m_minAvailableDescriptors.m_samplers)
        {
            PrintWarning(Warn::W1, srgInfo->m_declNode->start,
                         "SRG ", srgUid.m_name,
                         " bumped samplers to ", numSamplerUsed, " which overshoots the minimum supported sampler count guaranteed by the specification (from --min-descriptors argument currently set to ",
                         options.m_minAvailableDescriptors.m_samplers, ")");
        }

        int numTextureUsed = bindInfo.m_untainted.GetAccumulated(BindingType::T);
        if (options.m_minAvailableDescriptors.m_textures >= 0
            && numTextureUsed > options.m_minAvailableDescriptors.m_textures)
        {
            PrintWarning(Warn::W1, srgInfo->m_declNode->start,
                         "SRG ", srgUid.m_name,
                         " bumped textures to ", numTextureUsed, " which overshoots the minimum supported texture count guaranteed by the specification (from --min-descriptors argument currently set to ",
                         options.m_minAvailableDescriptors.m_textures, ")");
        }

        int numViewUsed = bindInfo.m_untainted.GetAccumulated(BindingType::B) + bindInfo.m_untainted.GetAccumulated(BindingType::U);
        if (options.m_minAvailableDescriptors.m_buffers >= 0
            && numViewUsed > options.m_minAvailableDescriptors.m_buffers)
        {
            PrintWarning(Warn::W1, srgInfo->m_declNode->start,
                         "SRG ", srgUid.m_name,
                         " bumped data views to ", numViewUsed, " which overshoots the minimum supported data views count guaranteed by the specification (from --min-descriptors argument currently set to ",
                         options.m_minAvailableDescriptors.m_buffers, ")");
        }

        int totalUsed = numSamplerUsed + numTextureUsed + numViewUsed;
        if (options.m_minAvailableDescriptors.m_descriptorsTotal >= 0
            && totalUsed > options.m_minAvailableDescriptors.m_descriptorsTotal)
        {
            PrintWarning(Warn::W1, srgInfo->m_declNode->start,
                         "SRG ", srgUid.m_name,
                         " bumped descriptors to ", totalUsed, " which overshoots the minimum supported descriptor count guaranteed by the specification (from --min-descriptors argument currently set to ",
                         options.m_minAvailableDescriptors.m_descriptorsTotal, ")");
        }
    }

    RootSigDesc::SrgParamDesc Backend::ReflectOneExternalResource(IdentifierUID id, MultiBindingLocationMaker& bindInfo, RootSigDesc& rootSig) const
    {
        Kind kind = m_ir->GetKind(id);
        int count = 1;
        RootParamType paramType = RootParamType::SrgConstantCB;  // this is the default because when "kind" is not "Variable", this function is used on symbols of "kind" "SRG"
        bool isUnboundedArray = false;
        if (kind == Kind::Variable)
        {
            const auto* memberInfo = m_ir->GetSymbolSubAs<VarInfo>(id.m_name);

            // With this variable we track the need to validate improperly
            // defined array sizes.
            bool shouldCheckForValidArraySize = true;
            isUnboundedArray = memberInfo->GetArrayDimensions().IsUnbounded();
            if (isUnboundedArray)
            {
                TypeClass typeClass = memberInfo->GetTypeClass();
                if ( CanBeDeclaredAsUnboundedArray(typeClass) )
                {
                    shouldCheckForValidArraySize = false;
                }
            }

            if (shouldCheckForValidArraySize)
            {
                CheckHasOneFoldedDimensionOrThrow(memberInfo->GetArrayDimensions(), "CodeEmitter::BuildSignatureDescription");
            }
            count = isUnboundedArray ? 1 : memberInfo->GetArrayDimensions().GetDimensionAt_OrDefault(0, 1);
            paramType = FindParamType(memberInfo->m_typeInfoExt);
        }

        auto regType = RootParamTypeToBindingType(paramType);

        BindingPair binding = bindInfo.GetCurrent(regType);
        if (isUnboundedArray
            && GetPlatformEmitter().RequiresUniqueSpaceForUnboundedArrays() /* refer to note 1 below */ )
        {
            // Note 1: On o3de/Atom, the vulkan RHI assumes SRG binding IDs to be 0-7 as there are fixed data structures
            //         using a MaxSRGs of 8 in the code. Adapting the RHI to remove this assumption would have been costly,
            //         therefore RequiresUniqueSpaceForUnboundedArrays is now true only for --namespace=dx.

            // Use a unique register space for every unbounded array because in DirectX 12 unbounded arrays consume
            // all remaining registers for that resource type. By making a unique space for each unbounded array
            // we support an unlimited number of them.
            // Note that this does not impact other platforms, where register spaces don't matter (DXC ignores them)
            // and unbounded arrays do not consume all remaining registers.
            // Also, on dx12 we don't necessarily need to call SignalIncrementRegister() in this case, and could instead
            // just use register 0 because the register space is unique, but since other platforms ignore the register
            // space it's easier to also increment the register on all platforms.
            
            binding.m_pair[BindingPair::Set::Untainted].m_logicalSpace = m_unboundedSpillSpace;
            binding.m_pair[BindingPair::Set::Merged].m_logicalSpace = m_unboundedSpillSpace;
            ++m_unboundedSpillSpace;
        }

        bindInfo.SignalIncrementRegister(regType, count);

        auto srgElementDesc = RootSigDesc::SrgParamDesc{ id, paramType, binding, count, -1, isUnboundedArray};

        rootSig.m_descriptorMap.emplace(id, srgElementDesc);

        return srgElementDesc;
    }

    RootSigDesc::SrgParamDesc Backend::ReflectOneExternalResourceAndWrapWithUnifyIndices(IdentifierUID id, MultiBindingLocationMaker& bindInfo, RootSigDesc& rootSig) const
    {
        auto paramDesc = ReflectOneExternalResource(id, bindInfo, rootSig);
        bindInfo.SignalUnifyIndices();
        return paramDesc;
    }

    RootSigDesc Backend::BuildSignatureDescription(const Options& options, int num32BitConst) const
    {
        MultiBindingLocationMaker bindInfo{ options };
        RootSigDesc rootSig;

        auto allSrgs = m_ir->m_symbols.GetOrderedSymbolsOfSubType_2<SRGInfo>();
        using Id_SrgInfo_Pair = decltype(allSrgs)::value_type;
        // let's order them by frequency:
        auto orderingFunction = [this](const Id_SrgInfo_Pair& srgSymbol1, const Id_SrgInfo_Pair& srgSymbol2)
            {
                auto srgSemantic1 = m_ir->GetSymbolSubAs<ClassInfo>(srgSymbol1.second->m_semantic->GetName())->Get<SRGSemanticInfo>();
                auto srgSemantic2 = m_ir->GetSymbolSubAs<ClassInfo>(srgSymbol2.second->m_semantic->GetName())->Get<SRGSemanticInfo>();
                return *srgSemantic1->m_frequencyId < *srgSemantic2->m_frequencyId;
            };
        std::sort(allSrgs.begin(), allSrgs.end(), orderingFunction);

        const bool useUniqueIndices = options.m_useUniqueIndices;

        // loop on SRGs since they're the containers of signature-inducing symbols
        for (auto& [srgUid, srgInfo] : allSrgs)
        {
            RootSigDesc::SrgDesc srgDesc;
            srgDesc.m_uid = srgUid;

            for (const auto& tId : srgInfo->m_srViews)
            {
                if (useUniqueIndices && std::find(srgInfo->m_unboundedArrays.begin(), srgInfo->m_unboundedArrays.end(), tId) != srgInfo->m_unboundedArrays.end())
                {
                    // This variable will be added to the end of the binding table.
                    // See: "REMARK: --unique-idx" a few lines below.
                    continue;
                }
                srgDesc.m_parameters.push_back(
                    ReflectOneExternalResourceAndWrapWithUnifyIndices(tId, bindInfo, rootSig) );
            }
            for (const auto& sId : srgInfo->m_samplers)
            {
                if (useUniqueIndices && !srgInfo->m_unboundedArrays.empty() && srgInfo->m_unboundedArrays[0] == sId)
                {
                    // This variable will be added to the end of the binding table.
                    // See: "REMARK: --unique-idx" a few lines below.
                    continue;
                }
                srgDesc.m_parameters.push_back(
                    ReflectOneExternalResourceAndWrapWithUnifyIndices(sId, bindInfo, rootSig) );
            }

            bool hasSrgConstants = !srgInfo->m_implicitStruct.GetMemberFields().empty();
            bool hasConstantBuffers = !srgInfo->m_CBs.empty();
            if (hasSrgConstants || (hasConstantBuffers && options.m_emitConstantBufferBody))
            {
                srgDesc.m_parameters.push_back(
                    ReflectOneExternalResourceAndWrapWithUnifyIndices(srgUid, bindInfo, rootSig));
            }
            if (!options.m_emitConstantBufferBody)  // emitCB is the SM5- "cbufer{}" block syntax. !emitCB is the "ConstantBuffer<>" SM5.1+ syntax
            {
                for (const auto& cId : srgInfo->m_CBs)
                {
                    srgDesc.m_parameters.push_back(
                        ReflectOneExternalResourceAndWrapWithUnifyIndices(cId, bindInfo, rootSig));
                }
            }

            // REMARK: --unique-idx
            // When using --unique-idx, it is imperative to reflect the unbounded array AFTER
            // The SRG constant buffer and any constant buffer owned by the SRG because
            // they are sharing the same register index range. Because an unbounded array
            // takes ownership of the remaining register range within a register space, it always
            // must be added after that last resource in the register space.
            if (useUniqueIndices)
            {
                for (const auto& tId : srgInfo->m_unboundedArrays)
                {
                    srgDesc.m_parameters.push_back(
                        ReflectOneExternalResourceAndWrapWithUnifyIndices(tId, bindInfo, rootSig));
                }
            }

            bindInfo.SignalIncrementSpace(/*overshoot callback:*/[&, srgInfo = srgInfo, srgUid = srgUid](int numSpaces, int spacesAvailable)
                {
                    PrintWarning(Warn::W1, srgInfo->m_declNode->start,
                                 "SRG ", srgUid.m_name, " on space ", numSpaces,
                                 " overshoots the minimum supported logical register space count guaranteed by the specification (from --min-descriptors argument currently set to ",
                                 spacesAvailable, ")");
                });

            bindInfo.SignalUnifyIndices();

            // validate binding counts if options are activated for it
            PrintWarningsRelatedToSpecOvershoots(bindInfo, options, srgInfo, srgUid);

            rootSig.m_srGroups.push_back(srgDesc);
        } // end for all SRGs

        // and separately (not within SRGs), the root constants:
        if (options.m_rootConstantsMaxSize > 0)
        {
            RootSigDesc::SrgDesc rootConstDesc;
            rootConstDesc.m_uid = m_ir->m_rootConstantStructUID;

            auto desc = RootSigDesc::SrgParamDesc{ rootConstDesc.m_uid, RootParamType::RootConstantCB,
                bindInfo.GetCurrent(BindingType::B), 1, num32BitConst };

            rootConstDesc.m_parameters.push_back(desc);
            rootSig.m_descriptorMap.emplace(rootConstDesc.m_uid, desc);
            rootSig.m_srGroups.push_back(rootConstDesc);
        }

        return rootSig;
    }

    //static
    const char* Backend::GetInputModifier(const TypeQualifiers& typeQualifier)
    {
        const bool in = TypeHasStorageFlag(typeQualifier, StorageFlag::In);
        const bool out = TypeHasStorageFlag(typeQualifier, StorageFlag::Out);
        const bool inout = (in && out) || TypeHasStorageFlag(typeQualifier, StorageFlag::InOut);
        return inout ? "inout"
                     : (in ? "in"
                           : (out ? "out" : ""));
    }

    // static
    string Backend::GetTypeModifier(const ExtendedTypeInfo& typeInfo, const Options& options, Modifiers bannedFlags /*= {}*/)
    {
        using namespace std::string_literals;
        string modifiers;
        bool isMatrix = typeInfo.m_coreType.m_arithmeticInfo.IsMatrix();
        if (typeInfo.CheckHasStorageFlag(StorageFlag::ColumnMajor) && !(bannedFlags & StorageFlag::ColumnMajor))
        {
            modifiers = "column_major";
        }
        else if (typeInfo.CheckHasStorageFlag(StorageFlag::RowMajor) && !(bannedFlags & StorageFlag::RowMajor))
        {
            modifiers = "row_major";
        }
        else if (options.m_forceEmitMajor && isMatrix)
        {
            modifiers = options.m_forceMatrixRowMajor ? "row_major" : "column_major";
        }

        auto maybeSpace = [&modifiers](){ return modifiers.empty() ? "" : " "; };
        using SF = StorageFlag;
        static const StorageFlag toReEmit[] = {SF::Static, SF::Extern, SF::Inline,
            SF::Const, SF::Volatile, SF::Precise, SF::Groupshared,
            SF::Uniform, SF::Globallycoherent, SF::Unsigned};
        for (int i = 0; i < std::size(toReEmit); ++i)
        {
            if (typeInfo.CheckHasStorageFlag(toReEmit[i]) && !(bannedFlags & toReEmit[i]))
            {
                modifiers += maybeSpace() + ToLower(StorageFlag::ToStr(toReEmit[i]));
            }
        }

        if (typeInfo.CheckHasStorageFlag(StorageFlag::Other) && !(bannedFlags & StorageFlag::Other))
        {
            for (const auto& flag : typeInfo.m_qualifiers.m_others)
            {
                modifiers += " " + flag;
            }
        }

        return modifiers;
    }

    string Backend::GetExtendedTypeInfo(const ExtendedTypeInfo& extTypeInfo, const Options& options, Modifiers banned, std::function<string(const TypeRefInfo&)> translator) const
    {
        string hlslString = GetTypeModifier(extTypeInfo, options, banned);
        hlslString += hlslString.empty() ? "" : " ";
        if (extTypeInfo.m_coreType.m_typeClass == TypeClass::Alias)
        {
            hlslString += GetExtendedTypeInfo(m_ir->GetSymbolSubAs<TypeAliasInfo>(extTypeInfo.m_coreType.m_typeId.GetName())->m_canonicalType, options, banned, translator);
        }
        else if (HasGenericParameter(extTypeInfo.m_coreType.m_typeClass) || !extTypeInfo.m_genericParameter.IsEmpty())
        {
            hlslString += translator(extTypeInfo.m_coreType)
                + "<" + translator(extTypeInfo.m_genericParameter);
            if (extTypeInfo.m_genericDims.IsArray())
            {
                hlslString += extTypeInfo.m_genericDims.ToString(", ", ", ", "");
            }
            hlslString += ">";
        }
        else
        {
            hlslString += translator(extTypeInfo.m_coreType);
        }

        return hlslString;
    }

    const PlatformEmitter& Backend::GetPlatformEmitter(IntermediateRepresentation* ir)
    {
        const auto p = PlatformEmitter::GetEmitter(ir->m_metaData.m_platformEmitterNamespace);
        if (p)
        {
            return *p;
        }

        return *PlatformEmitter::GetDefaultEmitter();
    }

    uint32_t Backend::GetNumberOf32BitConstants(const Options& options, const IdentifierUID& uid) const
    {
        // Count the number of 32 bin constants
        uint32_t numberOf32bitRootConstants = 0;
        auto* rootConstInfo = m_ir->GetSymbolSubAs<ClassInfo>(uid.GetName());
        if (options.m_rootConstantsMaxSize && rootConstInfo)
        {
            for (const auto& memberVar : rootConstInfo->GetMemberFields())
            {
                const auto& varInfo = *m_ir->GetSymbolSubAs<VarInfo>(memberVar.m_name);
                assert(!IsChameleon(varInfo.GetTypeClass()));
                auto exportedType = varInfo.m_typeInfoExt.m_coreType;

                if (!exportedType.IsPackable())
                {
                    throw std::logic_error{ " internal error: unpackable type ("
                        + exportedType.m_typeId.m_name
                        + ") found its way in layout member "
                        + memberVar.m_name };
                }

                // GetTotalSize of each member of the structure
                uint32_t size = varInfo.m_typeInfoExt.GetTotalSize(options.m_packDataBuffers, options.m_forceMatrixRowMajor);

                numberOf32bitRootConstants += (size / 4);
            }
            return numberOf32bitRootConstants;
        }
        return 0;
    }
}
