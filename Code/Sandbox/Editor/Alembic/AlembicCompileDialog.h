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

#ifndef CRYINCLUDE_EDITOR_ALEMBIC_ALEMBICCOMPILEDIALOG_H
#define CRYINCLUDE_EDITOR_ALEMBIC_ALEMBICCOMPILEDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>

#include <IXml.h>

#endif

namespace Ui
{
    class AlembicCompileDialog;
}

class CAlembicCompileDialog
    : public QDialog
{
    Q_OBJECT
public:
    CAlembicCompileDialog(const XmlNodeRef config);
    ~CAlembicCompileDialog();

    void OnInitDialog();

    QString GetUpAxis() const;
    QString GetPlaybackFromMemory() const;
    QString GetBlockCompressionFormat() const;
    QString GetMeshPrediction() const;
    QString GetUseBFrames() const;
    uint GetIndexFrameDistance() const;
    double GetPositionPrecision() const;
    float GetUVmax() const;


private:
    struct SConfig
    {
        SConfig()
            : m_upAxis("Y")
            , m_playbackFromMemory("0")
            , m_blockCompressionFormat("deflate")
            , m_meshPrediction("1")
            , m_useBFrames("1")
            , m_indexFrameDistance(10)
            , m_positionPrecision(1.0)
            , m_uvMax(1.0f) {}

        bool operator ==(const SConfig& other) const
        {
            return m_blockCompressionFormat == other.m_blockCompressionFormat
                   && m_upAxis == other.m_upAxis
                   && m_playbackFromMemory == other.m_playbackFromMemory
                   && m_meshPrediction == other.m_meshPrediction
                   && m_useBFrames == other.m_useBFrames
                   && m_indexFrameDistance == other.m_indexFrameDistance
                   && m_positionPrecision == other.m_positionPrecision
                   && m_uvMax == other.m_uvMax;
        }

        QString m_name;
        QString m_blockCompressionFormat;
        QString m_upAxis;
        QString m_playbackFromMemory;
        QString m_meshPrediction;
        QString m_useBFrames;
        uint m_indexFrameDistance;
        double m_positionPrecision;
        float  m_uvMax;
    };

    void SetFromConfig(const SConfig& config);
    void UpdateEnabledStates();
    void UpdatePresetSelection();
    SConfig LoadConfig(const QString& fileName, XmlNodeRef xml) const;

    void OnRadioYUp();
    void OnRadioZUp();
    void OnPlaybackFromMemory();
    void OnBlockCompressionSelected();
    void OnMeshPredictionCheckBox();
    void OnUseBFramesCheckBox();
    void OnIndexFrameDistanceChanged();
    void OnPositionPrecisionChanged();
    void OnUVmaxChanged();
    void OnPresetSelected();

    SConfig m_config;
    std::vector<SConfig> m_presets;

    QScopedPointer<Ui::AlembicCompileDialog> m_ui;
};
#endif // CRYINCLUDE_EDITOR_ALEMBIC_ALEMBICCOMPILEDIALOG_H
