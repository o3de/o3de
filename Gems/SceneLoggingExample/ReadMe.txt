The Scene Logging Example demonstrates how to extend the SceneAPI by adding additional logging to the pipeline. The SceneAPI is 
a collection of libraries that handle loading scene files and converting content to data that the Open 3D Engine and its editor can load. 

The following approach is used:
    1. The SceneBuilder and SceneData load and convert the scene file (for example, .fbx) into a graph that is stored in memory. 
    2. SceneCore and SceneData are used to create a manifest with instructions about how to export the file. 
    3. SceneData analyzes the manifest and memory graph and creates defaults.
    4. Scene Settings allows updates to the manifest through a UI.
    5. The ResourceCompilerScene uses the instructions from the manifest and the data in the graph to create assets. These assets are ready for Open 3D Engine to use.

The example gem demonstrates the following key features:
    - Initialization of the SceneAPI libraries.
        (See SceneLoggingExampleModule.cpp)
    - Adding a LoadingComponent to hook into the scene loading and react to loading events.
        (See Processing/LoadingTrackingProcessor)
    - Extension of the Scene Settings UI and ability to set defaults using the BehaviorComponent.
        (See Groups/LoggingGroup and Behaviors/LoggingGroupBehavior)
    - Adding an ExportingComponent to hook into the scene converting and exporting events.
        (See Behaviors/ExportTrackingProcessor)

