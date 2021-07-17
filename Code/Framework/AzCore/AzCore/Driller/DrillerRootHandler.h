/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_DRILLER_ROOT_HANDLER_H
#define AZCORE_DRILLER_ROOT_HANDLER_H

#include <AzCore/Driller/Stream.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    namespace Debug
    {
        // Please check DrillerRootHandler class... this is the one for direct use.

        /**
         * Handler for the <Frame><StartData><Driller></Driller></StartData></Frame> tag.
         */
        class DrillerDrillerdataHandler
            : public DrillerHandlerParser
        {
        public:
            class ParamHandler
                : public DrillerHandlerParser
            {
            public:
                virtual void                    OnData(const DrillerSAXParser::Data& dataNode)
                {
                    if (dataNode.m_name == AZ_CRC("Name", 0x5e237e06))
                    {
                        dataNode.Read(m_param->name);
                    }
                    else if (dataNode.m_name == AZ_CRC("Description", 0x6de44026))
                    {
                        m_param->desc = NULL;                   // ignored
                    }
                    else if (dataNode.m_name == AZ_CRC("Type", 0x8cde5729))
                    {
                        dataNode.Read(m_param->type);
                    }
                    else if (dataNode.m_name == AZ_CRC("Value", 0x1d775834))
                    {
                        dataNode.Read(m_param->value);
                    }
                }
                Driller::Param*     m_param;
            };

            virtual DrillerHandlerParser*   OnEnterTag(u32 tagName)
            {
                if (tagName == AZ_CRC("Param", 0xa4fa7c89))
                {
                    m_drillerInfo->params.push_back();
                    m_paramHandler.m_param = &m_drillerInfo->params.back();
                    return &m_paramHandler;
                }
                return NULL;
            }

            virtual void                    OnData(const DrillerSAXParser::Data& dataNode)
            {
                if (dataNode.m_name == AZ_CRC("Name", 0x5e237e06))
                {
                    dataNode.Read(m_drillerInfo->id);
                }
            }

            DrillerManager::DrillerInfo*    m_drillerInfo;
            ParamHandler    m_paramHandler;
        };


        /**
         * Handler for the <Frame><StartData></StartData></Frame> tag
         */
        class DrillerStartdataHandler
            : public DrillerHandlerParser
        {
        public:
            virtual DrillerHandlerParser*   OnEnterTag(u32 tagName)
            {
                if (tagName == AZ_CRC("Driller", 0xa6e1fb73))
                {
                    m_drillers.push_back();
                    m_drillerDataHandler.m_drillerInfo = &m_drillers.back();
                    return &m_drillerDataHandler;
                }
                return NULL;
            }
            virtual void                    OnData(const DrillerSAXParser::Data& dataNode)
            {
                if (dataNode.m_name == AZ_CRC("Platform", 0x3952d0cb))
                {
                    dataNode.Read(m_platform);
                }
            }

            unsigned int                        m_platform;
            DrillerManager::DrillerListType     m_drillers;
            DrillerDrillerdataHandler           m_drillerDataHandler;
        };

        /**
         * Handler for the <Frame></Frame> tag
         */
        template<class DrillerContainer>
        class FrameHandler
            : public DrillerHandlerParser
        {
        public:
            FrameHandler()
                : DrillerHandlerParser(DrillerContainer::s_isWarnOnMissingDrillers)
                , m_currentFrame(-1) {}

            virtual DrillerHandlerParser*   OnEnterTag(u32 tagName) { return m_drillersContainer.FindDrillerHandler(tagName); }
            virtual void                    OnData(const DrillerSAXParser::Data& dataNode)
            {
                if (dataNode.m_name == AZ_CRC("FrameNum", 0x85a1a919))
                {
                    dataNode.Read(m_currentFrame);
                }
            }
            DrillerContainer m_drillersContainer;
            int              m_currentFrame;
        };

        /**
         * Use this class a input parameter to DrillerSAXParserHandler::DrillerSAXParserHandler(). It will handle all root level
         * tags for a standard driller input stream stream.
         *
         * DrillerContainer should comply to the following requirements:
         * - default constructible
         * - has a static const bool s_isWarnOnMissingDrillers member to indicate if you want to
         * trigger a warning when a driller is not found in the class.
         * - implement a function DrillerHandlerParser* DrillerContainer::FindDrillerHandler(u32 drillerName)
         *
         */
        template<class DrillerContainer>
        class DrillerRootHandler
            : public DrillerHandlerParser
        {
        public:
            DrillerContainer*               GetDrillerContainer()   { return m_frameHandler.m_drillersContainer; }

            virtual DrillerHandlerParser*   OnEnterTag(u32 tagName)
            {
                if (tagName == AZ_CRC("StartData", 0xecf3f53f))
                {
                    return &m_drillerSessionInfo;
                }
                if (tagName == AZ_CRC("Frame", 0xb5f83ccd))
                {
                    return &m_frameHandler;
                }
                return NULL;
            }
            DrillerStartdataHandler         m_drillerSessionInfo;
            FrameHandler<DrillerContainer>  m_frameHandler;
        };
    } // namespace Debug
} // namespace AZ

#endif // AZCORE_DRILLER_ROOT_HANDLER_H
#pragma once


