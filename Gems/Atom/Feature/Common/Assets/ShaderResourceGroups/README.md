There must be only one scenesrg.srgi and one viewsrg.srgi files
per game project. They are always located in \<DevFolder>/\<Project>/ShaderLib,
This location can not change, otherwise the asset processor won't be able to find
the SceneSrg and the ViewSrg.

Each game project can customize the content of scenesrg.srgi and viewsrg.srgi as needed.

The preprocessor that works on behalf of the Shader Compiler defines the following priority for paths used to search for included files (ordered here from highest to lowest priority):

1. Paths defined in the file \<DevFolder>/\<Project>/Config/shader_global_build_options.json  
    1.1. Each path inside *shader_global_build_options.json* is relative to the \<DevFolder>/ path.  
  
2. All valid scan folders returned by the Asset Processor with added "ShaderLib/" folder at the end. Here is a short summary of the folders that are scanned by the AP:  
2.1. Folders specified in \<DevFolder>/AssetProcessorPlatformConfig.ini  
2.2. The Asset/ subdirectory within each Gem enabled for the Game Project. (Recursive). Examples:  
2.2.1. \<DevFolder>/Gems/Atom/Asset/  
2.2.2. \<DevFolder>/Gems/Camera/Asset/  
2.3. The root folder of each Gem enabled for the Game Project. (Non Recursive). Examples:  
2.2.1. \<DevFolder>/Gems/Atom/  
2.2.2. \<DevFolder>/Gems/Camera/

3. Finally, at the lowest priority is the folder: \<DevFolder>/Gems/  
  
With that being said, if a project doesn't provide its own *shader_global_build_options.json* then each time an azsl/azsli calls  
#include \<scenesrg.srgi>  
it will default to  
\<DevFolder>/\<Project>/ShaderLib/scenesrg.srgi  
 Similarly occurs with #include \<viewsrg.srgi>  
  
Because documents may become stale from time to time, the ultimate source of truth on how include paths are resolved is the source file  
 \<DevFolder>/Gems/Atom/Asset/Shader/Code/Source/Editor/CommonFiles/Preprocessor.cpp  
 Function name: void InitializePreprocessorOptions(...)  
 And yes, even the location of that file may change in the future ;-).








