/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/OSAllocator.h>


//! Add GetO3deTypeName and GetO3deTypeId definitions for commonly used O3DE types
namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(AZ::Uuid, "AZ::Uuid", "{E152C105-A133-4d03-BBF8-3D4B2FBA3E2A}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(PlatformID, "PlatformID", "{0635D08E-DDD2-48DE-A7AE-73CC563C57C3}");

    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(char, "char", "{3AB0037F-AF8D-48ce-BCA0-A170D18B2C03}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(AZ::s8, "AZ::s8", "{58422C0E-1E47-4854-98E6-34098F6FE12D}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(short, "short", "{B8A56D56-A10D-4dce-9F63-405EE243DD3C}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(int, "int", "{72039442-EB38-4d42-A1AD-CB68F7E0EEF6}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(long, "long", "{8F24B9AD-7C51-46cf-B2F8-277356957325}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(AZ::s64, "AZ::s64", "{70D8A282-A1EA-462d-9D04-51EDE81FAC2F}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(unsigned char, "unsigned char", "{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(unsigned short, "unsigned short", "{ECA0B403-C4F8-4b86-95FC-81688D046E40}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(unsigned int, "unsigned int", "{43DA906B-7DEF-4ca8-9790-854106D3F983}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(unsigned long, "unsigned long", "{5EC2D6F7-6859-400f-9215-C106F5B10E53}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(AZ::u64, "AZ::u64", "{D6597933-47CD-4fc8-B911-63F3E2B0993A}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(float, "float", "{EA2C3E90-AFBE-44d4-A90D-FAAF79BAF93D}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(double, "double", "{110C4B14-11A8-4e9d-8638-5051013A56AC}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(bool, "bool", "{A0CA880C-AFE4-43cb-926C-59AC48496112}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(void, "void", "{C0F1AFAD-5CB3-450E-B0F5-ADB5D46B0E22}");
}

namespace AZStd
{
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(monostate, "AZStd::monostate", "{B1E9136B-D77A-4643-BE8E-2ABDA246AE0E}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(allocator, "allocator", "{E9F5A3BE-2B3D-4C62-9E6B-4E00A13AB452}");

    // Make sure to postfix the Template Uuid to the template parameters Type Uuid
    // to maintain the same aggregate Uuid for the string classes
    AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID(char_traits, "AZStd::char_traits",, "{9B018C0C-022E-4BA4-AE91-2C1E8592DBB2}", \
        inline);
    AZ_TYPE_INFO_INTERNAL_BOTHFIX_UUID(char_traits, "AZStd::char_traits",, "{9B018C0C-022E-4BA4-AE91-2C1E8592DBB2}", \
        inline, AZ_TYPE_INFO_INTERNAL_TYPENAME);

    AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID(basic_string_view, "AZStd::basic_string_view",, "{D348D661-6BDE-4C0A-9540-FCEA4522D497}", \
        inline);
    AZ_TYPE_INFO_INTERNAL_BOTHFIX_UUID(basic_string_view, "AZStd::basic_string_view",, "{D348D661-6BDE-4C0A-9540-FCEA4522D497}", \
        inline, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);

    AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID(basic_string, "AZStd::basic_string",, "{C26397ED-8F60-4DF6-8320-0D0C592DA3CD}", \
        inline);
    AZ_TYPE_INFO_INTERNAL_BOTHFIX_UUID(basic_string, "AZStd::basic_string",, "{C26397ED-8F60-4DF6-8320-0D0C592DA3CD}", \
        inline, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_TYPENAME);

    AZ_TEMPLATE_INFO_INTERNAL_BOTHFIX_UUID(basic_fixed_string, "AZStd::fixed_string",, "{FA339E31-C383-49C7-80AC-5E1A3D8FA296}", \
        inline);
    AZ_TYPE_INFO_INTERNAL_BOTHFIX_UUID(basic_fixed_string, "AZStd::fixed_string",, "{FA339E31-C383-49C7-80AC-5E1A3D8FA296}", \
        inline, AZ_TYPE_INFO_INTERNAL_TYPENAME, AZ_TYPE_INFO_INTERNAL_AUTO, AZ_TYPE_INFO_INTERNAL_TYPENAME);


    // Explicit instantiations of the GetO3de* functions for char_traits<char>
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_INSTANTIATE(char_traits, char)

    // Explicit instantiations of the GetO3de* functions for basic_string_view<char, char_traits<char>> aka AZStd::string_view
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_INSTANTIATE(basic_string_view, char, char_traits<char>);

    // Explicit instantiations of the GetO3de* functions for basic_string<char, char_traits<char>, allocator> aka AZStd::string
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_INSTANTIATE(basic_string, char, char_traits<char>, allocator);

    // Explicit instantiations of the GetO3de* functions for basic_string<char, char_traits<char>, AZ::AZStdAlloc<AZ::OsAllocator>> aka AZ::OSString
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_INSTANTIATE(basic_string, char, char_traits<char>, AZ::OSStdAllocator);

    // Explicit instantiations of the GetO3de* functions for fixed_string<char, 1024, char_traits<char>> aka AZStd::fixed_string<1024>
    // Other aliases: AZ::IO::FixedMaxPathString, AZ::SettingsRegistryInterface::FixedValueString
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_INSTANTIATE(basic_fixed_string, char, 1024, char_traits<char>);

    // Explicit instantiations of the GetO3de* functions for fixed_string<char, 512, char_traits<char>> aka AZStd::fixed_string<512>
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_INSTANTIATE(basic_fixed_string, char, 512, char_traits<char>);

    // Explicit instantiations of the GetO3de* functions for fixed_string<char, 256, char_traits<char>> aka AZStd::fixed_string<256>
    // Other aliases: AZ::CVarFixedString
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_INSTANTIATE(basic_fixed_string, char, 256, char_traits<char>);

}
