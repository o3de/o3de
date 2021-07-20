/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Driller/Driller.h>
#include <AzCore/Driller/DrillerBus.h>

namespace ScriptCanvas
{
    class Graph;
    class Node;

    namespace Profiler
    {
        class DrillerInterface
            : public AZ::Debug::DrillerEBusTraits
        {
        public:

            // On Entry
            // On Execute/Upate
            // On Exit
            virtual void OnNodeExecute(Node*) = 0;

        };

        using DrillerBus = AZ::EBus<DrillerInterface>;

        class DrillerCommandInterface
            : public AZ::Debug::DrillerEBusTraits
        {
        public:

            virtual Graph* RequestDrilledGraph() = 0;
        };

        using DrillerCommandBus = AZ::EBus<DrillerCommandInterface>;

        class Driller
            : public AZ::Debug::Driller
            , public DrillerBus::Handler
        {
            bool m_isDetailedCapture;
            
            Graph* m_drilledGraph;

            using ParamArrayType = AZStd::vector<Param>;
            ParamArrayType m_params;
        
        public:

            AZ_CLASS_ALLOCATOR(Driller, AZ::SystemAllocator, 0);

            const char*  GroupName() const override { return "ScriptCanvasDrillers"; }
            const char*  GetName() const override { return "ScriptCanvasGraphDriller"; }
            const char*  GetDescription() const override { return "Drilling the Script Canvas execution engine"; }
            int          GetNumParams() const override { return static_cast<int>(m_params.size()); }
            const Param* GetParam(int index) const override { return &m_params[index]; }

            Driller()
                : m_isDetailedCapture(false)
                , m_drilledGraph(nullptr)
            {
                Param isDetailed;
                isDetailed.desc = "IsDetailedDrill";
                isDetailed.name = AZ_CRC("IsDetailedDrill", 0x2155cef2);
                isDetailed.type = Param::PT_BOOL;
                isDetailed.value = 0;
                m_params.push_back(isDetailed);
            }

            void Start(const Param* params = nullptr, int numParams = 0) override
            {
                m_isDetailedCapture = m_params[0].value != 0;
                if (params)
                {
                    for (int i = 0; i < numParams; i++)
                    {
                        if (params[i].name == m_params[0].name)
                        {
                            m_isDetailedCapture = params[i].value != 0;
                        }
                    }
                }

                DrillerCommandBus::BroadcastResult(m_drilledGraph, &DrillerCommandInterface::RequestDrilledGraph);
//                AZ_TEST_ASSERT(m_drilledGraph != nullptr); /// Make sure we have our object by the time we started the driller

                m_output->BeginTag(AZ_CRC("ScriptCanvasGraphDriller", 0xb161ccb2));
                m_output->Write(AZ_CRC("OnStart", 0x8b372fca), m_isDetailedCapture);
                // write drilled object initial state
                m_output->EndTag(AZ_CRC("ScriptCanvasGraphDriller", 0xb161ccb2));

                BusConnect();
            }

            void Stop() override
            {
                //m_drilledGraph = nullptr;
                //m_output->BeginTag(AZ_CRC("ScriptCanvasGraphDriller"));
                //m_output->Write(AZ_CRC("OnStop"), m_isDetailedCapture);
                //m_output->EndTag(AZ_CRC("ScriptCanvasGraphDriller"));
                //BusDisconnect();
            }

            void OnNodeExecute(Node* node) override;
        };


    }
}

