/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Locale.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/IO/GenericStreams.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/PoseDataFactory.h>
#include <EMotionFX/Source/TransformData.h>
#include <CsvSerializers.h>
#include <FeatureSchema.h>

namespace EMotionFX::MotionMatching
{
    CsvWriterBase::~CsvWriterBase()
    {
        End();
    }

    bool CsvWriterBase::OpenFile(const char* filename, int openMode)
    {
        m_file.Close();
        if (!m_file.Open(filename, openMode))
        {
            End();
            return false;
        }

        return true;
    }

    void CsvWriterBase::End()
    {
        m_file.Close();
        m_tempBuffer.clear();
    }

    void CsvWriterBase::WriteLine(AZStd::string& line)
    {
        line = AZ::StringFunc::RStrip(line, ",");
        line += "\n";
        m_file.Write(line.data(), line.size());
    }

    void CsvWriterBase::WriteVector3ToString(const AZ::Vector3& vec, AZStd::string& text)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // ensure that %f is read/written using the "C" locale.

        text += AZStd::string::format("%.8f,%.8f,%.8f,", vec.GetX(), vec.GetY(), vec.GetZ());
    };

    void CsvWriterBase::WriteFloatArrayToString(const AZStd::vector<float>& values, AZStd::string& text)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // ensure that %f is read/written using the "C" locale.

        text.reserve(text.size() + values.size() * 10);
        for (float value : values)
        {
            text += AZStd::string::format("%.8f,", value);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool PoseWriterCsv::Begin(const char* filename, ActorInstance* actorInstance, const WriteSettings& writeSettings)
    {
        m_settings = writeSettings;

        if (!actorInstance)
        {
            return false;
        }

        if (!CsvWriterBase::OpenFile(filename, AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            return false;
        }

        m_actorInstance = actorInstance;

        SaveColumnNamesToString(m_tempBuffer);
        return true;
    }

    void PoseWriterCsv::SaveColumnNamesToString(AZStd::string& outText)
    {
        const Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
        const size_t numEnabledJoints = m_actorInstance->GetNumEnabledNodes();
        outText.clear();
        outText.reserve(50 * numEnabledJoints);

        auto SaveVector3ColumnNames = [](AZStd::string& text, const char* jointName, const char* vecName)
        {
            text += AZStd::string::format("%s.%s.X,%s.%s.Y,%s.%s.Z,", jointName, vecName, jointName, vecName, jointName, vecName);
        };

        for (size_t i = 0; i < numEnabledJoints; ++i)
        {
            const size_t jointIndex = m_actorInstance->GetEnabledNode(i);
            const Node* joint = skeleton->GetNode(jointIndex);

            // Position
            if (m_settings.m_writePositions)
            {
                SaveVector3ColumnNames(outText, joint->GetName(), "Pos");
            }

            // Rotation
            if (m_settings.m_writeRotations)
            {
                SaveVector3ColumnNames(outText, joint->GetName(), "RotBasisX");
                SaveVector3ColumnNames(outText, joint->GetName(), "RotBasisY");
            }
        }

        WriteLine(m_tempBuffer);
    }

    void PoseWriterCsv::WritePose(Pose& pose, const ETransformSpace transformSpace)
    {
        if (!IsReady() || !m_actorInstance)
        {
            return;
        }

        SavePoseToString(pose, transformSpace, m_tempBuffer);
        WriteLine(m_tempBuffer);
    }

    void PoseWriterCsv::SavePoseToString(Pose& pose, const ETransformSpace transformSpace, AZStd::string& outText)
    {
        const size_t numEnabledJoints = m_actorInstance->GetNumEnabledNodes();
        outText.clear();
        outText.reserve(10 * 3 * 3 * numEnabledJoints);

        for (size_t i = 0; i < numEnabledJoints; ++i)
        {
            const size_t jointIndex = m_actorInstance->GetEnabledNode(i);

            Transform transform = Transform::CreateIdentity();
            switch (transformSpace)
            {
            case TRANSFORM_SPACE_LOCAL:
                {
                    transform = pose.GetLocalSpaceTransform(jointIndex);
                    break;
                }
            case TRANSFORM_SPACE_MODEL:
                {
                    transform = pose.GetModelSpaceTransform(jointIndex);
                    break;
                }
            default:
                {
                    AZ_Error("Motion Matching", false, "Unsupported transform space");
                    break;
                }
            }

            // Position
            if (m_settings.m_writePositions)
            {
                const AZ::Vector3 position = transform.m_position;
                WriteVector3ToString(position, outText);
            }

            // Rotation
            // Store rotation as the X and Y axes The Z axis can be reconstructed by the cross product of the X and Y axes.
            if (m_settings.m_writeRotations)
            {
                const AZ::Quaternion rotation = transform.m_rotation;
                AZ::Matrix3x3 rotationMatrix = AZ::Matrix3x3::CreateFromQuaternion(rotation);
                WriteVector3ToString(rotationMatrix.GetBasisX().GetNormalizedSafe(), outText);
                WriteVector3ToString(rotationMatrix.GetBasisY().GetNormalizedSafe(), outText);
            }
        }
    }

    void PoseWriterCsv::End()
    {
        m_actorInstance = nullptr;
        CsvWriterBase::End();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool QueryVectorWriterCsv::Begin(const char* filename, const FeatureSchema* featureSchema)
    {
        if (!featureSchema)
        {
            return false;
        }

        if (!CsvWriterBase::OpenFile(filename, AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            return false;
        }

        // Save column names in the first row
        const AZStd::vector<AZStd::string> columnNames = featureSchema->CollectColumnNames();
        m_tempBuffer.clear();
        if (!columnNames.empty())
        {
            for (size_t i = 0; i < columnNames.size(); ++i)
            {
                if (i != 0)
                {
                    m_tempBuffer += ",";
                }

                m_tempBuffer += columnNames[i];
            }
        }
        WriteLine(m_tempBuffer);

        return true;
    }

    void QueryVectorWriterCsv::Write(const QueryVector* queryVector)
    {
        if (!IsReady() || !queryVector)
        {
            return;
        }

        m_tempBuffer.clear();
        WriteFloatArrayToString(queryVector->GetData(), m_tempBuffer);
        WriteLine(m_tempBuffer);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool BestMatchingFrameWriterCsv::Begin(const char* filename)
    {
        if (!CsvWriterBase::OpenFile(filename, AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            return false;
        }

        m_tempBuffer = "Best Matching Frames";
        WriteLine(m_tempBuffer);
        return true;
    }

    void BestMatchingFrameWriterCsv::Write(size_t frame)
    {
        m_tempBuffer = AZStd::to_string(frame);
        WriteLine(m_tempBuffer);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    PoseReaderCsv::~PoseReaderCsv()
    {
        End();
    }

    bool PoseReaderCsv::Begin(const char* filename, const ReadSettings& readSettings)
    {
        m_settings = readSettings;

        AZ::IO::SystemFile file;
        if (!file.Open(filename, AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
        {
            return false;
        }

        AZ::IO::SystemFile::SizeType fileSize = file.Length();
        AZStd::string textBuffer;
        textBuffer.resize(fileSize);
        file.Read(fileSize, textBuffer.data());
        file.Close();

        AZStd::vector<AZStd::string> lines;
        AZ::StringFunc::Tokenize(textBuffer, lines, '\n');
        if (lines.empty())
        {
            return false;
        }

        m_columnNamesLine = lines[0];
        lines.erase(lines.begin());
        m_poseValueLines = lines;

        return true;
    }

    void PoseReaderCsv::ApplyPose(ActorInstance* actorInstance, Pose& pose, const ETransformSpace transformSpace, size_t index)
    {
        AZStd::vector<AZStd::string> valueTokens;
        AZ::StringFunc::Tokenize(m_poseValueLines[index], valueTokens, ',');

        Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();

        size_t valueIndex = 0;
        const size_t numEnabledJoints = actorInstance->GetNumEnabledNodes();
        for (size_t i = 0; i < numEnabledJoints; ++i)
        {
            const size_t jointIndex = actorInstance->GetEnabledNode(i);

            Transform transform = bindPose->GetLocalSpaceTransform(jointIndex);
            switch (transformSpace)
            {
            case TRANSFORM_SPACE_LOCAL:
                {
                    transform = pose.GetLocalSpaceTransform(jointIndex);
                    break;
                }
            case TRANSFORM_SPACE_MODEL:
                {
                    transform = pose.GetModelSpaceTransform(jointIndex);
                    break;
                }
            default:
                {
                    AZ_Error("Motion Matching", false, "Unsupported transform space");
                    break;
                }
            }

            auto LoadVector3FromString = [&valueTokens](size_t& valueIndex, AZ::Vector3& outVec)
            {
                outVec.SetX(AZStd::stof(valueTokens[valueIndex + 0]));
                outVec.SetY(AZStd::stof(valueTokens[valueIndex + 1]));
                outVec.SetZ(AZStd::stof(valueTokens[valueIndex + 2]));
                valueIndex += 3;
            };

            // Position
            if (m_settings.m_readPositions)
            {
                LoadVector3FromString(valueIndex, transform.m_position);
            }

            // Rotation
            if (m_settings.m_readRotations)
            {
                // Load the X and Y axes.
                AZ::Vector3 basisX = AZ::Vector3::CreateZero();
                AZ::Vector3 basisY = AZ::Vector3::CreateZero();
                LoadVector3FromString(valueIndex, basisX);
                LoadVector3FromString(valueIndex, basisY);
                basisX.NormalizeSafe();
                basisY.NormalizeSafe();

                // Create a 3x3 rotation matrix by the X and Y axes and construct the Z-axis as the
                // cross-product of the X and Y axes.
                AZ::Matrix3x3 rotationMatrix = AZ::Matrix3x3::CreateIdentity();
                rotationMatrix.SetBasisX(basisX);
                rotationMatrix.SetBasisY(basisY);
                rotationMatrix.SetBasisZ(basisX.Cross(basisY));

                // Convert the rotation matrix to a quaternion.
                transform.m_rotation = AZ::Quaternion::CreateFromMatrix3x3(rotationMatrix);
            }

            switch (transformSpace)
            {
            case TRANSFORM_SPACE_LOCAL:
                {
                    pose.SetLocalSpaceTransform(jointIndex, transform);
                    break;
                }
            case TRANSFORM_SPACE_MODEL:
                {
                    pose.SetModelSpaceTransform(jointIndex, transform);
                    break;
                }
            default:
                {
                    AZ_Error("Motion Matching", false, "Unsupported transform space");
                    break;
                }
            }
        }
    }

    void PoseReaderCsv::End()
    {
        m_columnNamesLine.clear();
        m_poseValueLines.clear();
    }
} // namespace EMotionFX::MotionMatching
