/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <sstream>
#include <string.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Metastream_Traits_Platform.h>

#include "MetastreamGem.h"

#if AZ_TRAIT_METASTREAM_USE_CIVET
#include "CivetHttpServer.h"
#endif // AZ_TRAIT_METASTREAM_USE_CIVET

namespace Metastream
{

    void Metastream::MetastreamReflectComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MetastreamReflectComponent, AZ::Component>()
                ->Version(0)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<MetastreamRequestBus>("MetastreamRequestBus")
                ->Event("StartHTTPServer", &MetastreamRequestBus::Events::StartHTTPServer)
                ->Event("StopHTTPServer", &MetastreamRequestBus::Events::StopHTTPServer)

                ->Event("AddStringToCache", &MetastreamRequestBus::Events::AddStringToCache)
                ->Event("AddBoolToCache", &MetastreamRequestBus::Events::AddBoolToCache)
                ->Event("AddDoubleToCache", &MetastreamRequestBus::Events::AddDoubleToCache)
                ->Event("AddUnsigned64ToCache", &MetastreamRequestBus::Events::AddUnsigned64ToCache)
                ->Event("AddSigned64ToCache", &MetastreamRequestBus::Events::AddSigned64ToCache)
                ->Event("AddEntityIdToCache", &MetastreamRequestBus::Events::AddEntityIdToCache)

                ->Event("AddArrayToCache", &MetastreamRequestBus::Events::AddArrayToCache)
                ->Event("AddObjectToCache", &MetastreamRequestBus::Events::AddObjectToCache)
                
                ->Event("AddArrayToObject", &MetastreamRequestBus::Events::AddArrayToObject)
                ->Event("AddObjectToObject", &MetastreamRequestBus::Events::AddObjectToObject)
                ->Event("AddObjectToArray", &MetastreamRequestBus::Events::AddObjectToArray)

                ->Event("AddStringToArray", &MetastreamRequestBus::Events::AddStringToArray)
                ->Event("AddBoolToArray", &MetastreamRequestBus::Events::AddBoolToArray)
                ->Event("AddDoubleToArray", &MetastreamRequestBus::Events::AddDoubleToArray)
                ->Event("AddUnsigned64ToArray", &MetastreamRequestBus::Events::AddUnsigned64ToArray)
                ->Event("AddSigned64ToArray", &MetastreamRequestBus::Events::AddSigned64ToArray)
                ->Event("AddEntityIdToArray", &MetastreamRequestBus::Events::AddEntityIdToArray)

                ->Event("AddStringToObject", &MetastreamRequestBus::Events::AddStringToObject)
                ->Event("AddBoolToObject", &MetastreamRequestBus::Events::AddBoolToObject)
                ->Event("AddDoubleToObject", &MetastreamRequestBus::Events::AddDoubleToObject)
                ->Event("AddUnsigned64ToObject", &MetastreamRequestBus::Events::AddUnsigned64ToObject)
                ->Event("AddSigned64ToObject", &MetastreamRequestBus::Events::AddSigned64ToObject)
                ->Event("AddEntityIdToObject", &MetastreamRequestBus::Events::AddEntityIdToObject)
                ;
        }
    }


    MetastreamGem::MetastreamGem()
        : m_serverEnabled(0)
        , m_serverOptionsCVar(nullptr)
    {
        m_descriptors.push_back(MetastreamReflectComponent::CreateDescriptor());

        // Initialise the cache
        m_cache = std::unique_ptr<DataCache>(new DataCache());

        MetastreamRequestBus::Handler::BusConnect();
    }
    MetastreamGem::~MetastreamGem()
    {
        MetastreamRequestBus::Handler::BusDisconnect();
    }

    void MetastreamGem::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
    {
        using namespace Metastream;

        switch (event)
        {
        case ESYSTEM_EVENT_GAME_POST_INIT:
            // All other Gems will exist at this point, for a full list of civet options, see https://github.com/civetweb/civetweb/blob/master/docs/UserManual.md
            // 
            // Note: 1) For security reasons, the option "enable_directory_listing" is forced to "no".
            //       2) The following options are ignored for security reasons: enable_directory_listing, cgi_interpreter, run_as_user, put_delete_auth_file
            //       3) Options are a set of key=value separated by the semi colon character. If a option needs to use the ';' or '='
            //          then use $semi or $equ.
            //
            m_serverOptionsCVar = REGISTER_STRING("metastream_serveroptions", "document_root=Gems/Metastream/Files;listening_ports=8082", VF_NULL, "Metastream HTTP Server options");

            REGISTER_CVAR2("metastream_enabled", &m_serverEnabled, 0, VF_READONLY, "State of the Metastream HTTP server (READONLY)");

            REGISTER_COMMAND("metastream_start", StartHTTPServerCmd, 0, "Starts the Metastream HTTP server");
            REGISTER_COMMAND("metastream_stop", StopHTTPServerCmd, 0, "Stops the Metastream HTTP server");

            break;

        case ESYSTEM_EVENT_FULL_SHUTDOWN:
        case ESYSTEM_EVENT_FAST_SHUTDOWN:
            // Put your shutdown code here
            // Other Gems may have been shutdown already, but none will have destructed
            if (m_server.get())
            {
                m_server->Stop();
                m_serverEnabled = 0;
            }
            break;
        }
    }

    void MetastreamGem::AddStringToCache(const char* table, const char* key, const char* value)
    {
        if (m_cache.get())
        {
            m_cache->AddToCache(table, key, value);
        }
    }

    void MetastreamGem::AddBoolToCache(const char* table, const char* key, bool value)
    {
        if (m_cache.get())
        {
            m_cache->AddToCache(table, key, value);
        }
    }

    void MetastreamGem::AddVec3ToCache(const char* table, const char* key, const Vec3 & value)
    {
        if (m_cache.get())
        {
            m_cache->AddToCache(table, key, AZ::Vector3(value.x, value.y, value.z));
        }
    }

    void MetastreamGem::AddDoubleToCache(const char* table, const char* key, double value)
    {
        if (m_cache.get())
        {
            m_cache->AddToCache(table, key, value);
        }
    }

    void MetastreamGem::AddUnsigned64ToCache(const char* table, const char* key, AZ::u64 value)
    {
        if (m_cache.get())
        {
            m_cache->AddToCache(table, key, value);
        }
    }

    void MetastreamGem::AddSigned64ToCache(const char* table, const char* key, AZ::s64 value)
    {
        if (m_cache.get())
        {
            m_cache->AddToCache(table, key, value);
        }
    }

    void MetastreamGem::AddArrayToCache(const char* table, const char* key, const char* arrayName)
    {
        if (m_cache.get())
        {
            m_cache->AddArrayToCache(table, key, arrayName);
        }
    }

    void MetastreamGem::AddObjectToCache(const char* table, const char* key, const char* objectName)
    {
        if (m_cache.get())
        {
            m_cache->AddObjectToCache(table, key, objectName);
        }
    }

    void MetastreamGem::AddStringToArray(const char* table, const char* arrayName, const char* value)
    {
        if (m_cache.get())
        {
            m_cache->AddToArray(table, arrayName, value);
        }
    }

    void MetastreamGem::AddBoolToArray(const char* table, const char* arrayName, bool value)
    {
        if (m_cache.get())
        {
            m_cache->AddToArray(table, arrayName, value);
        }
    }

    void MetastreamGem::AddVec3ToArray(const char* table, const char* arrayName, const Vec3 & value)
    {
        if (m_cache.get())
        {
            m_cache->AddToArray(table, arrayName, AZStd::string::format("[%f,%f,%f]", value.x, value.y, value.z).c_str());
        }
    }

    void MetastreamGem::AddDoubleToArray(const char* table, const char* arrayName, double value)
    {
        if (m_cache.get())
        {
            m_cache->AddToArray(table, arrayName, value);
        }
    }

    void MetastreamGem::AddUnsigned64ToArray(const char* table, const char* arrayName, AZ::u64 value)
    {
        if (m_cache.get())
        {
            m_cache->AddToArray(table, arrayName, value);
        }
    }

    void MetastreamGem::AddSigned64ToArray(const char* table, const char* arrayName, AZ::s64 value)
    {
        if (m_cache.get())
        {
            m_cache->AddToArray(table, arrayName, value);
        }
    }

    void MetastreamGem::AddArrayToObject(const char* table, const char* destObjectName, const char *key, const char *srcArrayName)
    {
        if (m_cache.get())
        {
            m_cache->AddArrayToObject(table, destObjectName, key, srcArrayName);
        }
    }

    void MetastreamGem::AddObjectToObject(const char* table, const char* destObjectName, const char *key, const char* sourceObjectName)
    {
        if (m_cache.get())
        {
            m_cache->AddObjectToObject(table, destObjectName, key, sourceObjectName);
        }
    }

    void MetastreamGem::AddObjectToArray(const char* table, const char* destArrayName, const char* sourceObjectName)
    {
        if (m_cache.get())
        {
            m_cache->AddObjectToArray(table, destArrayName, sourceObjectName);
        }
    }


    void MetastreamGem::AddStringToObject(const char *table, const char* objectName, const char* key, const char* value)
    {
        if (m_cache.get())
        {
            m_cache->AddToObject(table, objectName, key, value);
        }
    }

    void MetastreamGem::AddBoolToObject(const char* table, const char* objectName, const char* key, bool value)
    {
        if (m_cache.get())
        {
            m_cache->AddToObject(table, objectName, key, value);
        }
    }

    void MetastreamGem::AddVec3ToObject(const char *table, const char* objectName, const char* key, const Vec3 & value)
    {
        if (m_cache.get())
        {
            m_cache->AddToObject(table, objectName, key, AZ::Vector3(value.x, value.y, value.z));
        }
    }

    void MetastreamGem::AddDoubleToObject(const char *table, const char* objectName, const char* key, double value)
    {
        if (m_cache.get())
        {
            m_cache->AddToObject(table, objectName, key, value);
        }
    }

    void MetastreamGem::AddUnsigned64ToObject(const char *table, const char* objectName, const char* key, AZ::u64 value)
    {
        if (m_cache.get())
        {
            m_cache->AddToObject(table, objectName, key, value);
        }
    }

    void MetastreamGem::AddSigned64ToObject(const char *table, const char* objectName, const char* key, AZ::s64 value)
    {
        if (m_cache.get())
        {
            m_cache->AddToObject(table, objectName, key, value);
        }
    }


    bool MetastreamGem::StartHTTPServer()
    {
        if (!m_serverOptionsCVar)
        {
            return false;
        }

#if AZ_TRAIT_METASTREAM_USE_CIVET
        // Start server if it is not started
        if (!m_server.get())
        {
            // Initialise and start the HTTP server
            m_server = std::unique_ptr<BaseHttpServer>(new CivetHttpServer(m_cache.get()));
            AZStd::string serverOptions = m_serverOptionsCVar->GetString();
            CryLogAlways("Initializing Metastream: Options=\"%s\"", serverOptions.c_str());

            bool result = m_server->Start(serverOptions.c_str());
            if (result)
            {
                m_serverEnabled = 1;
            }

            return result;
        }
        else
        {
            // Server already started
            return true;
        }
#else
        return false;
#endif
    }

    void Metastream::MetastreamGem::StopHTTPServer()
    {
        // Stop server if it is started
        if (m_server.get())
        {
            m_server->Stop();
            m_server.reset();

            ClearCache();
            m_serverEnabled = 0;
        }
    }

    void Metastream::MetastreamGem::StartHTTPServerCmd(IConsoleCmdArgs* /*args*/)
    {
        EBUS_EVENT(Metastream::MetastreamRequestBus, StartHTTPServer);
    }

    void Metastream::MetastreamGem::StopHTTPServerCmd(IConsoleCmdArgs* /*args*/)
    {
        EBUS_EVENT(Metastream::MetastreamRequestBus, StopHTTPServer);
    }

    // Unit test methods to get at m_cache and status
    bool Metastream::MetastreamGem::IsServerEnabled() const
    {
        return (m_serverEnabled == 1);
    }

    std::string Metastream::MetastreamGem::GetDatabasesJSON() const
    {
        if (m_cache)
        {
            return m_cache->GetDatabasesJSON();
        }
        return "";
    }

    std::string Metastream::MetastreamGem::GetTableKeysJSON(const std::string& tableName) const
    {
        if (m_cache)
        {
            return m_cache->GetTableKeysJSON(tableName);
        }
        return "";
    }

    bool Metastream::MetastreamGem::ClearCache()
    {
        if (m_cache)
        {
            m_cache->ClearCache();
            return true;
        }
        return false;
    }

}

AZ_DECLARE_MODULE_CLASS(Gem_Metastream, Metastream::MetastreamGem)
