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
#include <AzCore/Serialization/SerializeContext.h>

#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>

namespace GraphCanvas
{
    /////////////////////////////
    // GraphCanvasMimeContainer
    /////////////////////////////
    
    void GraphCanvasMimeContainer::Reflect(AZ::ReflectContext* reflectContext)
    {    
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        
        if (serializeContext)
        {
            serializeContext->Class<GraphCanvasMimeContainer>()
                ->Version(0)
                ->Field("MimeEvents", &GraphCanvasMimeContainer::m_mimeEvents);
            ;
        }
    }    

    GraphCanvasMimeContainer::~GraphCanvasMimeContainer()
    {
        for (GraphCanvasMimeEvent* mimeEvent : m_mimeEvents)
        {
            delete mimeEvent;
        }
    }
    
    bool GraphCanvasMimeContainer::ToBuffer(AZStd::vector<char>& buffer)
    {
        buffer.clear();
        AZ::IO::ByteContainerStream<AZStd::vector<char> > ms(&buffer);
        return AZ::Utils::SaveObjectToStream(ms, AZ::DataStream::ST_BINARY, this);
    }
    
    bool GraphCanvasMimeContainer::FromBuffer(const char* data, AZStd::size_t size)
    {
        AZ::IO::MemoryStream ms(data, size);

        GraphCanvasMimeContainer* pContainer = AZ::Utils::LoadObjectFromStream<GraphCanvasMimeContainer>(ms, nullptr);
        if (pContainer)
        {
            m_mimeEvents = AZStd::move(pContainer->m_mimeEvents);

            pContainer->m_mimeEvents.clear();
            delete pContainer;

            return true;
        }

        return false;
    }

    bool GraphCanvasMimeContainer::FromBuffer(const AZStd::vector<char>& buffer)
    {
        return FromBuffer(buffer.data(), buffer.size());
    }
}
