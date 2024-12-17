# Customizing Shader Resource Groups

Please read:  
*\<engine\>/Gems/Atom/Feature/Common/Assets/ShaderResourceGroups/README.md*  
for details on how to customize scenesrg.srgi and viewsrg.srgi.

This folder (<engine\>/Gems/Atom/Asset/DefaultProjectShaderLib) will be used if the project doesn't contain 
the corresponding files in \<Project>/ShaderLib. To modify them, copy the contents of this folder to the
<Project>/ShaderLib folder.

Note that files in the engine can receive updates which are not migrated to existing projects. If you copy these files,
you have to keep them up-to-date manually. 

Old project templates contained the ShaderLib - folder by default. To maintain backwards compatability with these projects,
we ignore the <Project>/ShaderLib folder if its contents were never modified, and use the engine folder instead.
