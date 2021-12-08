"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# for underlying data structures, see Code\Framework\AzFramework\AzFramework\Physics\Shape.h

class ColliderConfiguration():
    """
    Configuration for a collider

    Attributes
    ----------
    Trigger: bool
        Should this shape act as a trigger shape.

    Simulated: bool
        Should this shape partake in collision in the physical simulation.

    InSceneQueries: bool
        Should this shape partake in scene queries (ray casts, overlap tests, sweeps).

    Exclusive: bool
        Can this collider be shared between multiple bodies?

    Position: [float, float, float] Vector3
        Shape offset relative to the connected rigid body.

    Rotation: [float, float, float, float] Quaternion
        Shape rotation relative to the connected rigid body.

    ColliderTag: str
        Identification tag for the collider.

    RestOffset: float
        Bodies will come to rest separated by the sum of their rest offsets.

    ContactOffset: float
        Bodies will start to generate contacts when closer than the sum of their contact offsets.
    Methods
    -------
    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        self.Trigger = True
        self.Simulated = True
        self.InSceneQueries = True
        self.Exclusive = True
        self.Position = [0.0, 0.0, 0.0]
        self.Rotation = [0.0, 0.0, 0.0, 1.0]
        self.ColliderTag = ''
        self.RestOffset = 0.0
        self.ContactOffset = 0.02

    def to_dict(self):
        return self.__dict__

# for underlying data structures, see Code\Framework\AzFramework\AzFramework\Physics\Character.h

class CharacterColliderNodeConfiguration():
    """
    Shapes to define the animation's to model of physics

    Attributes
    ----------
    name : str
        debug name of the node

    shapes : `list` of `tuple` of (ColliderConfiguration, ShapeConfiguration) 
        a list of pairs of collider and shape configuration

    Methods
    -------
    add_collider_shape_pair(colliderConfiguration, shapeConfiguration)
        Helper function to add a collider and shape configuration at the same time

    to_dict()
        Converts contents to a Python dictionary

    """
    def __init__(self):
        self.name = ''
        self.shapes = [] # List of Tuple of (ColliderConfiguration, ShapeConfiguration) 
        
    def add_collider_shape_pair(self, colliderConfiguration, shapeConfiguration) -> None:
        pair = (colliderConfiguration, shapeConfiguration)
        self.shapes.append(pair)
        
    def to_dict(self):
        data = {}
        shapeList = []
        for index, shape in enumerate(self.shapes):
            tupleValue = (shape[0].to_dict(),  # ColliderConfiguration
                          shape[1].to_dict())  # ShapeConfiguration
            shapeList.append(tupleValue)
        data['name'] = self.name
        data['shapes'] = shapeList
        return data

class CharacterColliderConfiguration():
    """
    Information required to create the basic physics representation of a character.

    Attributes
    ----------
    nodes : `list` of CharacterColliderNodeConfiguration
        a list of CharacterColliderNodeConfiguration nodes

    Methods
    -------
    add_character_collider_node_configuration(colliderConfiguration, shapeConfiguration)
        Helper function to add a character collider node configuration into the nodes

    add_character_collider_node_configuration_node(name, colliderConfiguration, shapeConfiguration)
        Helper function to add a character collider node configuration into the nodes 

        **Returns**: CharacterColliderNodeConfiguration

    to_dict()
        Converts contents to a Python dictionary

    """
    def __init__(self):
        self.nodes = [] # list of CharacterColliderNodeConfiguration
        
    def add_character_collider_node_configuration(self, characterColliderNodeConfiguration) -> None:
        self.nodes.append(characterColliderNodeConfiguration)
        
    def add_character_collider_node_configuration_node(self, name, colliderConfiguration, shapeConfiguration) -> CharacterColliderNodeConfiguration:
        characterColliderNodeConfiguration = CharacterColliderNodeConfiguration()
        self.add_character_collider_node_configuration(characterColliderNodeConfiguration)
        characterColliderNodeConfiguration.name = name
        characterColliderNodeConfiguration.add_collider_shape_pair(colliderConfiguration, shapeConfiguration)
        return characterColliderNodeConfiguration

    def to_dict(self):
        data = {}
        nodeList = []
        for node in self.nodes:
            nodeList.append(node.to_dict())
        data['nodes'] = nodeList
        return data

# see Code\Framework\AzFramework\AzFramework\Physics\ShapeConfiguration.h for underlying data structures

class ShapeConfiguration():
    """
    Base class for all the shape collider configurations

    Attributes
    ----------
    scale : [float, float, float]
        a 3-element list to describe the scale along the X, Y, and Z axises such as [1.0, 1.0, 1.0]

    Methods
    -------
    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self, shapeType):
        self._shapeType = shapeType
        self.scale = [1.0, 1.0, 1.0]

    def to_dict(self):
        return {
            "$type": self._shapeType,
            "Scale": self.scale
        }
    
class SphereShapeConfiguration(ShapeConfiguration):
    """
    The configuration for a Sphere collider

    Attributes
    ----------
    radius: float
        a scalar value to define the radius of the sphere

    Methods
    -------
    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__('SphereShapeConfiguration')
        self.radius = 0.5

    def to_dict(self):
        data = super().to_dict()
        data['Radius'] = self.radius
        return data    

class BoxShapeConfiguration(ShapeConfiguration):
    """
    The configuration for a Box collider

    Attributes
    ----------
    dimensions: [float, float, float]
        The width, height, and depth dimensions of the Box collider

    Methods
    -------
    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__('BoxShapeConfiguration')
        self.dimensions = [1.0, 1.0, 1.0]

    def to_dict(self):
        data = super().to_dict()
        data['Configuration'] = self.dimensions
        return data
    
class CapsuleShapeConfiguration(ShapeConfiguration):
    """
    The configuration for a Capsule collider

    Attributes
    ----------
    height: float
        The height of the Capsule

    radius: float
        The radius of the Capsule

    Methods
    -------
    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__('CapsuleShapeConfiguration')
        self.height = 1.00
        self.radius = 0.25
    
    def to_dict(self):
        data = super().to_dict()
        data['Height'] = self.height
        data['Radius'] = self.radius
        return data    
    
class PhysicsAssetShapeConfiguration(ShapeConfiguration):
    """
    The configuration for a Asset collider using a mesh asset for collision

    Attributes
    ----------
    asset: { "assetHint": assetReference }
        the name of the asset to load for collision information

    assetScale: [float, float, float]
        The scale of the asset shape such as [1.0, 1.0, 1.0]

    useMaterialsFromAsset: bool
        Auto-set physics materials using asset's physics material names

    subdivisionLevel: int
        The level of subdivision if a primitive shape is replaced with a convex mesh due to scaling.

    Methods
    -------
    set_asset_reference(self, assetReference: str)
        Helper function to set the asset reference to the collision mesh such as 'my/folder/my_mesh.azmodel'

    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__('PhysicsAssetShapeConfiguration')
        self.asset = {}
        self.assetScale = [1.0, 1.0, 1.0]
        self.useMaterialsFromAsset = True
        self.subdivisionLevel = 4
        
    def set_asset_reference(self, assetReference: str) -> None:
        self.asset = { "assetHint": assetReference }

    def to_dict(self):
        data = super().to_dict()
        data['PhysicsAsset'] = self.asset
        data['AssetScale'] = self.assetScale
        data['UseMaterialsFromAsset'] = self.useMaterialsFromAsset
        data['SubdivisionLevel'] = self.subdivisionLevel
        return data    

# for underlying data structures, see Code\Framework\AzFramework\AzFramework\Physics\Configuration\JointConfiguration.h

class JointConfiguration():
    """
    The joint configuration

    see also: class AzPhysics::JointConfiguration

    Attributes
    ----------
    Name: str
        For debugging/tracking purposes only.

    ParentLocalRotation: [float, float, float, float]
        Parent joint frame relative to parent body.

    ParentLocalPosition: [float, float, float]
        Joint position relative to parent body.

    ChildLocalRotation: [float, float, float, float]
        Child joint frame relative to child body.

    ChildLocalPosition: [float, float, float]
        Joint position relative to child body.

    StartSimulationEnabled: bool 
        When active, the joint will be enabled when the simulation begins.

    Methods
    -------
    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        self.Name = ''
        self.ParentLocalRotation = [0.0, 0.0, 0.0, 1.0]
        self.ParentLocalPosition = [0.0, 0.0, 0.0]
        self.ChildLocalRotation = [0.0, 0.0, 0.0, 1.0]
        self.ChildLocalPosition = [0.0, 0.0, 0.0]
        self.StartSimulationEnabled = True

    def to_dict(self):
        return self.__dict__

# for underlying data structures, see Code\Framework\AzFramework\AzFramework\Physics\Configuration\SimulatedBodyConfiguration.h

class SimulatedBodyConfiguration():
    """
    Base Class of all Physics Bodies that will be simulated.

    see also: class AzPhysics::SimulatedBodyConfiguration

    Attributes
    ----------
    name: str
        For debugging/tracking purposes only.

    position: [float, float, float]
        starting position offset

    orientation: [float, float, float, float]
        starting rotation (Quaternion)

    startSimulationEnabled: bool
        to start when simulation engine starts

    Methods
    -------
    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        self.name = ''
        self.position = [0.0, 0.0, 0.0]
        self.orientation = [0.0, 0.0, 0.0, 1.0]
        self.startSimulationEnabled = True

    def to_dict(self):
        return {
            "name" : self.name,
            "position" : self.position,
            "orientation" : self.orientation,
            "startSimulationEnabled" : self.startSimulationEnabled
        }
    

# for underlying data structures, see Code\Framework\AzFramework\AzFramework\Physics\Configuration\RigidBodyConfiguration.h

class RigidBodyConfiguration(SimulatedBodyConfiguration):
    """
    PhysX Rigid Body Configuration

    see also: class AzPhysics::RigidBodyConfiguration

    Attributes
    ----------
    initialLinearVelocity: [float, float, float]
        Linear velocity applied when the rigid body is activated.

    initialAngularVelocity: [float, float, float]
        Angular velocity applied when the rigid body is activated (limited by maximum angular velocity)

    centerOfMassOffset: [float, float, float]
        Local space offset for the center of mass (COM).

    mass: float
        The mass of the rigid body in kilograms. 
        A value of 0 is treated as infinite.
        The trajectory of infinite mass bodies cannot be affected by any collisions or forces other than gravity.

    linearDamping: float
        The rate of decay over time for linear velocity even if no forces are acting on the rigid body.

    angularDamping: float
        The rate of decay over time for angular velocity even if no forces are acting on the rigid body.

    sleepMinEnergy: float
        The rigid body can go to sleep (settle) when kinetic energy per unit mass is persistently below this value.

    maxAngularVelocity: float
        Clamp angular velocities to this maximum value.

    startAsleep: bool
        When active, the rigid body will be asleep when spawned, and wake when the body is disturbed.

    interpolateMotion: bool
        When active, simulation results are interpolated resulting in smoother motion.

    gravityEnabled: bool
        When active, global gravity affects this rigid body.

    kinematic: bool
        When active, the rigid body is not affected by gravity or other forces and is moved by script.

    ccdEnabled: bool
        When active, the rigid body has continuous collision detection (CCD). 
        Use this to ensure accurate collision detection, particularly for fast moving rigid bodies. 
        CCD must be activated in the global PhysX preferences.

    ccdMinAdvanceCoefficient: float
        Coefficient affecting how granularly time is subdivided in CCD.

    ccdFrictionEnabled: bool
        Whether friction is applied when resolving CCD collisions.

    computeCenterOfMass: bool
        Compute the center of mass (COM) for this rigid body.

    computeInertiaTensor: bool
        When active, inertia is computed based on the mass and shape of the rigid body.

    computeMass: bool
        When active, the mass of the rigid body is computed based on the volume and density values of its colliders.

    Methods
    -------
    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__()

        # Basic initial settings.
        self.initialLinearVelocity = [0.0, 0.0, 0.0]
        self.initialAngularVelocity = [0.0, 0.0, 0.0]
        self.centerOfMassOffset = [0.0, 0.0, 0.0]

        # Simulation parameters.
        self.mass = 1.0
        self.linearDamping = 0.05
        self.angularDamping = 0.15
        self.sleepMinEnergy = 0.005
        self.maxAngularVelocity = 100.0
        self.startAsleep = False
        self.interpolateMotion = False
        self.gravityEnabled = True
        self.kinematic = False
        self.ccdEnabled = False
        self.ccdMinAdvanceCoefficient = 0.15
        self.ccdFrictionEnabled = False
        self.computeCenterOfMass = True
        self.computeInertiaTensor = True
        self.computeMass = True

        # Flags to restrict motion along specific world-space axes.
        self.lockLinearX = False
        self.lockLinearY = False
        self.lockLinearZ = False

        # Flags to restrict rotation around specific world-space axes.
        self.lockAngularX = False
        self.lockAngularY = False
        self.lockAngularZ = False

        # If set, non-simulated shapes will also be included in the mass properties calculation.
        self.includeAllShapesInMassCalculation = False

    def to_dict(self):
        data = super().to_dict()
        data["Initial linear velocity"] = self.initialLinearVelocity
        data["Initial angular velocity"] = self.initialAngularVelocity
        data["Linear damping"] = self.linearDamping
        data["Angular damping"] = self.angularDamping
        data["Sleep threshold"] = self.sleepMinEnergy
        data["Start Asleep"] = self.startAsleep
        data["Interpolate Motion"] = self.interpolateMotion
        data["Gravity Enabled"] = self.gravityEnabled
        data["Kinematic"] = self.kinematic
        data["CCD Enabled"] = self.ccdEnabled
        data["Compute Mass"] = self.computeMass
        data["Lock Linear X"] = self.lockLinearX
        data["Lock Linear Y"] = self.lockLinearY
        data["Lock Linear Z"] = self.lockLinearZ
        data["Lock Angular X"] = self.lockAngularX
        data["Lock Angular Y"] = self.lockAngularY
        data["Lock Angular Z"] = self.lockAngularZ
        data["Mass"] = self.mass
        data["Compute COM"] = self.computeCenterOfMass
        data["Centre of mass offset"] = self.centerOfMassOffset
        data["Compute inertia"] = self.computeInertiaTensor
        data["Maximum Angular Velocity"] = self.maxAngularVelocity
        data["Include All Shapes In Mass"] = self.includeAllShapesInMassCalculation
        data["CCD Min Advance"] = self.ccdMinAdvanceCoefficient
        data["CCD Friction"] = self.ccdFrictionEnabled        
        return data

# for underlying data structures, see Code\Framework\AzFramework\AzFramework\Physics\Ragdoll.h

class RagdollNodeConfiguration(RigidBodyConfiguration):
    """
    Ragdoll node Configuration

    see also: class Physics::RagdollConfiguration

    Attributes
    ----------
    JointConfig: JointConfiguration
        Ragdoll joint node configuration

    Methods
    -------
    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__()
        self.JointConfig = JointConfiguration()

    def to_dict(self):
        data = super().to_dict()
        data['JointConfig'] = self.JointConfig.to_dict()
        return data

class RagdollConfiguration():
    """
    A configuration of join nodes and a character collider configuration for a ragdoll

    see also: class Physics::RagdollConfiguration

    Attributes
    ----------
    nodes: `list` of  RagdollNodeConfiguration
        A list of RagdollNodeConfiguration entries

    colliders: CharacterColliderConfiguration
        A CharacterColliderConfiguration

    Methods
    -------
    add_ragdoll_node_configuration(ragdollNodeConfiguration)
        Helper function to add a single ragdoll node configuration (normally for each joint/bone node)

    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        self.nodes = [] # list of RagdollNodeConfiguration
        self.colliders = CharacterColliderConfiguration()
        
    def add_ragdoll_node_configuration(self, ragdollNodeConfiguration) -> None:
        self.nodes.append(ragdollNodeConfiguration)

    def to_dict(self):
        data = {}
        nodeList = []
        for index, node in enumerate(self.nodes):
            nodeList.append(node.to_dict())
        data['nodes'] = nodeList
        data['colliders'] = self.colliders.to_dict()
        return data
