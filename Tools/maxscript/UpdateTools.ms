-------------------------------------------------------------------------------
-- UpdateTools.ms
-- Version 2.0
-- Updates local CryTools files
-------------------------------------------------------------------------------

version_ = "CryToolsUpdate 2.2"

-------------------------------------------------------------------------------
--	Get The Build Dirs
-------------------------------------------------------------------------------

--	Write Latest Builds on S:\_Builds to Local File
print "Retrieving list of latest builds from \\\\Storage\\builds"

DOScommand ("DIR \\\\storage\\builds\\procedurally_generated_builds\\ /B /O-D > \"" + sysInfo.tempDir + "cry_temp\\latest_build_crytoolss.txt\"")

if doesfileexist "\\\\storage\\builds\\procedurally_generated_builds\\" == false then
(
	messageBox "Cannot locate \\\\Storage\\builds\\procedurally_generated_builds\nYou may need to contact SYSTEM_SUPPORT." title: "S Drive not found!"
	return undefined
)

--	Gets the latest build number and build name
if crytools.existFile (sysInfo.tempDir + "cry_temp\\latest_build_crytoolss.txt") != false then
	(
	if doesfileexist "\\\\storage\\builds\\procedurally_generated_builds" != true then
		(
		messagebox "Cannot find \\\\storage\\builds\\procedurally_generated_builds\\"
		return undefined
		)
	latest_build_crytools_list = openFile (sysInfo.tempDir + "cry_temp\\latest_build_crytoolss.txt")
	crytools.latest_build = (readline latest_build_crytools_list)
	if crytools.latest_build == "TempBuildCopy" then
		(
		skipToNextLine latest_build_crytools_list
		crytools.latest_build = (readline latest_build_crytools_list)
		)
	buildnumberArray = filterstring crytools.latest_build "()"
	crytools.latestbuildnumber = buildnumberArray[2]
	close latest_build_crytools_list
	)

--	Get the local build number
print crytools.BuildPathFull
if crytools.existfile ((crytools.BuildPathFull + "Code_Changes.txt")) == false then
	(
			messageBox "Code_Changes.txt cannot be found in your build directory, have you removed it?" title: "Error!"
	)
else
	(
	perf_path = (crytools.BuildPathFull + "Code_Changes.txt")
	perf_changes = openFile perf_path
	skipToString perf_changes "in Build "
	local_build_line =  (readLine perf_changes)
	local_buildArray = (filterString local_build_line "-")
	crytools.localBuildNumber = local_buildArray[1]
	)

--	Get The Project Name
buildpatharray2 = filterstring crytools.BuildPathFull "\\"
crytools.project_name = buildpatharray2[2]
print (crytools.project_name + " is set as current project.")

--	Check For Rollback
rollback_check = openFile (sysInfo.tempDir + "cry_temp\\crytools.rollback_status.ini")
if crytools.rollback_status == undefined then 
(
crytools.rollback_status = "false"
output_rollbackINI = createfile (sysInfo.tempDir + "\\cry_temp\\crytools.rollback_status.ini")
format crytools.rollback_status to: output_rollbackINI
close output_rollbackINI
)
if rollback_check != undefined then
(
crytools.rollback_status = (readline rollback_check)
)

-------------------------------------------------------------------------------
--	UpdateUI
-------------------------------------------------------------------------------

print (sysInfo.username + " is requesting an update.")
rollout checkForUpdate version_
(
label tools_version "" align:#center
button update_btn " Check/Install Updates From Your Latest Build"
button update_btnAB "Retrieve Latest Tools\Sync"
checkbox BuildOn "Current Build" offset:[0,-4]
checkBox PerfOn "PerForce" offset:[84,-20] checked:true
checkbox HTTPOn "CryHTTP" offset:[151,-20]
checkbutton rollback_exporter "Rollback Exporter" offset:[-55,0]
button uninstall_tools "Uninstall CryTools" offset:[55,-26]
label current_exportTXT "LOCAL BUILD: Cannot find Code_Changes.txt" align:#center

on checkForUpdate open do
(
current_exportTXT.text = ("LOCAL BUILD: " + crytools.localBuildNumber + "    LATEST BUILD: " + crytools.latestbuildnumber)
tools_version.text = version_
if crytools.rollback_status == "true" do (rollback_exporter.checked = true)
if crytools.rollback_status == "false" do (rollback_exporter.checked = false)
)

on update_btn pressed do
(
filein (crytools.BuildPathFull + "Tools\\maxscript\\AddCryTools.ms")
current_exportTXT.text = ("LOCAL BUILD: " + crytools.localBuildNumber + "    LATEST BUILD: " + crytools.latestbuildnumber)
print ("Build updated from " + crytools.BuildPathFull)
--destroyDialog checkForUpdate
)

--	Get Latest From AB and Latest Build
-------------------------------------------------------------------------------
on update_btnAB pressed do
(
	try
	(
		if crytools.BuildPathFull == "J:\\Game04\\" then
		(
			messagebox "You are on Game04"
			return undefined
		)
		--	AB Stuff
		if HTTPOn.checked == true then
		(
			rollout httpSock "httpSock" width:0 height:0
		        (
		            activeXControl port "Microsoft.XMLHTTP" setupEvents:false releaseOnClose:false
		        );
		    createDialog httpSock pos:[-100,-100];
		    destroyDialog httpSock;
		
		    httpSock.port.open "GET" "http://www.crytek.com/index.htm" false;
		    httpSock.port.setrequestheader "If-Modified-Since" "Sat, 1 Jan 1900 00:00:00 GMT";
		    httpSock.port.send();
			
		    print (httpSock.port.responsetext);
		)
		
		--	P4 stuff
		if perfOn.checked == true then
		(
			p4Update = ("p4 sync " + crytools.BuildPathFull + "Tools\...")
			DOScommand p4Update
		)
		
		if BuildOn.checked == true then
		(
		--	Latest Build Stuff
		rollback_check = openFile (sysInfo.tempDir + "cry_temp\\crytools.rollback_status.ini")
		if rollback_check == undefined then (crytools.rollback_status = "false")
		crytools.rollback_status = "false"
		latestCryExport = (crytools.md5 ("\\\\Storage\\builds\\" + crytools.latest_build + "\\Tools\\CryExport8.dlu"))
			if crytools.md5 (crytools.maxDirTxt + "plugins\\CryExport8.dlu") != latestCryExport then
			(
				if crytools.existfile ("\\\\storage\\builds\\" + crytools.latest_build + "\\Tools\\CryExport8.dlu") == false then
				(
				messageBox ("There is no exporter on the build server in the latest folder [" + crytools.latest_build + "]") title: "No Exporter Found!"
				)
			else
			(
				messageBox ("There is a new exporter available in build " + crytools.latestbuildnumber) title: "New Exporter Found!"
				DOScommand (("copy /Y \\\\storage\\builds\\" + crytools.latest_build + "\\Tools\\CryExport8.dlu ") + (crytools.BuildPathFull + "Tools\\"))
			)
		)
	)
)	
	catch
	(
	messageBox "Either cannot locate the build server [\\\\Storage\\], or you do not have crytools.alienBrain correctly installed." title: "Something is wrong!"
	)
		
messageBox ("CryTools has checked Build [" + crytools.latestbuildnumber + "] for updates.\nPlease click the \"Check/Install Updates From Your Latest Build\" button to install any updates it found.") title: ("Checked Build \\Tools (" + localTime + ") - Checked Plugins From Build #" + crytools.latestbuildnumber)
)

--	Rollback Exporter
-------------------------------------------------------------------------------
on rollback_exporter changed state do
	(
	try
		if (rollback_exporter.checked == true) then 
		(
		crytools.rollback_status = "true"
		DOScommand ("mkdir \"" + sysInfo.tempDir + "cry_temp\\bad\\\"")
		DOScommand ("move /Y " + ("\"" + crytools.maxDirTxt + "plugins\\CryExport8.dlu\"") + " " + (sysInfo.tempDir + "cry_temp\\bad\\"))
		DOScommand ("move /Y " + ("\"" +sysInfo.tempDir + "cry_temp\\CryExport8.dlu\"") + " " + (crytools.maxDirTxt + "plugins\\"))
		print "CryExport8.dlu has been rolled back to the previous version."
		output_rollbackINI = openfile (sysInfo.tempDir + "\\cry_temp\\crytools.rollback_status.ini") mode:"w"
		format crytools.rollback_status to: output_rollbackINI
		close output_rollbackINI

		messageBox "CryExport8.dlu has been rolled back to the previous version.\nTo get a newer exporter later you must click \"Get Latest Tools From crytools.alienBrain/Current Build\", or update your build." title: "CryExport8.dlu Rolled Back!"
		)
		else
		(
		crytools.rollback_status = "false"
		output_rollbackINI = openfile (sysInfo.tempDir+ "\\cry_temp\\crytools.rollback_status.ini") mode:"w"
		format crytools.rollback_status to: output_rollbackINI
		close output_rollbackINI
		messageBox "You are no longer in rollback mode.\nTo get a newer exporter later you must click \"Get Latest Tools From crytools.alienBrain/Current Build\", or update your build." title: "CryExport8.dlu No Longer Rolled Back!"
		)
	catch
	( 
		messageBox "Rollback error 1442." title:"Error!"
		return undefined
	)
	)


--	Uninstall
-------------------------------------------------------------------------------

on uninstall_tools pressed do
(
rollout areYouSure "CryTools Uninstallation"
(
label doyouwant "Are you sure you want to completely remove CryTools?" align:#center
button uninstallNow "Yes" pos:[110,25]
button donotuninstall "No" pos:[150,25]
on donotuninstall pressed do
(
destroyDialog areYouSure
)
on uninstallNow pressed do
(
subMenu = menuMan.findMenu "CryTools"
menuMan.unRegisterMenu subMenu
deleteFile "$UI\\MacroScripts\\CryTools-UpdateTools.mcr"
deleteFile "$UI\\MacroScripts\\CryTools-CryRigging.mcr"
deleteFile "$UI\\MacroScripts\\CryTools-CryMorphManager.mcr"
deleteFile "$UI\\MacroScripts\\CryTools-CryAnimation.mcr"
crytools.maxDirTxt = (getdir #maxroot)
doscommand ("attrib -r \"" + crytools.maxDirTxt + "scripts\\startup\\LoadCryTools.ms\"")
doscommand ("del \"" + crytools.maxDirTxt + "scripts\\startup\\LoadCryTools.ms\"")
print (sysInfo.username + " has uninstalled CryTools.")
destroyDialog areYouSure
destroyDialog checkForUpdate
messageBox ("CryTools has been uninstalled. CryExport8.dlu is still installed, " + "sorry " + sysInfo.username) title: "Uninstallation complete!"
)
)
createDialog areYouSure 300 60 bgcolor:black fgcolor:white
)
)
createDialog checkForUpdate 250 137  bgcolor:black fgcolor:white