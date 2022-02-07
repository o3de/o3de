/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Skeleton.h>
#include <Source/Editor/SkeletonModel.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>


namespace EMotionFX
{
    int SkeletonModel::s_defaultIconSize = 16;
    int SkeletonModel::s_columnCount = 7;
    const char* SkeletonModel::s_jointIconPath = ":/EMotionFX/Joint.svg";
    const char* SkeletonModel::s_clothColliderIconPath = ":/EMotionFX/Cloth.svg";
    const char* SkeletonModel::s_hitDetectionColliderIconPath = ":/EMotionFX/HitDetection.svg";
    const char* SkeletonModel::s_ragdollColliderIconPath = ":/EMotionFX/RagdollCollider.svg";
    const char* SkeletonModel::s_ragdollJointLimitIconPath = ":/EMotionFX/RagdollJointLimit.svg";
    const char* SkeletonModel::s_simulatedJointIconPath = ":/EMotionFX/SimulatedObjectColored.svg";
    const char* SkeletonModel::s_simulatedColliderIconPath = ":/EMotionFX/SimulatedObjectCollider.svg";

    SkeletonModel::SkeletonModel()
        : m_selectionModel(this)
        , m_skeleton(nullptr)
        , m_actor(nullptr)
        , m_actorInstance(nullptr)
        , m_jointIcon(s_jointIconPath)
        , m_clothColliderIcon(s_clothColliderIconPath)
        , m_hitDetectionColliderIcon(s_hitDetectionColliderIconPath)
        , m_ragdollColliderIcon(s_ragdollColliderIconPath)
        , m_ragdollJointLimitIcon(s_ragdollJointLimitIconPath)
        , m_simulatedJointIcon(s_simulatedJointIconPath)
        , m_simulatedColliderIcon(s_simulatedColliderIconPath)
    {
        m_selectionModel.setModel(this);

        ActorEditorNotificationBus::Handler::BusConnect();

        ActorInstance* selectedActorInstance = nullptr;
        ActorEditorRequestBus::BroadcastResult(selectedActorInstance, &ActorEditorRequests::GetSelectedActorInstance);
        if (selectedActorInstance)
        {
            SetActorInstance(selectedActorInstance);
        }
        else
        {
            Actor* selectedActor = nullptr;
            ActorEditorRequestBus::BroadcastResult(selectedActor, &ActorEditorRequests::GetSelectedActor);
            SetActor(selectedActor);
        }

        // UI 2.0 styling paints icons white on row selection. It cannot discern between layers though, so everything that is non-transparent is filled with white.
        // As a temp solution, we specify the selected icon.
        m_jointIcon.addFile(s_jointIconPath, QSize(), QIcon::Selected);
        m_clothColliderIcon.addFile(s_clothColliderIconPath, QSize(), QIcon::Selected);
        m_hitDetectionColliderIcon.addFile(s_hitDetectionColliderIconPath, QSize(), QIcon::Selected);
        m_ragdollColliderIcon.addFile(s_ragdollColliderIconPath, QSize(), QIcon::Selected);
        m_ragdollJointLimitIcon.addFile(s_ragdollJointLimitIconPath, QSize(), QIcon::Selected);
        m_simulatedJointIcon.addFile(s_simulatedJointIconPath, QSize(), QIcon::Selected);
        m_simulatedColliderIcon.addFile(s_simulatedColliderIconPath, QSize(), QIcon::Selected);
    }

    SkeletonModel::~SkeletonModel()
    {
        // Calling Reset will trigger the modelReset signal, thus notify other widgets to clear cached information about this model.
        Reset();

        ActorEditorNotificationBus::Handler::BusDisconnect();
    }

    void SkeletonModel::SetActor(Actor* actor)
    {
        beginResetModel();

        m_actorInstance = nullptr;
        m_actor = actor;
        m_skeleton = nullptr;
        if (m_actor)
        {
            m_skeleton = actor->GetSkeleton();
        }
        UpdateNodeInfos(m_actor);

        endResetModel();
    }

    void SkeletonModel::SetActorInstance(ActorInstance* actorInstance)
    {
        beginResetModel();

        m_actorInstance = actorInstance;
        m_actor = nullptr;
        m_skeleton = nullptr;
        if (m_actorInstance)
        {
            m_actor = actorInstance->GetActor();
            m_skeleton = m_actor->GetSkeleton();
        }
        UpdateNodeInfos(m_actor);

        endResetModel();
    }

    QModelIndex SkeletonModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!m_skeleton)
        {
            AZ_Assert(false, "Cannot get model index. Skeleton invalid.");
            return QModelIndex();
        }

        if (parent.isValid())
        {
            const Node* parentNode = static_cast<const Node*>(parent.internalPointer());

            if (row >= static_cast<int>(parentNode->GetNumChildNodes()))
            {
                AZ_Assert(false, "Cannot get model index. Row out of range.");
                return QModelIndex();
            }

            const size_t childNodeIndex = parentNode->GetChildIndex(row);
            Node* childNode = m_skeleton->GetNode(childNodeIndex);
            return createIndex(row, column, childNode);
        }
        else
        {
            if (row >= static_cast<int>(m_skeleton->GetNumRootNodes()))
            {
                AZ_Assert(false, "Cannot get model index. Row out of range.");
                return QModelIndex();
            }

            const size_t rootNodeIndex = m_skeleton->GetRootNodeIndex(row);
            Node* rootNode = m_skeleton->GetNode(rootNodeIndex);
            return createIndex(row, column, rootNode);
        }
    }

    QModelIndex SkeletonModel::parent(const QModelIndex& child) const
    {
        if (!m_skeleton)
        {
            AZ_Assert(false, "Cannot get parent model index. Skeleton invalid.");
            return QModelIndex();
        }

        AZ_Assert(child.isValid(), "Expected valid child model index.");
        Node* childNode = static_cast<Node*>(child.internalPointer());
        Node* parentNode = childNode->GetParentNode();
        if (parentNode)
        {
            Node* grandParentNode = parentNode->GetParentNode();
            if (grandParentNode)
            {
                const int numChildNodes = aznumeric_caster(grandParentNode->GetNumChildNodes());
                for (int i = 0; i < numChildNodes; ++i)
                {
                    const Node* grandParentChildNode = m_skeleton->GetNode(grandParentNode->GetChildIndex(i));
                    if (grandParentChildNode == parentNode)
                    {
                        return createIndex(i, 0, parentNode);
                    }
                }
            }
            else
            {
                const int numRootNodes = aznumeric_caster(m_skeleton->GetNumRootNodes());
                for (int i = 0; i < numRootNodes; ++i)
                {
                    const Node* rootNode = m_skeleton->GetNode(m_skeleton->GetRootNodeIndex(i));
                    if (rootNode == parentNode)
                    {
                        return createIndex(i, 0, parentNode);
                    }
                }
            }
        }

        return QModelIndex();
    }

    int SkeletonModel::rowCount(const QModelIndex& parent) const
    {
        if (!m_skeleton)
        {
            return 0;
        }

        if (parent.isValid())
        {
            const Node* parentNode = static_cast<const Node*>(parent.internalPointer());
            return static_cast<int>(parentNode->GetNumChildNodes());
        }
        else
        {
            return static_cast<int>(m_skeleton->GetNumRootNodes());
        }
    }

    int SkeletonModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return s_columnCount;
    }

    QVariant SkeletonModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section)
            {
            case COLUMN_NAME:
                return "Name";
            default:
                return "";
            }
        }

        return QVariant();
    }

    QVariant SkeletonModel::data(const QModelIndex& index, int role) const
    {
        if (!m_skeleton || !index.isValid())
        {
            AZ_Assert(false, "Cannot get model data. Skeleton or model index invalid.");
            return QVariant();
        }

        Node* node = static_cast<Node*>(index.internalPointer());
        AZ_Assert(node, "Expected valid node pointer.");

        const size_t nodeIndex = node->GetNodeIndex();
        const NodeInfo& nodeInfo = m_nodeInfos[nodeIndex];

        switch (role)
        {
        case Qt::ToolTipRole:
        {
            switch (index.column())
            {
            case COLUMN_RAGDOLL_LIMIT:
            {
                return QString("Ragdoll Limit");
            }
            case COLUMN_CLOTH_COLLIDERS:
            {
                return QString("Cloth Colliders");
            }
            case COLUMN_HITDETECTION_COLLIDERS:
            {
                return QString("Hit Detection Colliders");
            }
            case COLUMN_RAGDOLL_COLLIDERS:
            {
                return QString("Ragdoll Colliders");
            }
            case COLUMN_SIMULATED_JOINTS:
            {
                return QString("Simulated Joints");
            }
            case COLUMN_SIMULATED_COLLIDERS:
            {
                return QString("Simulated Colliders");
            }
            default:
                break;
            }
            break;
        }
        case Qt::DisplayRole:
        {
            switch (index.column())
            {
            case COLUMN_NAME:
                return node->GetName();
            default:
                break;
            }
            break;
        }
        case Qt::CheckStateRole:
        {
            if (index.column() == 0 && nodeInfo.m_checkable)
            {
                return nodeInfo.m_checkState;
            }
            break;
        }
        case Qt::DecorationRole:
        {
            switch (index.column())
            {
            case COLUMN_NAME:
            {
                return m_jointIcon;
            }
            case COLUMN_RAGDOLL_LIMIT:
            {
                const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = m_actor->GetPhysicsSetup();
                if (physicsSetup)
                {
                    const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();
                    const Physics::RagdollNodeConfiguration* ragdollNodeConfig = ragdollConfig.FindNodeConfigByName(node->GetName());
                    if (ragdollNodeConfig)
                    {
                        return m_ragdollJointLimitIcon;
                    }
                }
                break;
            }
            case COLUMN_RAGDOLL_COLLIDERS:
            {
                const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = m_actor->GetPhysicsSetup();
                if (physicsSetup)
                {
                    const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();
                    Physics::CharacterColliderNodeConfiguration* ragdollColliderNodeConfig = ragdollConfig.m_colliders.FindNodeConfigByName(node->GetName());
                    if (ragdollColliderNodeConfig && !ragdollColliderNodeConfig->m_shapes.empty())
                    {
                        return m_ragdollColliderIcon;
                    }
                }
                break;
            }
            case COLUMN_HITDETECTION_COLLIDERS:
            {
                const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = m_actor->GetPhysicsSetup();
                if (physicsSetup)
                {
                    const Physics::CharacterColliderConfiguration& hitDetectionConfig = physicsSetup->GetHitDetectionConfig();
                    Physics::CharacterColliderNodeConfiguration* hitDetectionNodeConfig = hitDetectionConfig.FindNodeConfigByName(node->GetName());
                    if (hitDetectionNodeConfig && !hitDetectionNodeConfig->m_shapes.empty())
                    {
                        return m_hitDetectionColliderIcon;
                    }
                }
                break;
            }
            case COLUMN_CLOTH_COLLIDERS:
            {
                const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = m_actor->GetPhysicsSetup();
                if (physicsSetup)
                {
                    const Physics::CharacterColliderConfiguration& clothConfig = physicsSetup->GetClothConfig();
                    Physics::CharacterColliderNodeConfiguration* clothNodeConfig = clothConfig.FindNodeConfigByName(node->GetName());
                    if (clothNodeConfig && !clothNodeConfig->m_shapes.empty())
                    {
                        return m_clothColliderIcon;
                    }
                }
                break;
            }
            case COLUMN_SIMULATED_JOINTS:
            {
                const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = m_actor->GetSimulatedObjectSetup();
                if (simulatedObjectSetup)
                {
                    const AZStd::vector<SimulatedObject*> objects = simulatedObjectSetup->GetSimulatedObjects();
                    for (const SimulatedObject* object : objects)
                    {
                        if (object->FindSimulatedJointBySkeletonJointIndex(node->GetNodeIndex()))
                        {
                            return m_simulatedJointIcon;
                        }
                    }
                }
                break;
            }
            case COLUMN_SIMULATED_COLLIDERS:
            {
                const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = m_actor->GetPhysicsSetup();
                if (physicsSetup)
                {
                    const Physics::CharacterColliderConfiguration& simulatedColliderConfig = physicsSetup->GetSimulatedObjectColliderConfig();
                    Physics::CharacterColliderNodeConfiguration* simulatedColliderNodeConfig = simulatedColliderConfig.FindNodeConfigByName(node->GetName());
                    if (simulatedColliderNodeConfig && !simulatedColliderNodeConfig->m_shapes.empty())
                    {
                        return m_simulatedColliderIcon;
                    }
                }
                break;
            }
            }
            break;
        }
        case ROLE_NODE_INDEX:
            return qulonglong(nodeIndex);
        case ROLE_POINTER:
            return QVariant::fromValue(node);
        case ROLE_ACTOR_POINTER:
            return QVariant::fromValue(m_actor);
        case ROLE_ACTOR_INSTANCE_POINTER:
            return QVariant::fromValue(m_actorInstance);
        case ROLE_BONE:
            return nodeInfo.m_isBone;
        case ROLE_HASMESH:
            return nodeInfo.m_hasMesh;
        case ROLE_RAGDOLL:
        {
            const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = m_actor->GetPhysicsSetup();
            if (physicsSetup)
            {
                const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();
                const Physics::RagdollNodeConfiguration* ragdollNodeConfig = ragdollConfig.FindNodeConfigByName(node->GetName());
                return ragdollNodeConfig != nullptr;
            }
        }
        case ROLE_HITDETECTION:
        {
            const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = m_actor->GetPhysicsSetup();
            if (physicsSetup)
            {
                const Physics::CharacterColliderConfiguration& hitDetectionConfig = physicsSetup->GetHitDetectionConfig();
                Physics::CharacterColliderNodeConfiguration* hitDetectionNodeConfig = hitDetectionConfig.FindNodeConfigByName(node->GetName());
                return (hitDetectionNodeConfig && !hitDetectionNodeConfig->m_shapes.empty());
            }
        }
        case ROLE_CLOTH:
        {
            const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = m_actor->GetPhysicsSetup();
            if (physicsSetup)
            {
                const Physics::CharacterColliderConfiguration& clothConfig = physicsSetup->GetClothConfig();
                Physics::CharacterColliderNodeConfiguration* clothNodeConfig = clothConfig.FindNodeConfigByName(node->GetName());
                return (clothNodeConfig && !clothNodeConfig->m_shapes.empty());
            }
        }
        case ROLE_SIMULATED_JOINT:
        {
            const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = m_actor->GetSimulatedObjectSetup();
            if (simulatedObjectSetup)
            {
                const AZStd::vector<SimulatedObject*>& objects = simulatedObjectSetup->GetSimulatedObjects();
                const auto found = AZStd::find_if(objects.begin(), objects.end(), [node](const SimulatedObject* object)
                {
                    return object->FindSimulatedJointBySkeletonJointIndex(node->GetNodeIndex());
                });
                return found != objects.end();
            }
        }
        case ROLE_SIMULATED_OBJECT_COLLIDER:
        {
            const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = m_actor->GetPhysicsSetup();
            if (physicsSetup)
            {
                // TODO: Get the data from simulated collider setup.
                const Physics::CharacterColliderConfiguration& simulatedObjectConfig = physicsSetup->GetSimulatedObjectColliderConfig();
                Physics::CharacterColliderNodeConfiguration* simulatedObjectNodeConfig = simulatedObjectConfig.FindNodeConfigByName(node->GetName());
                return (simulatedObjectNodeConfig && !simulatedObjectNodeConfig->m_shapes.empty());
            }
        }
        default:
            break;
        }

        return QVariant();
    }

    Qt::ItemFlags SkeletonModel::flags(const QModelIndex& index) const
    {
        Qt::ItemFlags result = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

        if (!m_skeleton || !index.isValid())
        {
            AZ_Assert(false, "Cannot get model data. Skeleton or model index invalid.");
            return Qt::NoItemFlags;
        }

        Node* node = static_cast<Node*>(index.internalPointer());
        AZ_Assert(node, "Expected valid node pointer.");

        const size_t nodeIndex = node->GetNodeIndex();
        const NodeInfo& nodeInfo = m_nodeInfos[nodeIndex];

        if (nodeInfo.m_checkable)
        {
            result |= Qt::ItemIsUserCheckable;
        }

        return result;
    }

    bool SkeletonModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (!m_skeleton || !index.isValid())
        {
            AZ_Assert(false, "Cannot get model data. Skeleton or model index invalid.");
            return false;
        }

        const Node* node = static_cast<Node*>(index.internalPointer());
        AZ_Assert(node, "Expected valid node pointer.");

        const size_t nodeIndex = node->GetNodeIndex();
        NodeInfo& nodeInfo = m_nodeInfos[nodeIndex];

        switch (role)
        {
        case Qt::CheckStateRole:
        {
            if (index.column() == 0 && nodeInfo.m_checkable)
            {
                nodeInfo.m_checkState = (Qt::CheckState)value.toInt();
                return true;
            }
            break;
        }
        }

        return true;
    }

    QModelIndex SkeletonModel::GetModelIndex(Node* node) const
    {
        if (!node)
        {
            return QModelIndex();
        }

        Node* parentNode = node->GetParentNode();
        if (parentNode)
        {
            const int numChildNodes = aznumeric_caster(parentNode->GetNumChildNodes());
            for (int i = 0; i < numChildNodes; ++i)
            {
                const Node* childNode = m_skeleton->GetNode(parentNode->GetChildIndex(i));
                if (childNode == node)
                {
                    return createIndex(i, 0, node);
                }
            }
        }

        const int numRootNodes = aznumeric_caster(m_skeleton->GetNumRootNodes());
        for (int i = 0; i < numRootNodes; ++i)
        {
            const Node* rootNode = m_skeleton->GetNode(m_skeleton->GetRootNodeIndex(i));
            if (rootNode == node)
            {
                return createIndex(i, 0, node);
            }
        }

        return QModelIndex();
    }

    QModelIndexList SkeletonModel::GetModelIndicesForFullSkeleton() const
    {
        QModelIndexList result;
        const size_t jointCount = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < jointCount; ++i)
        {
            Node* joint = m_skeleton->GetNode(i);
            result.push_back(GetModelIndex(joint));
        }
        return result;
    }

    void SkeletonModel::Reset()
    {
        beginResetModel();
        endResetModel();
    }

    void SkeletonModel::SetCheckable(bool isCheckable)
    {
        if (rowCount() > 0)
        {
            for (NodeInfo& nodeInfo : m_nodeInfos)
            {
                nodeInfo.m_checkable = isCheckable;
            }

            QVector<int> roles;
            roles.push_back(Qt::CheckStateRole);
            dataChanged(index(0, 0), index(rowCount() - 1, 0), roles);
        }
    }

    void SkeletonModel::ForEach(const AZStd::function<void(const QModelIndex& modelIndex)>& func)
    {
        QModelIndex modelIndex;
        const size_t jointCount = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < jointCount; ++i)
        {
            Node* joint = m_skeleton->GetNode(i);
            modelIndex = GetModelIndex(joint);
            if (modelIndex.isValid())
            {
                func(modelIndex);
            }
        }
    }

    void SkeletonModel::ActorSelectionChanged(Actor* actor)
    {
        SetActor(actor);
    }

    void SkeletonModel::ActorInstanceSelectionChanged(EMotionFX::ActorInstance* actorInstance)
    {
        SetActorInstance(actorInstance);
    }

    void SkeletonModel::UpdateNodeInfos(Actor* actor)
    {
        if (!actor)
        {
            m_nodeInfos.clear();
            return;
        }

        const size_t numLodLevels = actor->GetNumLODLevels();
        const Skeleton* skeleton = actor->GetSkeleton();
        const size_t numNodes = skeleton->GetNumNodes();
        m_nodeInfos.resize(numNodes);

        AZStd::vector<AZStd::vector<size_t> > boneListPerLodLevel;
        boneListPerLodLevel.resize(numLodLevels);
        for (size_t lodLevel = 0; lodLevel < numLodLevels; ++lodLevel)
        {
            actor->ExtractBoneList(lodLevel, &boneListPerLodLevel[lodLevel]);
        }

        for (size_t nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
        {
            NodeInfo& nodeInfo = m_nodeInfos[nodeIndex];

            // Is bone?
            nodeInfo.m_isBone = AZStd::any_of(begin(boneListPerLodLevel), end(boneListPerLodLevel), [nodeIndex](const AZStd::vector<size_t>& lodLevel)
            {
                return AZStd::find(begin(lodLevel), end(lodLevel), nodeIndex) != end(lodLevel);
            });

            // Has mesh?
            nodeInfo.m_hasMesh = false;
            for (size_t lodLevel = 0; lodLevel < numLodLevels; ++lodLevel)
            {
                if (actor->GetMesh(lodLevel, nodeIndex))
                {
                    nodeInfo.m_hasMesh = true;
                    break;
                }
            }
        }
    }
} // namespace EMotionFX
