/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DebugOutput.h"

namespace AZ::SceneAPI::Utilities
{
    void DebugOutput::Write(const char* name, const char* data)
    {
        m_output += AZStd::string::format("\t%s: %s\n", name, data);
    }

    void DebugOutput::WriteArray(const char* name, const unsigned int* data, int size)
    {
        m_output += AZStd::string::format("\t%s: ", name);
        for (int index = 0; index < size; ++index)
        {
            m_output += AZStd::string::format("%d, ", data[index]);
        }
        m_output += AZStd::string::format("\n");
    }

    void DebugOutput::Write(const char* name, const AZStd::string& data)
    {
        Write(name, data.c_str());
    }

    void DebugOutput::Write(const char* name, double data)
    {
        m_output += AZStd::string::format("\t%s: %f\n", name, data);
    }

    void DebugOutput::Write(const char* name, uint64_t data)
    {
        m_output += AZStd::string::format("\t%s: %" PRIu64 "\n", name, data);
    }

    void DebugOutput::Write(const char* name, int64_t data)
    {
        m_output += AZStd::string::format("\t%s: %" PRId64 "\n", name, data);
    }

    void DebugOutput::Write(const char* name, const DataTypes::MatrixType& data)
    {
        AZ::Vector3 basisX{};
        AZ::Vector3 basisY{};
        AZ::Vector3 basisZ{};
        AZ::Vector3 translation{};
        data.GetBasisAndTranslation(&basisX, &basisY, &basisZ, &translation);

        m_output += AZStd::string::format("\t%s:\n", name);

        m_output += "\t";
        Write("BasisX", basisX);

        m_output += "\t";
        Write("BasisY", basisY);

        m_output += "\t";
        Write("BasisZ", basisZ);

        m_output += "\t";
        Write("Transl", translation);
    }

    void DebugOutput::Write(const char* name, bool data)
    {
        m_output += AZStd::string::format("\t%s: %s\n", name, data ? "true" : "false");
    }

    void DebugOutput::Write(const char* name, Vector3 data)
    {
        m_output += AZStd::string::format("\t%s: <% f, % f, % f>\n", name, data.GetX(), data.GetY(), data.GetZ());
    }

    void DebugOutput::Write(const char* name, AZStd::optional<bool> data)
    {
        if (data.has_value())
        {
            Write(name, data.value());
        }
        else
        {
            Write(name, "Not set");
        }
    }
    void DebugOutput::Write(const char* name, AZStd::optional<float> data)
    {
        if (data.has_value())
        {
            Write(name, data.value());
        }
        else
        {
            Write(name, "Not set");
        }
    }

    void DebugOutput::Write(const char* name, AZStd::optional<AZ::Vector3> data)
    {
        if (data.has_value())
        {
            Write(name, data.value());
        }
        else
        {
            Write(name, "Not set");
        }
    }

    const AZStd::string& DebugOutput::GetOutput() const
    {
        return m_output;
    }
}
