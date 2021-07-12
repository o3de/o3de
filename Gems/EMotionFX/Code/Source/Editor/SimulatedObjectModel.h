/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/RTTI/TypeInfo.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Node.h>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QItemSelectionModel>
#include <Editor/ActorEditorBus.h>
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <QIcon>
#endif


namespace EMotionFX
{
    class Skeleton;
    class SimulatedCommon;
    class SimulatedObject;
    class SimulatedJoint;

    // Simulated object model
    // Columns: Node Name
    class SimulatedObjectModel
        : public QAbstractItemModel
    {
        Q_OBJECT // AUTOMOC

    public:
        enum ColumnIndex
        {
            COLUMN_NAME
        };

        enum Role
        {
            ROLE_OBJECT_PTR = Qt::UserRole,
            ROLE_OBJECT_INDEX,
            ROLE_OBJECT_NAME,
            ROLE_JOINT_PTR,
            ROLE_JOINT_BOOL,
            ROLE_ACTOR_PTR
        };

        SimulatedObjectModel();
        ~SimulatedObjectModel() override;

        // QAbstractItemModel override
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& child) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;

        const QItemSelectionModel* GetSelectionModel() const { return m_selectionModel; }
        QItemSelectionModel* GetSelectionModel() { return m_selectionModel; }

        const Actor* GetActor() const { return m_actor; }

        void SetActor(Actor* actor);
        void SetActorInstance(ActorInstance* actorInstance);

        QModelIndex GetModelIndexByObjectIndex(size_t objectIndex);
        QModelIndex FindModelIndex(SimulatedObject* object);
        void AddJointsToSelection(QItemSelection& selection, size_t objectIndex, const AZStd::vector<AZ::u32>& jointIndices);

    private:
        // Command callbacks.
        void RegisterCommandCallbacks();
        void PreAddObject();
        void PostAddObject();
        void PreRemoveObject(size_t objectIndex);
        void PostRemoveObject();
        void PostObjectChanged();

        // Callbacks
#define SIMULATEDOBJECTMODEL_CALLBACK(CLASSNAME)                                                                             \
    class CLASSNAME                                                                                                          \
        : public MCore::Command::Callback                                                                                    \
    {                                                                                                                        \
    public:                                                                                                                  \
        explicit CLASSNAME(SimulatedObjectModel* simulatedObjectModel, bool executePreUndo = false, bool executePreCommand = false)   \
            : MCore::Command::Callback(executePreUndo, executePreCommand)                                                    \
            , m_simulatedObjectModel(simulatedObjectModel)                                                                   \
        {}                                                                                                                   \
        bool Execute(MCore::Command * command, const MCore::CommandLine & commandLine) override;                             \
        bool Undo(MCore::Command * command, const MCore::CommandLine & commandLine) override;                                \
    private:                                                                                                                 \
        SimulatedObjectModel* m_simulatedObjectModel;                                                                        \
    };                                                                                                                       \
    friend class CLASSNAME;
        SIMULATEDOBJECTMODEL_CALLBACK(CommandAddSimulatedObjectPreCallback);
        SIMULATEDOBJECTMODEL_CALLBACK(CommandAddSimulatedObjectPostCallback);
        SIMULATEDOBJECTMODEL_CALLBACK(CommandRemoveSimulatedObjectPreCallback);
        SIMULATEDOBJECTMODEL_CALLBACK(CommandRemoveSimulatedObjectPostCallback);
        SIMULATEDOBJECTMODEL_CALLBACK(CommandAdjustSimulatedObjectPostCallback);
        SIMULATEDOBJECTMODEL_CALLBACK(CommandAddSimulatedJointsPreCallback);
        SIMULATEDOBJECTMODEL_CALLBACK(CommandAddSimulatedJointsPostCallback);
        SIMULATEDOBJECTMODEL_CALLBACK(CommandRemoveSimulatedJointsPreCallback);
        SIMULATEDOBJECTMODEL_CALLBACK(CommandRemoveSimulatedJointsPostCallback);
        SIMULATEDOBJECTMODEL_CALLBACK(CommandAdjustSimulatedJointPostCallback);

        static int s_columnCount;
        static const char* s_simulatedObjectIconPath;

        void InitModel(Actor* actor);

        AZStd::vector<MCore::Command::Callback*> m_commandCallbacks;

        Skeleton* m_skeleton = nullptr;
        Actor* m_actor = nullptr;
        ActorInstance* m_actorInstance = nullptr;
        QItemSelectionModel* m_selectionModel = nullptr;

        QIcon m_objectIcon;
    };
} // namespace EMotionFX
