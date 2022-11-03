/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/SystemFile.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/TransformSpace.h>

namespace EMotionFX
{
    class ActorInstance;
};

namespace EMotionFX::MotionMatching
{
    class FeatureSchema;
    class QueryVector;

    //! Base class providing some helpers for saving data to CSV files
    class EMFX_API CsvWriterBase
    {
    public:
        virtual ~CsvWriterBase();

    protected:
        bool OpenFile(const char* filename, int openMode);
        virtual void End();
        bool IsReady() const { return m_file.IsOpen(); }

        void WriteLine(AZStd::string& line);
        void WriteVector3ToString(const AZ::Vector3& vec, AZStd::string& text);
        void WriteFloatArrayToString(const AZStd::vector<float>& values, AZStd::string& text);

    protected:
        AZ::IO::SystemFile m_file;
        AZStd::string m_tempBuffer;
    };

    //! Store a list of skeletal poses in a table
    //! The first row contains the value component names, e.g. "LeftArm.Position.X"
    //! Each following row represents a skeletal pose
    //! Position and rotation values for all enabled joints are stored (scale is skipped).
    //! Position XYZ is stored in 3x columns. Rotation which internally is stored as a quaternion
    //! is converted to a rotation matrix and the XY-axes of it are stored as 6x components/columns.
    //! To reconstruct the rotation quaternion, we first need the cross-product of the X and Y axes to
    //! get the Z axis, create a rotation matrix from that and then convert it back to a quaternion.
    class EMFX_API PoseWriterCsv
        : public CsvWriterBase
    {
    public:
        virtual ~PoseWriterCsv() = default;

        struct WriteSettings
        {
            bool m_writePositions = true;
            bool m_writeRotations = true;
        };

        bool Begin(const char* filename, ActorInstance* actorInstance, const WriteSettings& writeSettings);
        void WritePose(Pose& pose, const ETransformSpace transformSpace);
        void End() override;

    private:
        void SaveColumnNamesToString(AZStd::string& outText);
        void SavePoseToString(Pose& pose, const ETransformSpace transformSpace, AZStd::string& outText);

        ActorInstance* m_actorInstance = nullptr;
        WriteSettings m_settings;
    };

    //! Store a list of query vectors in a table
    //! The first row contains the names of the features in the very vector
    //! based on the currently used feature schema
    //! Each following row represents a query vector
    class EMFX_API QueryVectorWriterCsv
        : public CsvWriterBase
    {
    public:
        virtual ~QueryVectorWriterCsv() = default;

        bool Begin(const char* filename, const FeatureSchema* featureSchema);
        void Write(const QueryVector* queryVector);
    };

    //! Store a list of best matching frames in a table
    //! The first row contains the column name
    //! Each following row represents a best matching frame
    class EMFX_API BestMatchingFrameWriterCsv
        : public CsvWriterBase
    {
    public:
        virtual ~BestMatchingFrameWriterCsv() = default;

        bool Begin(const char* filename);
        void Write(size_t frame);
    };

    //! The counter-part to PoseWriterCsv which loads a CSV file containing poses and can apply
    //! them onto an actor instance.
    class EMFX_API PoseReaderCsv
    {
    public:
        ~PoseReaderCsv();

        struct ReadSettings
        {
            bool m_readPositions = true;
            bool m_readRotations = true;
        };

        bool Begin(const char* filename, const ReadSettings& readSettings);
        void ApplyPose(ActorInstance* actorInstance, Pose& pose, const ETransformSpace transformSpace, size_t index);
        size_t GetNumPoses() const { return m_poseValueLines.size(); }
        void End();

    private:
        AZStd::string m_columnNamesLine;
        AZStd::vector<AZStd::string> m_poseValueLines;
        ReadSettings m_settings;
    };
} // namespace EMotionFX::MotionMatching
