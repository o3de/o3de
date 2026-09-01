/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzslcEmitter.h>
#include "CommonVulkanPlatformEmitter.h"

namespace AZ::ShaderCompiler
{
    string CommonVulkanPlatformEmitter::GetSpecializationConstant(const CodeEmitter& codeEmitter, const IdentifierUID& symbolUid, const Options& options) const
    {
        std::stringstream stream;
        auto* ir = codeEmitter.GetIR();
        auto* varInfo = ir->GetSymbolSubAs<VarInfo>(symbolUid.GetName());
        auto retInfo = varInfo->GetTypeRefInfo();
        string typeAsStr = codeEmitter.GetTranslatedName(retInfo, UsageContext::ReferenceSite);
        string defaultValue = codeEmitter.GetInitializerClause(varInfo);
        Modifiers forbidden = StorageFlag::Static;
        assert(varInfo->m_specializationId >= 0);
        stream << "[[vk::constant_id(" << varInfo->m_specializationId << ")]]\n";
        if (retInfo.m_typeClass == TypeClass::Enum)
        {
            // Enums are not a valid type for specialization constant, so we use the underlaying scalar type.
            // The we add a global static variable with the enum type that cast from the specialization constant.
            auto* enumClassInfo = ir->GetSymbolSubAs<ClassInfo>(retInfo.m_typeId.GetName());
            auto scalarType = enumClassInfo->Get<EnumerationInfo>()->m_underlyingType.m_arithmeticInfo.UnderlyingScalarToStr();
            string scName = JoinAllNestedNamesWithUnderscore(symbolUid.m_name) + "_SC_OPTION";
            stream << "const " << scalarType << " " << scName;
            // TODO: if using a default value, emit it as the underlying scalar type, since enums are not valid default values for
            // specialization constants (casting is also not allowed). We set default values for shader options at runtime so it's not
            // a problem.
            stream << " = (" << scalarType << ")" << "0;\n";
            // Set the default value for the global static variable as the value of the specialization constant.
            defaultValue = std::move(scName);
            forbidden = Modifiers{};
        }
        stream << codeEmitter.GetTranslatedName(varInfo->m_typeInfoExt, UsageContext::ReferenceSite, options, forbidden) + " ";
        stream << codeEmitter.GetTranslatedName(symbolUid.m_name, UsageContext::DeclarationSite) + " = ";
        stream << "(" << typeAsStr << ")" << (defaultValue.empty() ? "0" : defaultValue) << "; \n";
        return stream.str();
    }

    std::pair<string, string> CommonVulkanPlatformEmitter::GetDataViewHeaderFooter(
        const CodeEmitter& codeEmitter,
        const IdentifierUID& symbol,
        uint32_t bindInfoRegisterIndex,
        string_view registerTypeLetter,
        optional<string> stringifiedLogicalSpace,
        const Options& options) const
    {
        std::stringstream stream;
        optional<AttributeInfo> inputAttachmentIndexAttribute;
        auto varInfo = codeEmitter.GetIR()->GetSymbolSubAs<VarInfo>(symbol.GetName());
        bool isSubpassInput = StartsWith(varInfo->m_typeInfoExt.m_coreType.m_typeId.GetName(), "?SubpassInput");
        if (isSubpassInput)
        {
            inputAttachmentIndexAttribute = codeEmitter.GetIR()->m_symbols.GetAttribute(symbol, "input_attachment_index");
            if (inputAttachmentIndexAttribute)
            {
                inputAttachmentIndexAttribute->m_namespace = "vk";
                inputAttachmentIndexAttribute->m_category = AttributeCategory::Sequence;
                inputAttachmentIndexAttribute->m_argList[0] = static_cast<ConstNumericVal>(ExtractValueAs<int32_t>(get<ConstNumericVal>(inputAttachmentIndexAttribute->m_argList[0]), 0) + options.m_subpassInputsOffset);
                MakeOStreamStreamable soss(stream);
                CodeEmitter::EmitAttribute(*inputAttachmentIndexAttribute, soss);
                stream << "[[vk::binding(" << bindInfoRegisterIndex;
                if (stringifiedLogicalSpace)
                {
                    stream << ", " << *stringifiedLogicalSpace;
                }
                stream << ")]]\n";
            }
        }

        string registerString;
        if (!inputAttachmentIndexAttribute)
        {   // fallback to the base behavior in non-input-attachment cases for the `.. : register();` syntax.
            registerString = PlatformEmitter::GetDataViewHeaderFooter(codeEmitter,
                symbol,
                bindInfoRegisterIndex,
                registerTypeLetter,
                stringifiedLogicalSpace,
                options).second;
        }
        return { stream.str(), registerString };
    }
}
