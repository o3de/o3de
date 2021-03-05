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
#include "ChatPlay_precompiled.h"

#include <sstream>

#include <AzCore/std/smart_ptr/shared_ptr.h>

#include "WebSocketStream.h"

WebSocketStream::WebSocketStream(const char* address, const AZStd::shared_ptr<IStreamHandler>& streamHandler)
    : m_address(address)
{
    m_streamHandler = streamHandler;

    auto framedSend = [&](const char* message, size_t messageLength){
        this->SendMessageFromStream(message, messageLength);
    };
    m_streamHandler->SetSendFunction(framedSend);

    m_handshook = false;
}

IStreamHandler::HandlerState WebSocketStream::OnConnect()
{
    if (m_send)
    {
        // Initiate WebSocket handshake
        AZStd::string httpReq(PrepareWebSocketHeader());

        m_send(httpReq.c_str(), httpReq.length());
        return IStreamHandler::HandlerState::AWAITING_RESPONSE;
    }

    return IStreamHandler::HandlerState::HANDLER_ERROR;
}

IStreamHandler::HandlerState WebSocketStream::OnMessage(const char* message, size_t messageLength)
{
    if (!m_handshook)
    {
        // Look for handshake response
        if (AZStd::string_view{ message, messageLength }.find(RPL_SERVER_HANDSHAKE) != AZStd::string_view::npos)
        {
            m_handshook = true;

            // Connected; pass event through if a IStreamHandler was provided
            if (m_streamHandler)
            {
                return m_streamHandler->OnConnect();
            }
            else
            {
                return IStreamHandler::HandlerState::CONNECTED;
            }
        }
        else if (AZStd::string_view{ message, messageLength }.find(" 400 Bad Request") != AZStd::string_view::npos)
        {
            return IStreamHandler::HandlerState::HANDLER_ERROR;
        }
    }
    else
    {
        char* payload = new char[messageLength];
        size_t payloadLength = 0;
        IStreamHandler::HandlerState ret = IStreamHandler::HandlerState::MESSAGE_RECEIVED;

        // Decode the WebsSocket-framed message
        FrameType type = WebSocketStream::getFrame(message, messageLength, payload, messageLength, &payloadLength);

        if (type == TEXT_FRAME && payloadLength > 0)
        {
            // Pass message on to attached IStreamHandler
            if (m_streamHandler)
            {
                ret = m_streamHandler->OnMessage(payload, payloadLength);
            }
        }

        delete[] payload;
        return ret;
    }

    return IStreamHandler::HandlerState::UNHANDLED_RESPONSE;
}

bool WebSocketStream::SendMessage(const char* input, size_t inputLength)
{
    if (m_streamHandler)
    {
        return m_streamHandler->SendMessage(input, inputLength);
    }

    size_t payloadLength = WebSocketStream::frameSize(inputLength, true);

    char* payload = new char[payloadLength];
    WebSocketStream::makeFrame(TEXT_FRAME, input, inputLength, true, payload, payloadLength);

    if (m_handshook && m_send)
    {
        m_send(payload, payloadLength);
        return true;
    }

    return false;
}

bool WebSocketStream::SendMessageFromStream(const char* input, size_t inputLength)
{
    size_t payloadLength = WebSocketStream::frameSize(inputLength, true);

    char* payload = new char[payloadLength];
    WebSocketStream::makeFrame(TEXT_FRAME, input, inputLength, true, payload, payloadLength);

    if (m_handshook && m_send)
    {
        m_send(payload, payloadLength);
        return true;
    }

    return false;
}

/******************************************************************************/

AZStd::string WebSocketStream::PrepareWebSocketHeader()
{
    std::stringstream httpReq;
    httpReq << "GET / HTTP/1.1" << "\r\n";
    httpReq << "Host: " << m_address << "\r\n";
    httpReq << "Connection: keep-alive, Upgrade" << "\r\n";
    httpReq << "Upgrade: websocket" << "\r\n";
    httpReq << "Sec-WebSocket-Key: c3VoYWliJ3Mgc3BlY2lhbCBzdHJpbmc=" << "\r\n";
    httpReq << "Sec-WebSocket-Version: 13" << "\r\n";
    httpReq << "\r\n";

    return AZStd::string(httpReq.str().c_str());
}

size_t WebSocketStream::frameSize(size_t msgLength, bool mask)
{
    size_t frameLength = 2; // 1 byte for FIN, RSV1-3, and OPCODE; 1 byte for MASK and PAYLOAD LENGTH

    if (msgLength > 125 && msgLength <= 65535)
    {
        frameLength += 2; // 2 bytes for 16-bit EXTENDED PAYLOAD LENGTH
    }
    else if (msgLength > 65535)
    {
        frameLength += 8; // 8 bytes for 64-bit EXTENDED PAYLOAD LENGTH
    }

    if (mask)
    {
        frameLength += 4; // 4 bytes for MASKING KEY
    }

    return frameLength + msgLength;
}

size_t WebSocketStream::makeFrame(FrameType frameType, const char* msg, size_t msgLength, bool mask, char* buffer, size_t bufferLength)
{
    if (bufferLength < WebSocketStream::frameSize(msgLength, mask))
    {
        // buffer is not long enough
        return 0;
    }

    size_t pos = 0;
    size_t size = msgLength;
    buffer[pos++] = (char)frameType;

    if (size <= 125)
    {
        buffer[pos++] = size | 0x80; // 0x80 to set mask flag to 1
    }
    else if (size <= 65535)
    {
        buffer[pos++] = (char) (126 | 0x80); // 16 bit length follows; mask flag set to 1

        buffer[pos++] = (size >> 8) & 0xFF; // leftmost first
        buffer[pos++] = size & 0xFF;
    }
    else // >2^16-1 (65535)
    {
        buffer[pos++] = (char) (127 | 0x80); // 64 bit length follows; mask flag set to 1

        // write 8 bytes length (significant first)
        for (int i = 7; i >= 0; i--)
        {
            buffer[pos++] = ((size >> 8 * i) & 0xFF);
        }
    }

    size_t maskPos = pos;

    if (mask)
    {
        buffer[pos++] = static_cast<char>(rand());
        buffer[pos++] = static_cast<char>(rand());
        buffer[pos++] = static_cast<char>(rand());
        buffer[pos++] = static_cast<char>(rand());
    }

    memcpy((void*)(buffer + pos), msg, size);

    if (mask)
    {
        for (int i = 0; i < size; ++i)
        {
            buffer[pos + i] ^= buffer[maskPos + i % 4];
        }
    }

    return (size + pos);
}

WebSocketStream::FrameType WebSocketStream::getFrame(const char* rawInBuffer, size_t inLength, char* buffer, size_t bufferLength, size_t* outLength)
{
    unsigned char* inBuffer = new unsigned char[inLength];
    memcpy(inBuffer, rawInBuffer, inLength);
    
    //printf("getTextFrame()\n");
    if (inLength < 3) return INCOMPLETE_FRAME;

    char frameOpcode = inBuffer[0] & 0x0F;
    char frameFin = (inBuffer[0] >> 7) & 0x01;
    char frameMasked = (inBuffer[1] >> 7) & 0x01;

    // *** message decoding 

    size_t payloadLength = 0;

    int pos = 2;
    int length_field = inBuffer[1] & (~0x80);
    unsigned int mask = 0;

    if (length_field <= 125)
    {
        payloadLength = length_field;
    }
    else if (length_field == 126) // 16-bit PAYLOAD LENGTH
    {
        payloadLength = (inBuffer[2] << 8) + inBuffer[3];
        pos += 2;
    }
    else if (length_field == 127) // 64-bit PAYLOAD LENGTH
    {
        for (int i = 0; i < 8; ++i)
        {
            payloadLength = (payloadLength << 8) + inBuffer[pos + i];
        }
        pos += 8;
    }

    if (inLength < payloadLength + pos) {
        return INCOMPLETE_FRAME;
    }

    // note: messages from server should never be masked
    if (frameMasked) {
        mask = *((unsigned int*)(inBuffer + pos));
        //printf("MASK: %08x\n", mask);
        pos += 4;

        // unmask data:
        unsigned char* c = inBuffer + pos;
        for (int i = 0; i<payloadLength; i++) {
            c[i] = c[i] ^ ((char*)(&mask))[i % 4];
        }
    }

    if (payloadLength > bufferLength) {
        return ERROR_FRAME;
    }

    memcpy((void*)buffer, (void*)(inBuffer + pos), payloadLength);
    buffer[payloadLength] = 0;
    *outLength = payloadLength + 1;


    if (frameOpcode == 0x0) return (frameFin) ? TEXT_FRAME : INCOMPLETE_TEXT_FRAME;
    if (frameOpcode == 0x1) return (frameFin) ? TEXT_FRAME : INCOMPLETE_TEXT_FRAME;
    if (frameOpcode == 0x2) return (frameFin) ? BINARY_FRAME : INCOMPLETE_BINARY_FRAME;
    if (frameOpcode == 0x9) return PING_FRAME;
    if (frameOpcode == 0xA) return PONG_FRAME;

    return ERROR_FRAME;
}
