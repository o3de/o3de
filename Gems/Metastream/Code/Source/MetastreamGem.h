/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <Metastream/MetastreamBus.h>
#include <CryCommon/IConsole.h>

#include <IGem.h>
#include "BaseHttpServer.h"
#include "DataCache.h"

namespace Metastream
{
    class MetastreamReflectComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MetastreamReflectComponent, "{7777F7C2-5CD5-4DCE-BA53-086A8E14CEAF}");
        static void Reflect(AZ::ReflectContext* context);

        void Activate() override {}
        void Deactivate() override {}
    };

    class MetastreamGem
        : public CryHooksModule
        , public MetastreamRequestBus::Handler
    {
        AZ_RTTI(MetastreamGem, "{0BACF38B-9774-4771-89E2-B099EA9E3FE7}", CryHooksModule);
        
    public:
        MetastreamGem();
        ~MetastreamGem() override;

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

        virtual void AddStringToCache(const char* table, const char* key, const char* value) override;
        virtual void AddBoolToCache(const char* table, const char* key, bool value) override;
        virtual void AddVec3ToCache(const char* table, const char* key, const Vec3 & value) override;
        virtual void AddDoubleToCache(const char* table, const char* key, double value) override;
        virtual void AddUnsigned64ToCache(const char* table, const char* key, AZ::u64 value) override;
        virtual void AddSigned64ToCache(const char* table, const char* key, AZ::s64 value) override;

        virtual void AddArrayToCache(const char* table, const char* key, const char* arrayName) override;
        virtual void AddObjectToCache(const char* table, const char* key, const char* objectName) override;

        virtual void AddStringToArray(const char* table, const char* arrayName, const char* value) override;
        virtual void AddBoolToArray(const char* table, const char* arrayName, bool value) override;
        virtual void AddVec3ToArray(const char* table, const char* arrayName, const Vec3 & value) override;
        virtual void AddDoubleToArray(const char* table, const char* arrayName, double value) override;
        virtual void AddUnsigned64ToArray(const char* table, const char* arrayName, AZ::u64 value) override;
        virtual void AddSigned64ToArray(const char* table, const char* arrayName, AZ::s64 value) override;

        virtual void AddArrayToObject(const char* table, const char* destObjectName, const char *key, const char *srcArrayName) override;
        virtual void AddObjectToObject(const char* table, const char* destObjectName, const char *key, const char* sourceObjectName) override;
        virtual void AddObjectToArray(const char* table, const char* destArrayName, const char* sourceObjectName) override;

        virtual void AddStringToObject(const char* table, const char* objectName, const char* key, const char* value) override;
        virtual void AddBoolToObject(const char* table, const char* objectName, const char* key, bool value) override;
        virtual void AddVec3ToObject(const char* table, const char* objectName, const char* key, const Vec3 & value) override;
        virtual void AddDoubleToObject(const char* table, const char* objectName, const char* key, double value) override;
        virtual void AddUnsigned64ToObject(const char* table, const char* objectName, const char* key, AZ::u64 value) override;
        virtual void AddSigned64ToObject(const char* table, const char* objectName, const char* key, AZ::s64 value) override;

        virtual bool StartHTTPServer() override;
        virtual void StopHTTPServer() override;

    protected:
        // Unit test methods to get at m_cache and status
        bool IsServerEnabled() const;
        std::string GetDatabasesJSON() const;
        std::string GetTableKeysJSON(const std::string& tableName) const;
        bool ClearCache();

    private:
        // Console commands
        static void StartHTTPServerCmd(IConsoleCmdArgs* args);
        static void StopHTTPServerCmd(IConsoleCmdArgs* args);

        // CVars
        int m_serverEnabled;
        ICVar* m_serverOptionsCVar;

        std::unique_ptr<BaseHttpServer> m_server;
        std::unique_ptr<DataCache> m_cache;
    };
} // namespace Metastream
