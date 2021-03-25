-------------------------------------------------------------------------------
-- LoadCryTools.ms
-- Version 2.2 External
-- By: Christopher Evans
-------------------------------------------------------------------------------


-------------------------------------------------------------------------------
--	Get The Build Dirs
-------------------------------------------------------------------------------

--	Local Build Dir

if csexport != undefined then
(
	global maxDirTxt_crytools = (getdir #maxroot)
	global cryINI_crytools = (getdir #maxroot + "plugins\\CryExport.ini")

	errorFound = false

	try ( global buildPathFull_crytools = csexport.get_root_path() + "\\" )
	catch
	(
		global buildPathFull_crytools = getINISetting cryINI_crytools "SandBox" "buildPath"
		
		if buildPathFull_crytools == "" then
		(
			messageBox "Incompatible version of CryTools and CryExport" title:"Error loading CryTools"
			errorFound = true
		)
		else
			print "Loading CryTools from INI file"
	)

	if (doesfileexist(buildPathFull_crytools + "Bin64vc141\\Editor.exe") == true then
	    global editorPath_crytools = buildPathFull_crytools + "Bin64vc141\\Editor.exe"
	else if (doesfileexist(buildPathFull_crytools + "Bin64vc140\\Editor.exe") == true then
	    global editorPath_crytools = buildPathFull_crytools + "Bin64vc140\\Editor.exe"
	else
		messagebox("I cannot find Editor.exe")


--	Load CryTools
-------------------------------------------------------------------------------
	if errorFound == false then
	(
		if buildPathFull_crytools != "" then
		(
			if (doesfileexist (BuildPathFull_crytools + "Tools\\maxscript\\AddCryTools.ms")) == true then
				FileIn (BuildPathFull_crytools + "Tools\\maxscript\\AddCryTools.ms")
			else
				messagebox ("I cannot find" + (BuildPathFull_crytools + "Tools\\maxscript\\AddCryTools.ms"))
		)
		else
			messageBox "Can't find local Build from cryExport.ini" title:"Error loading CryTools"
	)
)
else
	messageBox "Error initialising CryTools:  CryExport plugin not found"


