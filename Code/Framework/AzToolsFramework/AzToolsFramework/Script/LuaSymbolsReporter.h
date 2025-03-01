/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


//AZTF-SHARED
#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace AzToolsFramework
{
    namespace Script
    {
        struct AZTF_API LuaPropertySymbol
        {
            AZ_TYPE_INFO(LuaPropertySymbol, "{5AFB147F-50A4-4F00-9F82-D8D5BBC970D6}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            bool m_canRead;
            bool m_canWrite;

            AZStd::string ToString() const;
        };

        struct AZTF_API LuaMethodSymbol
        {
            AZ_TYPE_INFO(LuaMethodSymbol, "{7B074A36-C81D-46A0-8D2F-62E426EBE38A}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZStd::string m_debugArgumentInfo;

            AZStd::string ToString() const;
        };

        struct AZTF_API LuaClassSymbol
        {
            AZ_TYPE_INFO(LuaClassSymbol, "{5FBE5841-A8E1-44B6-BEDA-22302CF8DF5F}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZ::Uuid m_typeId;
            AZStd::vector<LuaPropertySymbol> m_properties;
            AZStd::vector<LuaMethodSymbol> m_methods;

            AZStd::string ToString() const;
        };

        struct AZTF_API LuaEBusSender
        {
            AZ_TYPE_INFO(LuaEBusSender, "{23EE4188-0924-49DB-BF3F-EB7AAB6D5E5C}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZStd::string m_debugArgumentInfo;
            AZStd::string m_category;

            AZStd::string ToString() const;
        };

        struct AZTF_API LuaEBusSymbol
        {
            AZ_TYPE_INFO(LuaEBusSymbol, "{381C5639-A916-4D2E-B825-50A3F2D93137}");
            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            bool m_canBroadcast;
            bool m_canQueue;
            bool m_hasHandler;

            AZStd::vector<LuaEBusSender> m_senders;

            AZStd::string ToString() const;
        };

    } // namespace Script
} // namespace AzToolsFramework
