# Prefab Gem

The Prefab gem handles processing the prefab source assets into spawnable product assets, the Editor behavior for procedural prefabs, and handling procedural prefab asset types.

Procedural prefabs are prefabs produced through the scene building pipeline. Procedural prefabs are read-only prefab templates since the source scene assets (e.g. FBX) are authorative not the Editor; that is, procedural prefabs are created during the scene pipeline not edited in the Editor. Procedural prefabs are created by default for source scene assets or when the scene builder updates a scene manifest to add Prefab Group rules. To learn more about procedural prefabs, see link **TBL** to get more details and examples.

## Reference Links

TBW

- LINK_TO_PROC_PREFABS
- LINK_TO_UDP_MATERIAL

## Terminology

- Prefab – A prefabricated description of an entity-component tree in a JSON document
- Spawnable - A prefab asset that has been compiled into a spawnable prefab asset, content that can be loaded - dynamically while a game is running.
Authored Prefab - A prefab that was created by a content creator via the O3DE Editor, and is tracked as a source asset.
- Procedural Prefab - A prefab that is output from this new system, a prefab that only exists in the asset cache and is not hand authored.
- Source Scene Asset – A file that describes the 3D data such as meshes, cameras, lights, and materials organized in a node hierarchy; example file extensions FBX, DAE, STP
- Scene Manifest – a document paired with a Scene File to describe a rule set used to export product assets from a scene
- Digital Content Creation Package (DCC Package) – this is a digital asset creation package such as Blender, Maya, or 3D Studio Max
- Technical pipeline developers - people (such as Technical Artists and Tool Programmers) that are in charge of preparing assets for the O3DE engine to consume; they typically use Python scripts or even C++ code to extend the asset pipeline to transform and/or prepare custom assets