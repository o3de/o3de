/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#ifndef GEM_MULTIPLAYER_MULTIPLAYERUTILS_H
#define GEM_MULTIPLAYER_MULTIPLAYERUTILS_H

#include <AzCore/std/string/string.h>
#include <GridMate/GridMate.h>
#include <GridMate/Session/Session.h>
#include <GridMate/Session/LANSession.h>

#include <GridMate/Carrier/Driver.h>

namespace Platform
{
    void InitCarrierDesc(GridMate::CarrierDesc& carrierDesc);
}


#pragma push_macro("max")   // files included through gridmate undef max, which causes later compile issues for modules that have std:max in their header

#ifdef NET_SUPPORT_SECURE_SOCKET_DRIVER
#   include <GridMate/Carrier/SecureSocketDriver.h>
#endif

#pragma pop_macro("max")    // restore previous disabling of max

#include <Multiplayer/IMultiplayerGem.h>
#include <CertificateManager/ICertificateManagerGem.h>

namespace Multiplayer
{
    struct Utils
    {
        static GridMate::Driver::BSDSocketFamilyType CVarToFamilyType(const char* str)
        {
            AZ_Assert(str, "Invalid value");

            if (!azstricmp(str, "IPv4"))
            {
                return GridMate::Driver::BSD_AF_INET;
            }
            else if (!azstricmp(str, "IPv6"))
            {
                return GridMate::Driver::BSD_AF_INET6;
            }
            else
            {
                AZ_Warning("GridMate", false, "Invalid value '%s' for ip version", str);
                return GridMate::Driver::BSD_AF_INET;
            }
        }

        static void InitCarrierDesc(GridMate::CarrierDesc& carrierDesc)
        {
            if (!carrierDesc.m_simulator)
            {
                EBUS_EVENT_RESULT(carrierDesc.m_simulator,Multiplayer::MultiplayerRequestBus,GetSimulator);
            }

            carrierDesc.m_port = gEnv->pConsole->GetCVar("cl_clientport")->GetIVal();
            carrierDesc.m_connectionTimeoutMS = 10000;
            carrierDesc.m_threadUpdateTimeMS = 30;
            carrierDesc.m_threadInstantResponse = true;
            carrierDesc.m_driverIsCrossPlatform = true;
            carrierDesc.m_securityData = gEnv->pConsole->GetCVar("gm_securityData")->GetString();
            carrierDesc.m_familyType = CVarToFamilyType(gEnv->pConsole->GetCVar("gm_ipversion")->GetString());
            carrierDesc.m_version = gEnv->pConsole->GetCVar("gm_version")->GetIVal();

            ApplyDisconnectDetectionSettings(carrierDesc);

            Platform::InitCarrierDesc(carrierDesc);
        }

        static void ApplyDisconnectDetectionSettings(GridMate::CarrierDesc& carrierDesc)
        {
            carrierDesc.m_enableDisconnectDetection = !!gEnv->pConsole->GetCVar("gm_disconnectDetection")->GetIVal();

            if (gEnv->pConsole->GetCVar("gm_disconnectDetectionRttThreshold"))
            {
                carrierDesc.m_disconnectDetectionRttThreshold = gEnv->pConsole->GetCVar("gm_disconnectDetectionRttThreshold")->GetFVal();
            }

            if (gEnv->pConsole->GetCVar("gm_disconnectDetectionPacketLossThreshold"))
            {
                carrierDesc.m_disconnectDetectionPacketLossThreshold = gEnv->pConsole->GetCVar("gm_disconnectDetectionPacketLossThreshold")->GetFVal();
            }
        }

        // Synchronize session state and load map if sv_map is passed via parameters
        static void SynchronizeSessionState(const GridMate::GridSession* session)
        {
            if (session && session->IsHost())
            {
                AZStd::string mapName;

                // Push any session parameters we may have received back to any matching CVars.
                for (unsigned int i = 0; i < session->GetNumParams(); ++i)
                {
                    if (strcmp(session->GetParam(i).m_id.c_str(),"sv_map") == 0)
                    {
                        mapName = session->GetParam(i).m_value.c_str();
                    }
                    else
                    {
                        ICVar* var = gEnv->pConsole->GetCVar(session->GetParam(i).m_id.c_str());
                        if (var)
                        {
                            var->Set(session->GetParam(i).m_value.c_str());
                        }
                        else
                        {
                            CryLogAlways("Unable to bind session property '%s:%s' to CVar. CVar does not exist.", session->GetParam(i).m_id.c_str(), session->GetParam(i).m_value.c_str());
                        }
                    }
                }                

                if (!mapName.empty())
                {
                    // If we have an actual level to load, load it.
                    AZStd::string loadCommand = "map ";
                    loadCommand += mapName;
                    gEnv->pConsole->ExecuteString(loadCommand.c_str(), false, true);
                }
            }
        }    
    };

    struct LAN
    {
        static void StartSessionService(GridMate::IGridMate* gridMate)
        {
            if (!GridMate::HasGridMateService<GridMate::LANSessionService>(gridMate))
            {
                GridMate::StartGridMateService<GridMate::LANSessionService>(gridMate, GridMate::SessionServiceDesc());
            }
        }

        static void StopSessionService(GridMate::IGridMate* gridMate)
        {
            GridMate::StopGridMateService<GridMate::LANSessionService>(gridMate);
        }
    };
    
    struct NetSec
    {
        friend class MultiplayerCVars;
        friend class MultiplayerModule;

        static bool CanCreateSecureSocketForHosting()
        {
            bool hasPublicKey = false;
            bool hasPrivateKey = false;

            EBUS_EVENT_RESULT(hasPublicKey,CertificateManager::CertificateManagerRequestsBus,HasPublicKey);
            EBUS_EVENT_RESULT(hasPrivateKey,CertificateManager::CertificateManagerRequestsBus,HasPrivateKey);

            return hasPublicKey && hasPrivateKey;
        }

        static bool CanCreateSecureSocketForJoining()
        {
            bool hasCertificateAuthority = false;
            EBUS_EVENT_RESULT(hasCertificateAuthority,CertificateManager::CertificateManagerRequestsBus,HasCertificateAuthority);

            return hasCertificateAuthority;
        }

        static void ConfigureCarrierDescForHost([[maybe_unused]] GridMate::CarrierDesc& carrierDesc)
        {
            bool netSecEnabled = false;
            EBUS_EVENT_RESULT(netSecEnabled, Multiplayer::MultiplayerRequestBus,IsNetSecEnabled);

            if (netSecEnabled)
            {
#if defined(NET_SUPPORT_SECURE_SOCKET_DRIVER)  
                if (CanCreateSecureSocketForHosting())
                {
                    GridMate::SecureSocketDesc desc;

                    EBUS_EVENT_RESULT(desc.m_privateKeyPEM,CertificateManager::CertificateManagerRequestsBus,RetrievePrivateKey);
                    EBUS_EVENT_RESULT(desc.m_certificatePEM,CertificateManager::CertificateManagerRequestsBus,RetrievePublicKey);                    

                    bool verifyClient = false;
                    EBUS_EVENT_RESULT(verifyClient,Multiplayer::MultiplayerRequestBus,IsNetSecVerifyClient);
                    desc.m_authenticateClient = verifyClient;

                    GridMate::SecureSocketDriver* secureDriver = aznew GridMate::SecureSocketDriver(desc);
                    EBUS_EVENT(Multiplayer::MultiplayerRequestBus,RegisterSecureDriver,secureDriver);

                    carrierDesc.m_driver = secureDriver;
                }
                else
                {
                    CryLog("Unable to use a secure connection because of missing certificate or private key.");
                }
#endif
            }
        }

        static void ConfigureCarrierDescForJoin([[maybe_unused]] GridMate::CarrierDesc& carrierDesc)
        {
            bool netSecEnabled = false;
            EBUS_EVENT_RESULT(netSecEnabled, Multiplayer::MultiplayerRequestBus,IsNetSecEnabled);
            if (netSecEnabled)
            {
#if defined(NET_SUPPORT_SECURE_SOCKET_DRIVER)  
                GridMate::SecureSocketDesc desc;                

                if (CanCreateSecureSocketForJoining())
                {
                    EBUS_EVENT_RESULT(desc.m_certificateAuthorityPEM ,CertificateManager::CertificateManagerRequestsBus,RetrieveCertificateAuthority);

                    GridMate::SecureSocketDriver* secureDriver = aznew GridMate::SecureSocketDriver(desc);
                    EBUS_EVENT(Multiplayer::MultiplayerRequestBus,RegisterSecureDriver,secureDriver);

                    carrierDesc.m_driver = secureDriver;
                }
                else
                {
                    CryLog("Unable to use a secure connection because of missing certificate or private key.");
                }
#endif
            }
        }

        static void OnSessionFailedToCreate([[maybe_unused]] GridMate::CarrierDesc& carrierDesc)
        {            
            bool netSecEnabled = false;
            EBUS_EVENT_RESULT(netSecEnabled, Multiplayer::MultiplayerRequestBus,IsNetSecEnabled);
            if (netSecEnabled)
            {
#if defined(NET_SUPPORT_SECURE_SOCKET_DRIVER)  
                // Clean up unused secure socket driver.
                delete carrierDesc.m_driver;
                carrierDesc.m_driver = nullptr;

                EBUS_EVENT(Multiplayer::MultiplayerRequestBus,RegisterSecureDriver, nullptr);
#endif
            }
        }

    private:
        static int s_NetsecEnabled;
        static int s_NetsecVerifyClient;
    };    
}

#endif
