"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# for underlying data structures, see Code\Framework\AzFramework\AzFramework\Physics\Shape.h

class ColliderConfiguration():
    """
    Configuration for a collider
    
    Attributes:
        Trigger: Should this shape act as a trigger shape.
        Simulated: Should this shape partake in collision in the physical simulation.
        InSceneQueries: Should this shape partake in scene queries (ray casts, overlap tests, sweeps).
        Exclusive: Can this collider be shared between multiple bodies?
        Position: Shape offset relative to the connected rigid body.
        Rotation: Shape rotation relative to the connected rigid body.
        ColliderTag: Identification tag for the collider.
        RestOffset: Bodies will come to rest separated by the sum of their rest offsets.
        ContactOffset: Bodies will start to generate contacts when closer than the sum of their contact offsets.   
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
    
    Attributes:
        name: debug name of the node
        shapes: a list of pairs of collider and shape configuration
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
                          shape[1].to_dict()); # ShapeConfiguration
            shapeList.append(tupleValue)
        data['name'] = self.name
        data['shapes'] = shapeList
        return data

class CharacterColliderConfiguration():
    """
    Information required to create the basic physics representation of a character.
    
    Attributes:
        nodes: a list of CharacterColliderNodeConfiguration nodes
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
    
    Attributes:
        scale: a 3-element list to describe the scale along the X, Y, and Z axises such as [1.0, 1.0, 1.0]
    """
    def __init__(self, shapeType):
        self._shapeType = shapeType
        self.scale = [1.0, 1.0, 1.0]

    def to_dict(self):
        return {
            "$type" : self._shapeType,
            "Scale" : self.scale
        }
    
class SphereShapeConfiguration(ShapeConfiguration):
    """
    The configuration for a Sphere collider
    
    Attributes:
        radius: a scalar value to define the radius of the sphere
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
    
    Attributes:
        dimensions: The width, height, and depth dimensions of the Box collider
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
    
    Attributes:
        height: The height of the Capsule
        radius: The radius of the Capsule
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
    
    Attributes:
        asset: the name of the asset to load for collision information
        assetScale: The scale of the asset shape such as [1.0, 1.0, 1.0]
        useMaterialsFromAsset: Auto-set physics materials using asset's physics material names
        subdivisionLevel: The level of subdivision if a primitive shape is replaced with a convex mesh due to scaling.
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
    
    Attributes:
        Name: For debugging/tracking purposes only.
        ParentLocalRotation: Parent joint frame relative to parent body.
        ParentLocalPosition: Joint position relative to parent body.
        ChildLocalRotation: Child joint frame relative to child body.
        ChildLocalPosition: Joint position relative to child body.
        StartSimulationEnabled: When active, the joint will be enabled when the simulation begins.

    see also: class AzPhysics::JointConfiguration
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
    
    Attributes:
        name: For debugging/tracking purposes only.
        position: starting position offset
        orientation(Quaternion): starting rotation
        startSimulationEnabled: to start when simulation engine starts

    see also: class AzPhysics::SimulatedBodyConfiguration
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
    desc
    
    Attributes:
        JointConfiguration: desc

    see also: class AzPhysics::RigidBodyConfiguration
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
        self.ccdEnabled = False # Whether continuous collision detection is enabled.
        self.ccdMinAdvanceCoefficient = 0.15 # Coefficient affecting how granularly time is subdivided in CCD.
        self.ccdFrictionEnabled = False # Whether friction is applied when resolving CCD collisions.
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
    
    Attributes:
        JointConfiguration: Joint configuration.
    
    see also: class Physics::RagdollConfiguration
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
    Configurationforacollider
    
    Attributes:
        nodes: A list of RagdollNodeConfiguration entries
        colliders: A CharacterColliderConfiguration
    
    see also: class Physics::RagdollConfiguration

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
