-------------------------------------------------------------------------------
-- Diagnostics.ms
-- Version 2.5
-- General cryTools control panel
-------------------------------------------------------------------------------

if diagnostics != undefined then
(
	destroydialog diagnostics
)

rollout diagnostics "cryTools Control Panel 2.5"
(
	group "Art"
	(
		checkbox warnMatsCheck "Check for Crytek shader at export"
		checkbox reparentTwistCheck "Re-parent biped twist bones at export"
	)
	group "Animation"
	(
		checkbox loadOldAnimTools "Load Old Animation Tools"
		checkbox noUnparentWeapons "Do not unparent $weapon_bone children at export"
		--checkbox updateCollectionsCheck "Auto-update pose collections on max file open" --enabled:false
		--checkbox syncCollectionsAtLoad "Sync pose collections at max start (P4)" --enabled:false
	)
	Group "Misc"
	(
		checkbox checkBeforeExport "Check before Export" checked:true
		checkbox suppressWarningsCheck "Suppress all export warnings"
		checkbox showSplashCheck "Show splash screen"
	)
	Group "Update/Uninstall/Rollback"
	(
		button update_btn "               Reload/Install Updates From Your Local Build                " align:#center enabled:false
		button update_btnAB "Retrieve Latest Tools\Sync" align:#left enabled:false
		checkbox BuildOn "LAN" offset:[150,-22] enabled:false
		checkBox PerfOn "PerForce" offset:[195,-20] checked:true enabled:false
		checkbox HTTPOn "CryHTTP" offset:[262,-20] enabled:false
		checkbutton rollback_exporter "Rollback Exporter" offset:[-112,0] enabled:false
		button uninstall_tools "Uninstall CryTools" offset:[-7,-26] enabled:false
		label current_exportTXT "LOCAL BUILD: Cannot find Code_Changes.txt" align:#center enabled:false
	)
	button dumpCryToolsGlobals "dump crytools global vars" offset:[-100,0]
	button callbackList "dump callbacks" offset:[18,-26]
	button callbackRemove "remove callbacks" offset:[117,-26] tooltip:"right click to activate" enabled:true
	--checkbox weaponChild "Remove $weapon_bone children (anim assets)" enabled:false
	
	--label spacer01 ""
	label maxVersionNum_crytools_LBL "MAX VERSION:" align:#left
	label maxDirTxt_LBL "MAX PATH:" align:#left
	label project_name_crytools_LBL "PROJECT: " align:#left
	edittext projectEnter text:"NONE" offset:[60,-20] fieldWidth:180 --enabled:false
	button pickProject "PICK" offset:[105,-21] height:15 --enabled:false
	button setProject "SET" offset:[148,-21] height:15 --enabled:false
	label domain_LBL "DOMAIN: " align:#left
	label BuildPathFull_LBL "BUILD PATH: " align:#left
	label cryINI_LBL "CRYEXPORT.INI PATH:" align:#left
	label cryToolsINI_LBL "CRYTOOLS.INI PATH:" align:#left
	label editorpath_LBL "EDITOR PATH:" align:#left
	label cbaPath_LBL "CBA PATH: UNDEFINED" align:#left
	label rollback_status_LBL "ROLLBACK STATUS:" align:#left
	label localBuildNumber_crytools_LBL "LOCAL BUILD #: " align:#left
	label latestbuildnumber_crytools_LBL "LATEST BUILD #:" align:#left
	label latest_build_crytools_LBL "LATEST BUILD ON SERVER: " align:#left
	--button refresh "REFRESH" offset:[140,-20]
	--button getPoses "Get latest (P4)" offset:[120,-493] --enabled:false
	
	on diagnostics open do
		(
			
			mat_dump = #("true","false","crytools.warnmats:")
			
			--warn mats set
			if crytools.warnmats == true then
			(
				warnMatsCheck.checked = true
			)
			else
			(
				warnMatsCheck.checked = false
			)
			
			--reparent twist set
			if crytools.reparenttwist == true then
			(
				reparenttwistcheck.checked = true
			)
			else
			(
				reparenttwistcheck.checked = false
			)
			
			--check before export
			if crytools.checkbeforeexport == true then
			(
				checkBeforeExport.checked = true
			)
			else
			(
				checkBeforeExport.checked = false
			)

			--suppress warnings set
			if crytools.suppresswarnings == true then
			(
				suppressWarningsCheck.checked = true
			)
			else
			(
				suppressWarningsCheck.checked = false
			)
			
			--do not unparent weapon_bone children
			if crytools.nounparentw == true then
			(
				noUnparentWeapons.checked = true
			)
			else
			(
				noUnparentWeapons.checked = false
			)
			
			--ActivateAnimTools
			if crytools.loadoldanimtools == true then
			(
				loadOldAnimTools.checked = true				
			)
			else
			(
				loadOldAnimTools.checked = false
			)
			
			--show splash
			if crytools.showSplash == true then
			(
				showSplashCheck.checked = true
			)
			else
			(
				showSplashCheck.checked = false
			)
			/*
			--update collections
			if crytools.updateCollections == true then
			(
				updateCollectionsCheck.checked = true
			)
			else
			(
				updateCollectionsCheck.checked = false
			)
			*/
			
			/*
			--sync collections
			if crytools.syncCollections == true then
			(
				syncCollectionsAtLoad.checked = true
			)
			else
			(
				syncCollectionsAtLoad.checked = false
			)
			*/
			maxDirTxt_LBL.text = ("MAX PATH:   " + crytools.maxDirTxt)
			maxVersionNum_crytools_LBL.text = ("MAX VERSION:   " + crytools.maxVersionNum as string)
			
			--crytools.rollback_status
			if crytools.rollback_status == undefined then
			(
				rollback_status_LBL.text = ("ROLLBACK STATUS:   UNDEFINED")
			)
			else
			(
				rollback_status_LBL.text = ("ROLLBACK STATUS:   " + crytools.rollback_status)
			)
				
			--crytools.project_name
			/*if crytools.project_name == undefined then
			(
				project_name_crytools_LBL.text = ("PROJECT:   UNDEFINED")
			)
			else
			(
				project_name_crytools_LBL.text = ("PROJECT:   " + crytools.project_name)
			)*/
			/*
			if crytools.project_name == undefined then
			(
				projectEnter.text = NONE
			)
			else
			(
				projectEnter.text = crytools.project_name
			)
			*/
			
			if crytools.BuildPathFull == undefined then
				projectEnter.text = "NONE"
			else
				projectEnter.text = crytools.BuildPathFull
			
				
			--DOMAIN
			if crytools.DOMAIN == undefined then
			(
				domain_LBL.text = ("DOMAIN:   UNDEFINED")
			)
			else
			(
				domain_LBL.text = ("DOMAIN:   " + crytools.DOMAIN)
			)
		
			cryINI_LBL.text = ("CRYEXPORT.INI PATH:   " + crytools.cryINI)
			cryToolsINI_LBL.text = ("CRYTOOLS.INI PATH:   " + (sysInfo.tempDir + "cry_temp\\crytools.ini"))
			
			editorpath_LBL.text = ("EDITOR PATH:  " + crytools.editorPath)
			cbaPath_LBL.text = ("CBA PATH: " + crytools.cbaPath)
			
			--crytools.BuildPathFull
			if crytools.BuildPathFull ==undefined then
			(	
				BuildPathFull_LBL.text = ("BUILD PATH:   UNDEFINED")
			)
			else
			(
				BuildPathFull_LBL.text = ("BUILD PATH:   " + crytools.BuildPathFull)
			)
				
			if crytools.latest_build != undefined then
			(
				localBuildNumber_crytools_LBL.text = ("LOCAL BUILD #:   " + crytools.localBuildNumber as string)
				latestbuildnumber_crytools_LBL.text = ("LATEST BUILD #:   " + crytools.latestbuildnumber as string)
				latest_build_crytools_LBL.text = ("LATEST BUILD ON SERVER:   " + crytools.latest_build)
			)
			
			-- in from updateTools
			current_exportTXT.text = ("LOCAL BUILD: " + crytools.localBuildNumber as String+ "    LATEST BUILD: " + crytools.latestbuildnumber as String)
			if crytools.rollback_status == "true" do (rollback_exporter.checked = true)
			if crytools.rollback_status == "false" do (rollback_exporter.checked = false)
		)
		
		on getPoses pressed do
		(
			p4Update = ("p4 sync " + crytools.BuildPathFull + "_Production\\Art\\Animation\\Human\\Resources\\poses\\...")
			crytools.scmd p4Update true
		)
		
		on dumpCryToolsGlobals pressed do
		(
			print (apropos "crytools")
		)
		
		on callbacklist pressed do
		(
			print (callbacks.show())
		)
		
		on callbackRemove rightclick do
		(
			callbacks.removescripts()
		)
		
		--warn mats checked
		on warnMatsCheck changed state do
		(
			cryTools.outToINI "CryTools" "warnmats" (state as String)
			crytools.warnmats = state
		)
		
		--reparent checked
		on reparentTwistCheck changed state do
		(
			cryTools.outToINI "CryTools" "reparent" (state as String)
			crytools.reparenttwist = state
		)
		
		--no unparent $weapon_bone checked
		on noUnparentWeapons changed state do
		(
			cryTools.outToINI "CryTools" "no_unparent_weapons" (state as String)
			crytools.nounparentw = state
		)
		
		--Activate Anim Tools
		on loadOldAnimTools changed state do
		(
			cryTools.outToINI "CryTools" "loadOld_animTools" (state as String)
			if loadOldAnimTools.checked == false then
			(
				try closeRolloutfloater CryAnimationTools catch()
				
				try
				(
					filein (crytools.BuildPathFull + "Tools\\maxscript\\cryAnim\\load.ms")
				)
				catch ( print "No  load.ms   in  cryAnim found" )
			)
			else
			(
				if cryTools.cryAnim != undefined then
				(
					cryTools.cryAnim.base.killCryAnim()
					filein (crytools.BuildPathFull + "Tools\\maxscript\\CryAnimationTools.ms")
				)
			)
			cryTools.loadoldanimtools = state
			crytools.generateMenu()
		)
		
		
		--check before export
		on checkBeforeExport changed state do
		(
			cryTools.outToINI "CryTools" "checkExport" (state as String)
			crytools.checkbeforeexport = state
		)

		
		--suppress warnings checked
		on suppressWarningsCheck changed state do
		(
			cryTools.outToINI "CryTools" "suppress" (state as String)
			crytools.suppresswarnings = state
		)
		
		--show splash checked
		on showSplashCheck changed state do
		(
			cryTools.outToINI "CryTools" "splash" (state as String)
			crytools.showSplash = state
		)
		
		/*
		--auto update collections
		on updateCollectionsCheck changed state do
		(
			if updateCollectionsCheck.checked == false then
			(
				oldData = (crytools.inFromINI (sysInfo.tempDir + "cry_temp\\crytools.ini") false)
				oldData[11] = "UPDATE_COLLECTIONS:false"
				crytools.outtoini oldData (sysInfo.tempDir + "cry_temp\\crytools.ini") false
				crytools.updateCollections = false
				callbacks.removescripts #filePostOpen id:#updateCollections	
			)
			else
			(
				oldData = (crytools.inFromINI (sysInfo.tempDir + "cry_temp\\crytools.ini") false)
				oldData[11] = "UPDATE_COLLECTIONS:true"
				crytools.outtoini oldData (sysInfo.tempDir + "cry_temp\\crytools.ini") false
				crytools.updateCollections = true
				txt = "if $bip01 != undefined then (\n"
				txt += "biped_ctrl = $bip01.controller\n"
				txt += "biped.deleteallcopycollections biped_ctrl\n"
				txt += "try(\n"
				txt += "biped.loadCopyPasteFile biped_ctrl \"J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male.cpy\"\n"
				txt += "biped.loadCopyPasteFile biped_ctrl \"J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_female.cpy\"\n"
				txt += "biped.loadCopyPasteFile biped_ctrl \"J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male_combat.cpy\"\n"
				txt += "biped.loadCopyPasteFile biped_ctrl \"J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male_crouch.cpy\"\n"
				txt += "biped.loadCopyPasteFile biped_ctrl \"J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male_prone.cpy\"\n"
				txt += "biped.loadCopyPasteFile biped_ctrl \"J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male_relaxed.cpy\"\n"
				txt += "biped.loadCopyPasteFile biped_ctrl \"J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male_stealth.cpy\")\n"
				txt += "catch (messagebox \"Cannot locate pose files\"))"
				callbacks.addscript #filePostOpen txt id:#updateCollections
			)
		)
		*/
		
		/*
		on syncCollectionsAtLoad changed state do
		(
			if syncCollectionsAtLoad.checked == false then
			(
				oldData = (crytools.inFromINI (sysInfo.tempDir + "cry_temp\\crytools.ini") false)
				oldData[12] = "SYNC_COLLECTIONS:false"
				crytools.outtoini oldData (sysInfo.tempDir + "cry_temp\\crytools.ini") false
				crytools.syncCollections = false
			)
			else
			(
				oldData = (crytools.inFromINI (sysInfo.tempDir + "cry_temp\\crytools.ini") false)
				oldData[12] = "SYNC_COLLECTIONS:true"
				crytools.outtoini oldData (sysInfo.tempDir + "cry_temp\\crytools.ini") false
				crytools.syncCollections = true
			)
		)
		*/
		
		on pickProject pressed do
		(
			local tempVar = (getSavePath caption:"Project Directory" initialDir:crytools.BuildPathFull)
			if tempVar != undefined then
			(
				if tempVar[tempVar.count] != "\\" then
					append tempVar "\\"
				--crytools.BuildPathFull = tempVar
				projectEnter.text = tempVar
			)
		)
		
		
		on setProject pressed do
		(
			local buildPathNew = ""
			local buildPathFilter = filterString projectEnter.text "\\"
			for i = 1 to (buildPathFilter.count - 1) do
				buildPathNew += buildPathFilter[i] + "\\"
			
			buildPathNew += buildPathFilter[buildPathFilter.count]
			
			if (queryBox ("Changing your project path to a bad location can render yout tools unusable.\nThis effects not only CryTools, but CryTif and others.\n\nAre you sure you would like to change your path to:\n" + buildPathNew) title:"Tread Carefullly.." beep:true) == true then
			(
				if crytools.maxversionnum >= 10 then
				(
					registry.openKey HKEY_CURRENT_USER "Software\\Crytek\\Settings\\" accessRights:#all key:&key1
					registry.setValue key1 "RootPath" #REG_SZ buildPathNew
				)
				else
				(
					messagebox "The ability to edit the registry has been limited to versions of Max10 and later.\nWe used to dynamically generate, execute, and delete VBScripts to accomplish this.\nWindows Vista does not like this, and it was a hack anyway."
					return undefined
				)
			)
			else
			(
				return undefined
			)
			
			local animlistPathNew = buildPathNew + "Game\Animations\Animations.cba"
			
			local folderArray = getDirectories (buildPathNew + "*")
			local folderArrayNew = #()
			if folderArray.count > 0 then
			(
				for i = 1 to folderArray.count do
				(
					tempArray = #(folderArray[i])
					--// Add subFolderFiles to list
					join tempArray (getDirectories (folderArray[i] + "*" ))
					
					join folderArrayNew tempArray
				)
				
			)
			
			
			local RCPath = ""
			
			for i = 1 to folderArrayNew.count do
			(
				
				local tempArray = getFiles (folderArrayNew[i] + "*.*")
				for f = 1 to tempArray.count do
				(
					if (findString tempArray[f] "rc.exe") != undefined then
					(
						RCPath = folderArrayNew[i]
						exit
					)
					if RCPath != "" then
						exit
				)
			)
			
			local editorPathNew = ""
			
			if RCPath != "" then
			(
				
				editorPathFilter = filterString RCPath "\\"
				for i = 1 to (editorPathFilter.count - 1) do
					editorPathNew += editorPathFilter[i] + "\\"
				
				editorPathNew += "Editor.exe"
			)
			
			/*setINISetting (getDir #maxroot + "Plugins\\CryExport.ini") "SandBox" "path" editorPathNew			
			cryTools.editorPath = editorPathNew
			
			setINISetting (getDir #maxroot + "Plugins\\CryExport.ini") "SandBox" "buildPath" buildPathNew
			cryTools.buildPathFull = buildPathNew
			
			setINISetting (getDir #maxroot + "Plugins\\CryExport.ini") "SandBox" "animlistpath" animlistPathNew
			cryTools.cbapath = animlistPathNew*/
			
			fileIn (getDir #maxroot + "scripts\\startup\\loadCryTools.ms")
		)
		
		--------------------------------------------------------------------------
		-- UPDATE / UNINSTALL / ROLLBACK
		--------------------------------------------------------------------------
		
		on BuildOn changed state do
		(
			 if BuildOn.checked == true then
			 (
			 	PerfOn.checked = false
				HTTPOn.checked = false
			 )
		)
		on PerfOn changed state do
		(
			if PerfOn.checked == true then
			(
			 	BuildOn.checked = false
				HTTPOn.checked = false
			)
		)
		on HTTPOn changed state do
		(
			if HTTPOn.checked == true then
			(
				PerfOn.checked = false
				BuildOn.checked = false
			)
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
					crytools.scmd p4Update true
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
							crytools.scmd (("copy /Y \\\\storage\\builds\\" + crytools.latest_build + "\\Tools\\CryExport8.dlu ") + (crytools.BuildPathFull + "Tools\\")) true
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
				crytools.scmd ("mkdir \"" + sysInfo.tempDir + "cry_temp\\bad\\\"") true
				crytools.scmd ("move /Y " + ("\"" + crytools.maxDirTxt + "plugins\\CryExport8.dlu\"") + " " + (sysInfo.tempDir + "cry_temp\\bad\\")) true
				crytools.scmd ("move /Y " + ("\"" +sysInfo.tempDir + "cry_temp\\CryExport8.dlu\"") + " " + (crytools.maxDirTxt + "plugins\\")) true
				print "CryExport8.dlu has been rolled back to the previous version."
				output_rollbackINI = openfile (sysInfo.tempDir + "\\cry_temp\\crytools.rollback_status.ini") mode:"w"
				format crytools.rollback_status to: output_rollbackINI
				close output_rollbackINI
		
				messageBox "CryExport8.dlu has been rolled back to the previous version.\nTo get a newer exporter later you must click \"Get Latest Tools From crytools.alienBrain/Current Build\", or update your build." title: "CryExport8.dlu Rolled Back!"
				)
				else
				(
				crytools.rollback_status = "false"
				output_rollbackINI = openfile (sysInfo.tempDir + "\\cry_temp\\crytools.rollback_status.ini") mode:"w"
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
					deleteFile "$UI\\MacroScripts\\CryTools-SceneBrowser.mcr"
					deleteFile "$UI\\MacroScripts\\CryTools-help.mcr"
					deleteFile "$UI\\MacroScripts\\CryTools-CryMorphManager.mcr"
					deleteFile "$UI\\MacroScripts\\CryTools-CryInfoLoader.mcr"
					deleteFile "$UI\\MacroScripts\\CryTools-CryInfo.mcr"
					deleteFile "$UI\\MacroScripts\\CryTools-CryArtistTools.mcr"
					deleteFile "$UI\\MacroScripts\\CryTools-ControlPanel.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-UVscaleUniform.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-UVcollapseVertical.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-CryKeys-UVcollapseHorizontal.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-showVertexColors.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-resetXformCollapse.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-preserveUV.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-exportNodes.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-exportAnim.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-changeRefCoordSys.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-CenterPivot.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-showHideVertexColors.mcr"
					deleteFile "$UI\\MacroScripts\\CryKeys-UVcollapseHorizontal.mcr"
					crytools.maxDirTxt = (getdir #maxroot)
					crytools.minusr (crytools.maxDirTxt + "scripts\\startup\\LoadCryTools.ms")
					sleep 1
					deleteFile (crytools.maxDirTxt + "scripts\\startup\\LoadCryTools.ms")
					print (sysInfo.username + " has uninstalled CryTools.")
					destroyDialog areYouSure
					--destroyDialog checkForUpdate
					messageBox ("CryExport8.dlu is still in your plugins folder because it is in use.\n" + sysInfo.username + ", cryTools has been uninstalled.") title: "Uninstallation complete!"
				)
			)
			createDialog areYouSure 300 60 bgcolor:black fgcolor:white
		)


		/*on button refresh pressed do
		(
			filein (crytools.BuildPathFull + "tools\\maxscript\\Diagnostics.ms")
		)*/
		
	)
createDialog diagnostics 350 600 style:#(#style_resizing,#style_titlebar,#style_minimizebox,#style_sunkenedge,#style_sysmenu)
