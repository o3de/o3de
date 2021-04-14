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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "ResourceCompilerPC_precompiled.h"
#include "DataWriter.h"
#include "CryEndian.h"

DataWriter::DataWriter()
{
    m_outputBuffer = NULL;
    Reset();
}

DataWriter::~DataWriter()
{
    this->Reset();
}

void DataWriter::Reset()
{
    if (m_outputBuffer)
    {
        free(m_outputBuffer);
    }

    m_currentBufferSize = 0;
    m_outputBuffer = NULL;
    m_writtenBytes = 0;
    m_labelMap.clear();
    m_offsetLocations.clear();
    m_bClosed = false;
    m_bSwapEndian = false;
}

void DataWriter::SetSwapEndian(bool bEnable)
{
    m_bSwapEndian = bEnable;
}

void* DataWriter::GetDataAndTakeOwnership()
{
    if (m_bClosed && m_outputBuffer)
    {
        void* returnData = m_outputBuffer;
        m_outputBuffer = NULL;
        Reset();
        return returnData;
    }
    return NULL;
}

uint32 DataWriter::GetDataSize() const
{
    return (m_bClosed && m_outputBuffer) ? m_writtenBytes : 0;
}

void DataWriter::BeginWriting()
{
    ExpandBuffer(1);
    AddLabel("fileStart");
}

void DataWriter::WriteUInt8(const uint8 v)
{
    ExpandBuffer(sizeof(v));
    uint8* out = (uint8*)(((uint8*)m_outputBuffer) + m_writtenBytes);
    *out = v;
    SwapEndian(*out, m_bSwapEndian);
    m_writtenBytes += sizeof(v);
}

void DataWriter::WriteInt8(const int8 v)
{
    ExpandBuffer(sizeof(v));
    int8* out = (int8*)(((uint8*)m_outputBuffer) + m_writtenBytes);
    *out = v;
    SwapEndian(*out, m_bSwapEndian);
    m_writtenBytes += sizeof(v);
}

void DataWriter::WriteUInt16(const uint16 v)
{
    ExpandBuffer(sizeof(v));
    uint16* out = (uint16*)(((uint8*)m_outputBuffer) + m_writtenBytes);
    *out = v;
    SwapEndian(*out, m_bSwapEndian);
    m_writtenBytes += sizeof(v);
}

void DataWriter::WriteInt16(const int16 v)
{
    ExpandBuffer(sizeof(v));
    int16* out = (int16*)(((uint8*)m_outputBuffer) + m_writtenBytes);
    *out = v;
    SwapEndian(*out, m_bSwapEndian);
    m_writtenBytes += sizeof(v);
}

void DataWriter::WriteUInt32(const uint32 v)
{
    ExpandBuffer(sizeof(v));
    uint32* out = (uint32*)(((uint8*)m_outputBuffer) + m_writtenBytes);
    *out = v;
    SwapEndian(*out, m_bSwapEndian);
    m_writtenBytes += sizeof(v);
}

void DataWriter::WriteInt32(const int32 v)
{
    ExpandBuffer(sizeof(v));
    int32* out = (int32*)(((uint8*)m_outputBuffer) + m_writtenBytes);
    *out = v;
    SwapEndian(*out, m_bSwapEndian);
    m_writtenBytes += sizeof(v);
}

void DataWriter::WriteFloat(const float v)
{
    ExpandBuffer(sizeof(v));
    float* out = (float*)(((uint8*)m_outputBuffer) + m_writtenBytes);
    *out = v;
    SwapEndian(*out, m_bSwapEndian);
    m_writtenBytes += sizeof(v);
}

void DataWriter::WriteData(const void* v, const uint32 size)
{
    ExpandBuffer(size);
    void* out = (void*)(((uint8*)m_outputBuffer) + m_writtenBytes);
    memcpy(out, v, size);
    m_writtenBytes += size;
}

void DataWriter::WriteAlign(const uint32 alignBytes)
{
    const uint32 a = m_writtenBytes % alignBytes;
    if (a != 0)
    {
        m_writtenBytes += (alignBytes - a);
    }
    ExpandBuffer();
}

void DataWriter::AddLabel(const string& label)
{
    const std::map<string, uint32>::iterator f = m_labelMap.find(label);
    if (f == m_labelMap.end())
    {
        m_labelMap.insert(std::make_pair(label, m_writtenBytes));
    }
}

void DataWriter::WriteOffsetInt32(const string& label)
{
    this->WriteOffsetInt32("", label);
}

void DataWriter::WriteOffsetInt32(const string& fromLabel, const string& label)
{
    SOffsetLocation loc;
    loc.b16Bit = false;
    loc.offset = m_writtenBytes;
    loc.labelName = label;
    loc.fromLabelName = fromLabel;
    m_offsetLocations.push_back(loc);
    WriteInt32(0);   // Make space to write the offset into later
}

void DataWriter::WriteOffsetInt16(const string& label)
{
    this->WriteOffsetInt16("", label);
}

void DataWriter::WriteOffsetInt16(const string& fromLabel, const string& label)
{
    SOffsetLocation loc;
    loc.b16Bit = true;
    loc.offset = m_writtenBytes;
    loc.labelName = label;
    loc.fromLabelName = fromLabel;
    m_offsetLocations.push_back(loc);
    WriteInt16(0);   // Make space to write the offset into later
}

bool DataWriter::EndWriting()
{
    m_missingLabels.clear();

    for (int iOffset = 0; iOffset < m_offsetLocations.size(); iOffset++)
    {
        const SOffsetLocation* loc = &m_offsetLocations[iOffset];
        const std::map<string, uint32>::iterator itLabel = m_labelMap.find(loc->labelName);

        if (itLabel != m_labelMap.end())
        {
            const uint32 labelLoc = (*itLabel).second;
            uint32 fromLoc = 0;
            if (loc->fromLabelName.length() > 0)
            {
                const std::map<string, uint32>::iterator itFromLabel = m_labelMap.find(loc->fromLabelName);
                if (itFromLabel != m_labelMap.end())
                {
                    fromLoc = (*itFromLabel).second;
                }
                else
                {
                    m_missingLabels.push_back(loc->fromLabelName);
                }
            }

            if (loc->b16Bit)
            {
                const int16 offset = (int16)(labelLoc - fromLoc);
                int16* out = (int16*)(((uint8*)m_outputBuffer) + loc->offset);
                *out = offset;
                SwapEndian(*out, m_bSwapEndian);
            }
            else
            {
                const int32 offset = labelLoc - fromLoc;
                int32* out = (int32*)(((uint8*)m_outputBuffer) + loc->offset);
                *out = offset;
                SwapEndian(*out, m_bSwapEndian);
            }
        }
        else
        {
            m_missingLabels.push_back(loc->labelName);
        }
    }

    m_bClosed = true;

    return m_missingLabels.empty();
}

uint32 DataWriter::CalculateSize(const string& fromLabel, const string& toLabel)
{
    const std::map<string, uint32>::iterator itFromLabel = m_labelMap.find(fromLabel);
    const std::map<string, uint32>::iterator itToLabel = m_labelMap.find(toLabel);

    uint32 size = 0;
    if (itFromLabel != m_labelMap.end() && itToLabel != m_labelMap.end())
    {
        const uint32 fromLoc = (*itFromLabel).second;
        const uint32 toLoc = (*itToLabel).second;
        if (fromLoc < toLoc)
        {
            size = toLoc - fromLoc;
        }
        else
        {
            size = 0;
        }
    }
    return size;
}

#define BUFFERINCREASESIZE (1024 * 1024)

void DataWriter::ExpandBuffer(const uint32 addBytes)
{
    if (m_writtenBytes + addBytes > m_currentBufferSize)
    {
        const uint32 newBufferSize = m_currentBufferSize + BUFFERINCREASESIZE;
        if (m_outputBuffer)
        {
            void* tmpBuffer = realloc(m_outputBuffer, newBufferSize);
            AZ_Assert(
                tmpBuffer != nullptr,
                "realloc failed, this is possible when allocating a large data array whose size is comparable to RAM size, and also when "
                "the memory is highly segmented");
            if (tmpBuffer)
            {
                m_outputBuffer = tmpBuffer;
                m_currentBufferSize = newBufferSize;
            }
        }
        else
        {
            m_outputBuffer = malloc(newBufferSize);
            m_currentBufferSize = newBufferSize;
        }
    }
}
