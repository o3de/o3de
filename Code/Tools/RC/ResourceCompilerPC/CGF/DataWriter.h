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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_DATAWRITER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_DATAWRITER_H
#pragma once

class DataWriter
{
public:
    DataWriter();
    virtual ~DataWriter();

    void Reset();

    void SetSwapEndian(bool bEnable);

    uint32 GetDataSize() const;
    void* GetDataAndTakeOwnership();

    void BeginWriting();

    void WriteUInt8(const uint8 v);
    void WriteInt8(const int8 v);
    void WriteUInt16(const uint16 v);
    void WriteInt16(const int16 v);
    void WriteUInt32(const uint32 v);
    void WriteInt32(const int32 v);
    void WriteFloat(const float v);

    void WriteData(const void* v, const uint32 size);

    void WriteAlign(const uint32 alignBytes);

    void AddLabel(const string& label);
    void WriteOffsetInt32(const string& label);
    void WriteOffsetInt32(const string& fromLabel, const string& label);
    void WriteOffsetInt16(const string& label);
    void WriteOffsetInt16(const string& fromLabel, const string& label);

    bool EndWriting();

    uint32 CalculateSize(const string& fromLabel, const string& toLabel);

private:

    void ExpandBuffer(const uint32 addBytes = 0);

    uint32 m_currentBufferSize;
    void* m_outputBuffer;
    uint32 m_writtenBytes;
    bool m_bClosed;
    bool m_bSwapEndian;

    std::map<string, uint32> m_labelMap;

    struct SOffsetLocation
    {
        bool b16Bit;
        uint32 offset;
        string labelName;
        string fromLabelName;
    };

    std::vector< SOffsetLocation > m_offsetLocations;
    std::vector<string> m_missingLabels;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGF_DATAWRITER_H
