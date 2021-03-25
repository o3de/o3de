-------------------------------------------------------------------------------
--	CryRiggingTools.ms
--	Version 2.5 External
--	by Christopher Evans
-------------------------------------------------------------------------------

global CryRiggingTools

udpInARR = #()
testFor = #()

fn ANoon_EnvelopeCallbackFunction =
(
WindowHandle = DialogMonitorOPS.GetWindowHandle()
theDialogName = UIAccessor.GetWindowText WindowHandle
if theDialogName != undefined and matchpattern theDialogName pattern:"*Load Envelopes*" do
UIAccessor.PressButtonByName WindowHandle "OK"
true
)

(--begin local scope
	crytools.maxDirTxt = (getdir #maxroot)

-------------------------------------------------------------------------------
--	Define functions
-------------------------------------------------------------------------------
	fn getStart =
	(
		animEnd = (animationrange.start as string)
		animEndArray = filterString animEnd "f"
		return (animEndArray[1] as float)
	)
	
	fn getEnd =
	(
		animEnd = (animationrange.end as string)
		animEndArray = filterString animEnd "f"
		return (animEndArray[1] as float)
	)
	
-------------------------------------------------------------------------------
--	CryRiggingTools interface
-------------------------------------------------------------------------------

	if CryRiggingTools != undefined do ( closerolloutfloater CryRiggingTools )
	CryRiggingTools = newrolloutfloater "CryRiggingTools 2.5" 190 815
	
--InternalTools - these are nothing special; just internal tools that are specific to our projects
-------------------------------------------------------------------------------
	rollout InternalTools "Internal Tools"
	(
		group "Helper Joints"
		(
			button showHelpers "Hide/Show helper joints" enabled:false
			button selectHelpers "Select helper joints"
			button UnSelectHelpers "Unselect helper joints"
		)
	
		group "File Fixes/Cleanup"
		(
			button mirror_weapon "Mirror Weapon Bone"
			button selectWeaponPos "Select New Attachment Points"
			checkbox deleteWeaponPos "delete/add"
			checkbox hideWeaponPos "hide/show" offset:[80,-20]
			button addTwistWire "Wire FP Hands Twist Bones"
			button renameDummies "Comment Out Nub Bones"
		)
		
		group "Locomotion Manager"
		(
			button locoChannel "Extract Data to LocoMan Node"
			checkbox lockX "Lock to X"
			checkbox lockY "Lock to Y" offset:[75,-20]
			checkbox ignoreXrot "Ignore Root Rotation"
			checkbox placeOnGround "Restrict to Ground Plane"
			checkbox useHead "Use Head for Direction"
			checkbox useRoot "Use Root for Direction"
			checkbox freezeLock	"Freeze and Lock"
		)
		group "Remove Unwanted Bones"
		(
			button removeBones "Remove Bones"
			checkbox remBonesChk "Remove from Skin"
			checkbox delBones "Delete bone objects"
		)
		
		on InternalTools open do
		(
			placeOnGround.checked = true
			useroot.checked = true
			freezelock.checked = true
		)
		
		--unselect helpers pressed
		on UnselectHelpers pressed do
		(
			deselect $'Bip01 L clavicular deltoid01'
			deselect $'Bip01 L clavicular deltoid02'
			deselect $'Bip01 R clavicular deltoid01'
			deselect $'Bip01 R clavicular deltoid02'
			deselect $'Bip01 R rear deltoid01'
			deselect $'Bip01 R rear deltoid02'
			deselect $'Bip01 L rear deltoid01'
			deselect $'Bip01 L rear deltoid02'
			deselect $'Bip01 L Lat Control'
			deselect $'Bip01 L Pec Control'
			deselect $'Bip01 R Pec Control'
			deselect $'Bip01 R Lat Control'
			deselect $'Bip01 L knee'
			deselect $'Bip01 R knee'
		)
	
		on selectHelpers pressed do
		(	
			select $'Bip01 L clavicular deltoid01'
			selectMore $'Bip01 L clavicular deltoid02'
			selectMore $'Bip01 R clavicular deltoid01'
			selectMore $'Bip01 R clavicular deltoid02'
			selectMore $'Bip01 R rear deltoid01'
			selectMore $'Bip01 R rear deltoid02'
			selectMore $'Bip01 L rear deltoid01'
			selectMore $'Bip01 L rear deltoid02'
			selectMore $'Bip01 L Lat Control'
			selectMore $'Bip01 R Lat Control'
			selectMore $'Bip01 L knee'
			selectMore $'Bip01 R knee'
			selectMore $'Bip01 R clavicular deltoid01_end'
			selectMore $'Bip01 R rear deltoid01_end'
			selectMore $'Bip01 R rear deltoid02_end'
			selectMore $'Bip01 R Lat Control_end'
			selectMore $'Bip01 R Pec Control_end01'
			selectMore $'Bip01 L Pec Control_end'
			selectMore $'Bip01 L Lat Control_end'
			selectMore $'Bip01 R clavicular deltoid_end'
			selectMore $'Bip01 L rear deltoid02_end'
			selectMore $'Bip01 L rear deltoid01_end'
			selectMore $'Bip01 R knee_end'
			selectMore $'Bip01 L knee_end'
			
			if $'Bip01 R Pec Control' == undefined then
			(
				selectMore $'Rope1 Seg1'
				selectMore $'Rope1 Seg2'
				selectMore $'Rope2 Seg1'
				selectMore $'Rope2 Seg2'
			)
			else
			(
				selectMore $'Bip01 L Pec Control'
				selectMore $'Bip01 R Pec Control'
			)
	
		)
	
	--	Add New Attachment Points
		on addWeapons pressed do
		(
			undo "add weaponPos attachments" on
			(
				if (doesfileexist (crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max")) != false then
				(
					mergeMAXfile (crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max") #("weaponPos_hurricane", "weaponPos_rifle02", "weaponPos_rifle01", "weaponPos_pistol_R_hip", "weaponPos_pistol_R_leg", "weaponPos_grenade_R_hip", "weaponPos_law", "weaponPos_grenade_L_hip", "weaponPos_pistol_L_hip", "weaponPos_pistol_L_leg") #alwaysReparent 
				)
				else
				(
					messagebox ("Cannot find " + crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max")
				)
			)
		)
		
		on selectWeaponPos pressed do
		(
			if deleteweaponpos.checked == true then
			(
				if $weaponPos_pistol_R_hip == undefined then
				(
					undo "add weaponPos attachments" on
					(
						if (doesfileexist (crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max")) != false then
						(
							mergeMAXfile (crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max") #("weaponPos_hurricane", "weaponPos_rifle02", "weaponPos_rifle01", "weaponPos_pistol_R_hip", "weaponPos_pistol_R_leg", "weaponPos_grenade_R_hip", "weaponPos_law", "weaponPos_grenade_L_hip", "weaponPos_pistol_L_hip", "weaponPos_pistol_L_leg") #alwaysReparent 
						)
						else
						(
							messagebox ("Cannot find " + crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max")
						)
					)
				)
				else
				(
					undo "delete weaponPos attachments" on
					(
						delete $weaponPos*
					)
				)
			)
			
			if hideweaponpos.checked == true then
			(
				if $weaponPos_law.ishidden == true then
				(
					unhide $weaponpos*
				)
				else
				(
					hide $weaponPos*
				)
			)
			
			else
			(
				select $weaponPos*
			)
		)
		
	--	mirror weapon bone
		on mirror_weapon pressed do
		(
			try
				with undo "weapon_bone" on
				(
					weapon_pos = in coordsys parent $weapon_bone.pos
					maxops.cloneNodes $weapon_bone clonetype:#copy actualNodelist:$weapon_bone
					$weapon_bone01.name = "alt_weapon_bone01"
					$alt_weapon_bone01.parent = $'Bip01 L Hand'
					crytools.MirrorObjs $alt_weapon_bone01 $weapon_bone $Bip01 #y #y
					in coordsys parent $alt_weapon_bone01.pos = weapon_pos
					rot_90 = eulerangles 0 0 -90
					in coordsys local rotate $weapon_bone rot_90
					in coordsys local rotate $alt_weapon_bone01 rot_90
				)
			catch
				(
					messageBox "Please load a biped." title:"Error"
				)
		)
		
		on addTwistWire pressed do
		(
			paramWire.connect $hand_R.rotation.controller[#X_Rotation] $twist03_R.rotation.controller[#X_Rotation] "X_Rotation*.6"
			paramWire.connect $hand_R.rotation.controller[#X_Rotation] $twist02_R.rotation.controller[#X_Rotation] "X_Rotation*.4"
			paramWire.connect $hand_R.rotation.controller[#X_Rotation] $twist01_R.rotation.controller[#X_Rotation] "X_Rotation*.25"
			
			paramWire.connect $hand_L.rotation.controller[#X_Rotation] $twist03_L.rotation.controller[#X_Rotation] "X_Rotation*.6"
			paramWire.connect $hand_L.rotation.controller[#X_Rotation] $twist02_L.rotation.controller[#X_Rotation] "X_Rotation*.4"
			paramWire.connect $hand_L.rotation.controller[#X_Rotation] $twist01_L.rotation.controller[#X_Rotation] "X_Rotation*.25"
		)
		
		on ignoreXrot changed state do
		(
			if ignorexrot.checked == true then
			(
				useroot.checked = false
				usehead.checked = false
				useroot.enabled = false
				usehead.enabled = false
			)
			else
			(
				useroot.enabled = true
				usehead.enabled = true
				usehead.checked = true
			)
		)
		
		on usehead changed state do
		(
			useroot.checked = false
		)
		on useroot changed state do
		(
			usehead.checked = false
		)
		
		-- add loco channel
		on locoChannel  pressed do
		(
			if $Locator_Locomotion == undefined then
			(
				if (doesfileexist (crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max")) != false then
				(
					mergeMAXfile (crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max") #("Locator_Locomotion")
				)
				else
				(
					messagebox ("Cannot find " + crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max")
				)
			)
			
			for i = (getstart()) to (getend()) do
			(
				with animate on
				(
					slidertime = i
					$Locator_Locomotion.transform = $Bip01.transform
					
					--ignore rotation
					if ignoreXrot.checked == true then
					(
						in coordsys world $Locator_Locomotion.rotation.controller.x_rotation = 0
						in coordsys world $Locator_Locomotion.rotation.controller.y_rotation = 0
						--in coordsys world $Locator_Locomotion.rotation.controller.z_rotation = -90
					)
					
					if lockX.checked == true then
					(
						in coordsys world $Locator_Locomotion.position.controller.x_position = 0
					)
					
					if lockY.checked == true then
					(
						in coordsys world $Locator_Locomotion.position.controller.y_position = 0
					)
					
					if useRoot.checked == true then
					(
						in coordsys world $Locator_Locomotion.rotation.controller.x_rotation = 0
						in coordsys world $Locator_Locomotion.rotation.controller.y_rotation = 0
						in coordsys world $Locator_Locomotion.rotation.controller.z_rotation += -90
						
					)
					
					if useHead.checked == true then
					(
						headXrot = $'Bip01 Head'.transform.rotation as eulerangles
						in coordsys world $Locator_Locomotion.rotation.controller.x_rotation = 0
						in coordsys world $Locator_Locomotion.rotation.controller.y_rotation = 0
						in coordsys world $Locator_Locomotion.rotation.controller.z_rotation = headXrot.x
					)
					
					--place on ground
					if placeOnground.checked == true then
					(
						in coordsys world $Locator_Locomotion.position.controller.z_position = 0
					)
				)
			)
			if freezeLock.checked == true then
			(
				$Locator_Locomotion.isfrozen = true
				$Locator_Locomotion.showfrozeningray = false
			)
		)
		
		on removeBones pressed do
		(
			if remBonesChk.checked == true then
			(
				remBones = $Bip*nub as array
				join remBones ($_* as array)
				join remBones ($weapon* as array)
				join remBones ($alt_weapon* as array)
				--join remBones ($bip01 as array)
				
				for obj in selection do
				(	
					modPanel.setCurrentObject obj.modifiers[#Skin]
					undo "Remove Bones" on
					(
						for u=1 to remBones.count do
						(
							for i=1 to (skinOps.getNumberBones obj.skin) do
							(
								if (skinOps.GetBoneName obj.skin i 0) == remBones[u].name then
								(
									skinOps.removeBone obj.skin i
									print ("removing " + remBones[u].name)
									print i
									exit loop
								)
							)
						)
					)
				)
			)
			if delBones.checked == true then
			(
				remBones = $Bip*nub as array
				join remBones ($_* as array)
				--join remBones ($weapon* as array)
				for obj in remBones do
				(
					delete obj
				)
			)
		)
		
		on renameDummies pressed do
		(
			try ($'Bip01 HeadNub'.name = "_Bip01 HeadNub") catch()
			try ($'Bip01 R Finger0Nub'.name = "_Bip01 R Finger0Nub") catch()
			try ($'Bip01 R Finger1Nub'.name = "_Bip01 R Finger1Nub") catch()
			try ($'Bip01 R Finger2Nub'.name = "_Bip01 R Finger2Nub") catch()
			try ($'Bip01 R Finger3Nub'.name = "_Bip01 R Finger3Nub") catch()
			try ($'Bip01 R Finger4Nub'.name = "_Bip01 R Finger4Nub") catch()
			try ($'Bip01 L Finger0Nub'.name = "_Bip01 L Finger0Nub") catch()
			try ($'Bip01 L Finger1Nub'.name = "_Bip01 L Finger1Nub") catch()
			try ($'Bip01 L Finger2Nub'.name = "_Bip01 L Finger2Nub") catch()
			try ($'Bip01 L Finger3Nub'.name = "_Bip01 L Finger3Nub") catch()
			try ($'Bip01 L Finger4Nub'.name = "_Bip01 L Finger4Nub") catch()
			--try ($'Bip01 R Toe0Nub'.name = "_Bip01 R Toe0Nub") catch()
			--try ($'Bip01 L Toe0Nub'.name = "_Bip01 L Toe0Nub") catch()
			try ($'Bip01 L Heel'.name = "_Bip01 L Heel") catch()
			try ($'Bip01 R Heel'.name = "_Bip01 R Heel") catch()
			try ($'Bip01LToeHelper'.name = "_Bip01LToeHelper") catch()
			try ($'Bip01RToeHelper'.name = "_Bip01RToeHelper") catch()
		)
	)

-------------------------------------------------------------------------------
--RiggingTools
-------------------------------------------------------------------------------
	rollout RiggingTools "Rigging Tools"
	(
		group "General Tools"
		(
			button matchPivotBtn "matchPivot" offset:[-45,0]
			button zeroOut	"zeroOut Rots" offset:[35,-26]
			button clampTimeline "Clamp Timeline at Current"
			button clampTimelineKey "Clamp Timeline at Last Key"
			button controllerXferChild "Copy/Past Controller"
			checkbox copyPos "Pos" checked:true offset:[0,0]
			checkbox copyRot "Rot" checked:true offset:[50,-20]
			checkbox copyScale "Scale" checked:false enabled:false offset:[100,-20]
			checkbox toChildren "to children" checked:true offset:[0,0]
			checkbox toSelection "to selected" enabled:false offset:[80,-20]
			button reduceKeysBTN "Reduce Keys on Sel"
		)
		
		group "Biped Tools"
		(
			dropdownlist bipSelect "" items:#("") width:130 offset:[-5,0]
			button refreshBipTools "<" offset:[65,-27] tooltip:"Refresh"
			checkbutton figureMode "Figure Mode" offset:[-40,0]
			checkbutton hideBiped " Hide Biped " offset:[40,-26]
			button bipMotion "Bip Motion Menu"
			button selectAllBIP "Select Biped Bones"
			checkbutton inPlace "Toggle In Place Mode"
			button convertBip "Convert Biped to Bones"
		)
		
		group "Rotate Bind Pose"
		(
			button rootBTN "Load Selected Skeleton Root"
			label rotBindLBL "No skeleton loaded."
			spinner xVal "X" type:#float range:[-358,358,0] fieldWidth:35 offset:[-103,0] enabled:false
			spinner yVal "Y" type:#float range:[-358,358,0] fieldWidth:35 offset:[-48,-21] enabled:false
			spinner zVal "Z" type:#float range:[-358,358,180] fieldWidth:35 offset:[8,-21] enabled:false
			button rotBindSel "   Rotate Selected Meshes   " enabled:false
		)
		
		group "Bone Tools"
		(
			label boneadjustLBL "Adjust Bones:" align:#left
			spinner boneTaper "Taper:" type:#float fieldWidth:25 offset:[-6,-18]
			spinner boneWidth "Width:" type:#float fieldWidth:25 range:[0,500,1] offset:[-85,0]
			spinner boneHeight "Height:" type:#float fieldwidth:25 range:[0,500,1] offset:[-6,-21]
			button nodes2bones "Convert Hierarchy to Bones"
			checkbox fromSelected "fromSelected" align:#left checked:true
			checkbox fromRoot "fromRoot" offset:[90,-20]
		)
		
		group "Vertex Tools"
		(
			button channelInfoBTN "channelInfo" offset:[-40,0]
			checkbutton vertColors "vertexColors" offset:[40,-26]
			button addChannel "Add cryChannel to Selected" enabled:false
			label cryChannel "There is currently no cryChannel"
		)
	
	--	on rigging tools open
			on RiggingTools open do
			(	
				if $bip01 != undefined then
				(
					if (LayerManager.getLayerFromName "Bip") == undefined then
					(
						messageBox "The BIP layer is not named \"Bip\"" title: "Error"
						return undefined
					)
					bip_layer = LayerManager.getLayerFromName "Bip"
					if bip_layer.ishidden == true then
					(
						hideBiped.checked = true
					)
					
					biped_ctrl=$bip01.controller
					if biped_ctrl.figureMode == true then
					(
						figureMode.checked = true
					)
					else
					(
						figureMode.checked = false
					)
					
					figureSet = "if $bip01 != undefined then\n"
					figureSet += "(\n"
					figureSet += "if $bip01.controller.figureMode == true then (CryRiggingTools.rollouts[2].figureMode.checked = true) else (CryRiggingTools.rollouts[2].figureMode.checked = false)\n"
					figureSet += ")\n"
					figureSet += "else (print \"no biped found\")"
					callbacks.addscript #filePostOpen figureSet id:#figureMode
					
					BIPhideSet = "bip_layer = LayerManager.getLayerFromName \"Bip\"\n"
					BIPhideSet += "if bip_layer == undefined then\n"
					BIPhideSet += "(\n"
					BIPhideSet += "if bip_layer.ishidden == true then (CryRiggingTools.rollouts[2].hideBiped.checked = true) else (CryRiggingTools.rollouts[2].hideBiped.checked = false)\n"
					BIPhideSet += ")\n"
					BIPhideSet += "else (print \"there is no bip layer\")"
					callbacks.addscript #filePostOpen BIPhideSet id:#bipHide
					
					global bips = (crytools.getBips())
					namearr = #()
					for i=1 to bips.count do
					(
						append namearr bips[i].name
					)
					bipSelect.items = namearr
					global selectedBip = ("$'" + bips[1].name + "'")
					global biped_ctrl = (execute (selectedBip + ".controller"))
				)
				
				
			--get rigging tools location
				--get rigging tools location
				local tempVar = cryTools.inFromINI "CryTools" "rigging_pos"
				if tempVar == "" then tempVar = [1000,0]
				else
				(
					try
					(
						tempVar = execute tempVar
						cryriggingtools.pos = tempVar
					)
					catch
					(
						cryTools.outToINI "CryTools" "rigging_pos" "[1000,0]"
						cryriggingtools.pos = [1000,0]
					)
				)
			)
			
	-- on rigging tools closed
			on RiggingTools close do
			(
				cryTools.outToINI "CryTools" "rigging_pos" (cryriggingtools.pos as String)

				callbacks.removescripts #filePostOpen id:#figureMode
				callbacks.removescripts #filePostOpen id:#bipHide
			)
			
	-- on clampTimeline pressed
		on clampTimeline pressed do
		(
			if animationrange.start == slidertime then
			(
				messagebox ("Timeline cannot be clamped to a single frame (" + slidertime as string + ")")
				return undefined
			)
			animationrange = interval animationrange.start slidertime
		)
	
	-- clampTimelineKey 
		on clampTimelineKey  pressed do
		(
			if $ == undefined then
			(
				messagebox "Nothing is selected"
				return undefined
			)
			
			try
			(
				if $.controller.keys.count == -1 then
				(
					positionK = $.position.controller.keys[$.position.controller.keys.count].time
					rotationK = $.rotation.controller.keys[$.rotation.controller.keys.count].time
					if positionK > rotationK then
					(
						if positionK == animationrange.start then (messagebox "Cannot clamp to timeline start"; return undefined)
						animationrange = interval animationrange.start positionK
						return undefined
					)
					else
					(
						if positionK == animationrange.start then (messagebox "Cannot clamp to timeline start"; return undefined)
						animationrange = interval animationrange.start rotationK
						return undefined
					)
				)
				
				if $ != undefined or $.controller.keys[$.controller.keys.count].time != animationrange.start then
				(
					animationrange = interval animationrange.start $.controller.keys[$.controller.keys.count].time
				)
				else
				(
					messagebox "Object's last keyframe is current key.\nCannot clamp timeline to 1 key"
				)
			)
			catch()
		)
		
		--controller copy/paste
		on controllerXferChild pressed do
		(
			if toChildren.checked == true then
			(
				undo "copy/paste controller" on
				(
					for mObj in selection do
					(
						if copyPos.checked and copyRot.checked == true then
						(
							crytools.copyPasteController mObj "children" "rp"
						)
					)
				)
			)
			if toSelection.checked == true then
			(
				
			)
		)
		
	--reduce keys
		on reduceKeysBTN pressed do
		(
			try
			(
				undo "reduceKeys" on
				(
					for obj in selection do
					(
						if copyPos.checked == true then
						(
							reduceKeys obj.position.controller 50 1f
						)
						if copyRot.checked == true then
						(
							reduceKeys obj.rotation.controller 50 1f
						)
					)
				)
			)
			catch()
		)
	
	--	crytools.matchPivot button
		on matchPivotBtn pressed do
		(
			if selection.count == undefined or selection.count != 2 then
			(
				messageBox "Please select two objects, the second being the object whos pivot you want to change." title: "Error"
				return undefined
			)
			undo "matchPivot" on
			(
				crytools.matchPivot $[1] $[2]
			)
		)
				
	--	zero out button
		on zeroOut pressed do
		(
		    objs = getCurrentSelection();
			oc = objs.count;
		   	undo "ZEROOut" on
			(
		   		setWaitCursor();
				for i in 1 to oc do 
			    (
					pt = Point pos:[0,0,0] isSelected:on centermarker:on axistripod:off cross:off Box:off constantscreensize:off drawontop:off size:20;
					pt.parent = objs[i].parent;
					coordsys world 
					(
						pt.scale = objs[i].scale;
						pt.rotation = objs[i].rotation;
						pt.position = objs[i].position;
						pt.wirecolor = (color 80 10 0);
						pt.name = (objs[i].name+"_ZERO");
					)
					objs[i].parent = pt;
			    )
				select objs;
				setArrowCursor();
			)
		)
	
		on refreshBipTools pressed do
		(
			try
			(
				global bips = (crytools.getBips())
				global namearr = #()
				for i=1 to bips.count do
				(
					append namearr bips[i].name
				)
				bipSelect.items = namearr
				global selectedBip = ("$'" + bips[1].name + "'")
				global biped_ctrl = (execute (selectedBip + ".controller"))
			)
		catch(print "no bips selected/found")
		)
	
		on bipSelect selected s do
		(
			print (bipSelect.items[s] + " selected.")
			selectedBip = ("$'" + bips[s].name + "'")
			global biped_ctrl = (execute (selectedBip + ".controller"))
		)
	
	-- figure mode button
			on figureMode changed state do
			(
			try
				if (figureMode.checked == true) then 
				(
					biped_ctrl=(execute (selectedBip + ".controller"))
					biped_ctrl.figureMode = true
				)
				else
				(
					biped_ctrl=(execute (selectedBip + ".controller"))
					biped_ctrl.figureMode = false
				)
				catch
				( 
					messageBox "Please load a biped." title:"Error"
					figuremode.Checked = false
					return undefined
				)
			)
	-- hide biped button
		on hideBiped  changed state do
		(
			try
			(
				if (hideBiped.checked == true) then
				(
					bip_layer = LayerManager.getLayerFromName "Bip"
					bip_layer.ishidden = true
				)
				else
				(
					bip_layer = LayerManager.getLayerFromName "Bip"
					bip_layer.ishidden = false
				)
			)
			catch
			(
				messageBox "Cannot find 'Biped' layer." title:"Error"
				hideBiped.checked = false
				return undefined
			)
		)
	
	--	bip motion menu button
		on bipMotion pressed do
		(
			try
			(
				selectedbipOrig = selectedbip as string
				selectedbip = crytools.cutstring selectedbip "$"
				selectedbip = crytools.cutstring selectedbip "'"
				selectedbip = crytools.cutstring selectedbip "'"
				select (getnodebyname selectedbip)
				max motion mode
				selectedbip = selectedbipOrig 
			)
			catch
			( 
				messageBox "Please load a biped." title:"Error"
				return undefined
			)
		)
			
	-- in place mode button
		on inPlace changed state do
		(
			try
				if (inPlace.checked == true) then 
				(
					biped_ctrl.inPlaceMode = true
				)
				else
				(
					biped_ctrl.inPlaceMode = false
				)
			catch
			( 
				messageBox "Please load  a biped, or take the biped out of figure mode." title:"Error"
				inPlace.checked = false
				return undefined
			)
		)
		
		on convertBip pressed do
		(
			--try
			--(
				undo "bip2bones" on
				(
					newbones = #()
					
					if selection == undefined then
					(
						messagebox "nothing is selected"
						return undefined
					)
					
					if selection.count != 1 then
					(
						messagebox "please select only one part of the biped to convert"
						return undefined
					)
					
					for obj in crytools.getChildren (cryTools.findRoot $) do
					(
						b = snapshot obj name:("bone_" + (obj.name as string))
						b.parent = undefined
						b.transform = obj.transform
						append newbones b
					)

					for i in newbones do
					(
						if (getnodebyname (crytools.cutstring i.name "bone_")).parent != undefined then
						(
							i.parent = getnodebyname ("bone_" + ((getnodebyname (crytools.cutstring i.name "bone_")).parent).name)
						)
					)
				)
			--)
			--catch()
		)
			
		on selectAllBIP pressed do
		(
			selectionN = #()
			for obj in $* do
			(
				if obj.classid[1] == 37157 then
				(
					append SelectionN obj
				)
			)
			select SelectionN
		)
		
		--rotate bind pose
		on rootBTN pressed do
		(
			if selection.count != 1 then
			(
				messagebox "Please select a root skeleton node"
				return undefined
			)
			else
			(
				rootBTN.text = $.name
				rotBindLBL.text = "Select character meshe(s)"
				rotBindSel.enabled = true
				xVal.enabled = true
				yVal.enabled = true
				zVal.enabled = true
			)
		)
		
		on rotBindSel pressed do
		(
			DialogMonitorOPS.RegisterNotification ANoon_EnvelopeCallbackFunction ID:#ANoon_Envelopes
			DialogMonitorOPS.Enabled = true
			
			if selection.count != 0 then
			(
				undo "flipCHR" on
				(
					items = selection as array
					
					if items.count < 2 then
					(
						if items[1].name == rootBTN.text then
						(
							messagebox ("Please select character meshes to be rotated\n(root currently selected (" + rootBTN.text + ")")
							return undefined
						)
						crytools.rotBind items[1] (getnodebyname rootBTN.text) 0 0 180 true
					)
					else
					(
						for i=1 to (items.count-1) do
						(
							crytools.rotBind items[i] (getnodebyname rootBTN.text) 0 0 180 false
						)
						crytools.rotBind items[items.count] (getnodebyname rootBTN.text) 0 0 180 true
					)
				)
			)
			DialogMonitorOPS.Enabled = false
			DialogMonitorOPS.UnRegisterNotification ID:#ANoon_Envelopes
		)
		
		-- bone tools
		on boneWidth changed val do
		(
			if $ != undefined then
			(
				for obj in selection do
				(
					if classof obj == BoneGeometry then obj.width = val
				)
			)
		)
			
		on boneHeight changed val do
		(
			if $ != undefined then
			(
				for obj in selection do
				(
					if classof obj == BoneGeometry then obj.height = val
				)
			)
		)
		
		on nodes2bones pressed do
		(
			undo "nodes2bones" on
			(
				if $ == undefined then
				(
					messagebox "please select a node"
					return undefined
				)
				
				if fromSelected.checked == false then
				(
					crytools.nodes2bones (crytools.findroot $)
				)
				else
				(
					crytools.nodes2bones $
				)
			)
		)
		
		on fromSelected changed state do
		(
			if fromselected.checked == true then fromRoot.checked = false
		)
		
		on fromRoot changed state do
		(
			if fromRoot.checked == true then fromSelected.checked = false
		)
	
		on boneTaper changed val do
		(
			if $ != undefined then
			(
				for obj in selection do
				(
					if classof obj == BoneGeometry then obj.taper = val
				)
			)
		)
			
		--	vertex tools
		on channelInfoBTN pressed do
		(
			channelInfo.Dialog ()
		)
			
		on vertColors changed state do
		(
			for obj in selection do
			(
				if obj.showVertexColors == false then
				(
					obj.showVertexColors = true
				)
				else
				(
					obj.showVertexColors = false
				)
			)
		)
			
		on addChannel pressed do
		(
			numChannels = (meshop.getNumVDataChannels $)
			channelinfo.addchannel $
			numChannels += 1
			cryChannel.text = ("cryChannel: channel " + (numchannels as string))
			channelinfo.NameChannel $ 3 numchannels "cry"
			channelinfo.update()
			--s.vertexColorType = #alpha
		)
			
	) -- End RiggingTools


-------------------------------------------------------------------------------
--Physics
-------------------------------------------------------------------------------
	rollout PhysicsSetup "Physics Setup"
	(
		button createPhys "Create Phys Skeleton"
		button createParentFrame "Create ParentFrame"
		
		on createPhys pressed do
		(
			if $ != undefined then
			(
				local root
				undo "createPhys" on
				(
					root = crytools.findRoot selection[1]
					parts = crytools.getChildren root
					parentRef = #(undefined)
					partRef = #()
					
					snapshot root name:((root.name as string) + " phys")
					
					for i = 1 to parts.count do
					(
						append parentRef (parts[i].parent.name + " phys")
						new = snapshot parts[i] name:((parts[i].name as string) + " phys")
						append partRef new
					)
					
					for i = 2 to partRef.count do
					(
						partRef[i].parent = getnodebyname (parts[i].parent.name + " phys")
					)
				)
				select (getnodebyname ((root.name as string) + " phys"))
			)
		)
		
		on createParentFrame pressed do
		(
			if selection != undefined then
			(
				undo "createParentFrame" on
				(
					for obj in selection do
					(
						frame = snapshot obj name:((obj.name as string) + " parentFrame")
						frame.parent = obj.parent
						obj.parent = frame
						--frame.ishidden = true
						--frame.isfrozen = true
					)
				)
			)
		)
	)
-------------------------------------------------------------------------------
--DiagnosticTools
-------------------------------------------------------------------------------
	rollout Diagnostic "Diagnostic Tools"
	(
	
	group "General Diagnostics"
	(
		button selectRoot "selectRoot" offset:[-45,0]
		button selChildren "selectChildren" offset:[35,-26]
		button numChildren "numChildren" offset:[-41,0]
		label numChildrenLabel "---" offset:[5,-20]
		checkbox printChildren "print" offset:[100,-20]
		label compareLabel "Compare Two Hierarchies:"
		checkbutton compare01 " Heirarchy1 " offset:[-40,0]
		button compare02 " Heirarchy2 " offset:[40,-26]
		checkbox checkConsistency "check consistency" enabled:false
	)
	
	on compare01 changed state do
	(
		if $ == undefined then
		(
			messagebox "Please select a hierarchy member!"
			compare01.checked = false
			return undefined
		)
		
		if compare01.checked == true then
		(
			comp1root = crytools.findRoot $
			print (comp1root.name + " hierarchy loaded.")
			tempArray = crytools.getChildren comp1root
			global crytools_compareArray1 = #()
			for obj in tempArray do
			(
				append crytools_compareArray1 obj.name
			)
		)
	)
	on compare02 pressed do
	(
		if $ == undefined then
		(
			messagebox "Please select a hierarchy member!"
			return undefined
		)
		comp2root = crytools.findRoot $
		compareArray2 = crytools.getChildren comp2root
		print (comp2root.name + " heirarchy loaded.")
		isIdentical = "yes"
		for obj in compareArray2 do
		(
			if (findItem crytools_compareArray1 obj.name) == 0 do
			(
				print obj.name
				isIdentical = "no"
			)
		)
		if isIdentical == "yes" then
		(
			print "hierarchy elements are identical"
		)
	)
	
	--	numChildren
	on numChildren pressed do
	(
		if $ == undefined or selection.count > 1 do
		(
			print "Select 1 node."
			numChildrenLabel.text = "--"
			return undefined
		)
		comp2root = crytools.findRoot $
		childArray = crytools.getChildren comp2root
		numChildrenLabel.text = (childArray.count as string)
		if printChildren.checked == true then
		(
			for obj in childArray do print obj.name
		)
	)
	
	on selChildren pressed do
	(
		selectMe = selection as array
		for obj in selection do
		(
			join selectMe (crytools.getChildren obj)
		)
		select selectMe
	)
	
	on selectRoot pressed do
	(
		if selection.count > 1 then
		(
			messagebox "Please select one node of the hierarchy"
			return undefined
		)
		if $ == undefined or $.parent == undefined then
		(
			print "Object has no parent or nothing is selected"
			return undefined
		)
		root_ = crytools.findRoot $
		select root_
	)
	
	/*group "Physique Diagnostics"
	(
		label label_bone01 "Select Verts that use X bones"
		editText buneNum_text fieldWidth:20 offset:[20,0]
		button fetchVertsBtn "Select" offset:[0,-24]
		label label_bone02 "Get Physique Bone Count"
		button boneCount "GetBoneCount" offset:[10,0]
		label bone_txt "0" offset:[-50,-20]
		checkbutton initialPose "Go to Initial Pose"
		button zeroVerts "Select verts with 0.0 weight"
	)
		
	group "Animation Diagnostics"
	(
		button getWeaponRoot "Get Weapon\Root Info" --pos:[0,0]
		label positions "POS: " offset:[-53,0]
		editText weapon_txt offset:[30,-20] fieldWidth:45
		editText root_txt offset:[80,-22] fieldWidth:45
		label translations "TRANS: " offset:[-60,0]
		editText weapon_trans_txt offset:[30,-20] fieldWidth:45
		editText root_trans_txt offset:[80,-22] fieldWidth:45
	)*/
	
	on boneCount pressed do
	(
	try (bone_txt.text = ((physiqueOps.getbonecount $) as string))
	catch
		( 
			messageBox "Please Select a character (mesh) with a Physique modifier." title:"Error"
			return undefined
		)
	)
	
	on fetchVertsBtn pressed do
	(
		vertSel = #{}
		numberOfBones = buneNum_text.text as float
		for i=1 to (polyOp.getNumVerts $) do
		(
			if (physiqueops.getvertexbonecount $ i) == numberOfBones then
			(
				append vertSel i
			)
			$.selectedVerts = vertsel
		)
	)
	
	on zeroVerts pressed do
	(
		if $selection.count > 0 then
		(
			vertSel = #{}
			for i = 1 to (polyOp.getNumVerts $) do
			(
				if (physiqueops.getvertexbonecount $ i) == 1 then
				(
					--bonesUsing = physiqueops.getVertexBones $ i
					if physiqueops.getVertexWeight  $ i 1 == 0.0 then
					(
						append vertSel i
					)
				)
				$.selectedVerts = vertsel
			)
		)
		else
			messageBox "Select Node before." title:"Error"
	)
	
	on getWeaponRoot pressed do
	(
		try
		(
			weapon_txt.text = ((in coordsys world $weapon_bone.position.z) as string)
			alt_weapon_txt = ((in coordsys world $alt_weapon_bone01.position.z) as string)
			
			bip01txt = ((in coordsys world $Bip01) as string)
			bip01txtArray = filterstring bip01txt ",]["
			rootZloc = bip01txtArray[4]
			root_txt.text = (rootZloc as string)
			
			print ("Frame: " + (slidertime as string))
			print ("root " + (root_txt.text as string))
			print ("weapon_bone " + (weapon_txt.text as string))
			print ("alt_weapon_bone01 " + (alt_weapon_txt as string))
			
			slidertime = animationrange.start
			w1trans1 = (in coordsys world $weapon_bone.position.x)
			w2trans1 = (in coordsys world $alt_weapon_bone01.position.x)
			bip01txt = ((in coordsys world $Bip01) as string)
			bip01txtArray = filterstring bip01txt ",]["
			rootXloc = bip01txtArray[2]
			rtrans1 = rootXloc
			
			slidertime = animationrange.end
			w1trans2 = (in coordsys world $weapon_bone.position.x)
			w2trans2 = (in coordsys world $alt_weapon_bone01.position.x)
			bip01txt = ((in coordsys world $Bip01) as string)
			bip01txtArray = filterstring bip01txt ",]["
			rootXloc = bip01txtArray[2]
			rtrans2 = rootXloc
			
			root_trans_txt.text = ((abs ((rtrans1 as float) - (rtrans2 as float))) as string)
			weapon_trans_txt.text = ((abs (w1trans1 - w1trans2)) as string)
			print ("root translation " + ((rtrans1 as float) - (rtrans2 as float)) as string)
			print ("weapon_bone translation " + (w1trans1 - w1trans2) as string)
			print ("alt_weapon_bone01 translation " + (w2trans1 - w2trans2) as string)
		)catch()
	)
	
	on initialPose changed state do
		(
		try
		(
			if (initialPose.checked == true) then 
			(
			physiqueOps.setInitialPose $ true
			)
			else
			(
			physiqueOps.setInitialPose $ false
			)
		)
		catch
		( 
			messageBox "Please Select a character (mesh) with a Physique modifier." title:"Error"
			return undefined
		)
		)
		
	group "Attachment\\Game Diagnostics"
		(
		label cdf_names "NAME: " offset:[-60,0]
		editText cdfName_txt offset:[30,-20] fieldWidth:110
		button cdfData "Generate CDF Attachment Data"
		)
	
	on Diagnostic open do
		(
		cdfName_txt.text = "Name of attachment."
		)
	
	on cdfData pressed do
		(
		if $ == undefined then
		(
		messageBox "Please Select an object" title:"Error"
		return undefined
		)
		global world_locArr = filterstring ((in coordsys world $.position) as string) "[]"
		quatRotArray = filterstring ((in coordsys world $.rotation) as string) ",() "
		global quatRot = ((quatRotArray[5] as string) + "," + (quatRotArray[2] as string) + "," + (quatRotArray[3] as string) + "," + (quatRotArray[4] as string))
		--print ("<Attachment AName=\"" + cdfName_txt.text + "\" Binding=\"" + (maxFilePath + $.name) + ".cdf\" BoneName=\"" + ($.parent.name) + "\" Position=\"" + (world_locArr[1] as string) + "\" Rotation=\"" + quatRot + "\" Type=\"CA_BONE\" />")
		
		rollout assumeHelp "Is this the file path of the CDF you are attaching?"
			(
			label assume01 "finding..." align:#center --Assumed from max file directory
			button yesCDF "Yes" align:#center offset: [-35,0]
			button noCDF "No, let me choose." align:#center offset:[40,-26]
			on yesCDF pressed do
				(
				global cdf_location = assume01.text
				global cdf_location_name = (getFilenameFile cdf_location)
				destroyDialog assumeHelp
					rollout assumeHelp2 "Is this the bone the CDF is attached to?"
					(
					label assume01 "finding..." align:#center --Assumed from max file directory
					button yesBONE "Yes" align:#center offset: [-40,0]
					pickbutton noBONE "No, Pick Bone" align:#center offset:[30,-26]
					on yesBONE pressed do
						(
						global parent_bone = assume01.text
						destroyDialog assumeHelp2
							rollout CDF_info_win "Generated CDF Bone Attachment info:"
							(
								edittext cdfInfoWin_txt text:"generating..." fieldWidth:390 height:260 pos:[1,3]
								
								on CDF_info_win open do
								(
								cdfInfoWin_txt.text = 	print ("<Attachment AName=\"" + cdfName_txt.text + "\" Binding=\"" + cdf_location + " \" BoneName=\"" + parent_bone + "\" Position=\"" + (world_locArr[1] as string) + "\" Rotation=\"" + quatRot + "\" Type=\"CA_BONE\" />")
								)
								
								on CDF_info_win resized size do
								(
									size1 = size as string
									size2 = filterstring size1 "[],"
									cdfInfoWin_txt.width = ((size2[1] as float) - 10)
									cdfInfoWin_txtheight = ((size2[2] as float) - 35)
								)
							)
							createDialog CDF_info_win 400 295 bgcolor:black fgcolor:white style:#(#style_resizing, #style_titlebar, #style_border, #style_sysmenu)
						)
					on noBONE picked obj do
						(
						try
							(
							global parent_bone = obj.name
							destroyDialog assumeHelp2
								rollout CDF_info_win "Generated CDF Bone Attachment info:"
								(
									edittext cdfInfoWin_txt text:"generating..." fieldWidth:390 height:260 pos:[1,3]
									
									on CDF_info_win open do
									(
									cdfInfoWin_txt.text = 	print ("<Attachment AName=\"" + cdfName_txt.text + "\" Binding=\"" + cdf_location + " \" BoneName=\"" + parent_bone + "\" Position=\"" + (world_locArr[1] as string) + "\" Rotation=\"" + quatRot + "\" Type=\"CA_BONE\" />")
									)
									
									on CDF_info_win resized size do
									(
										size1 = size as string
										size2 = filterstring size1 "[],"
										cdfInfoWin_txt.width = ((size2[1] as float) - 10)
										cdfInfoWin_txtheight = ((size2[2] as float) - 35)
									)
								)
							)
							catch
							(
							print "Nothing selected."
							return undefined
							)
							createDialog CDF_info_win 400 295 bgcolor:black fgcolor:white style:#(#style_resizing, #style_titlebar, #style_border, #style_sysmenu)
						)
					on assumeHelp2 open do
						(
						assume01.text = $.parent.name
						)
					)
					createDialog assumeHelp2 300 50 bgcolor:black fgcolor:white
				)
				
			on noCDF pressed do
				(
				global cdf_location = getMAXSaveFileName caption:"C:"
				if cdf_location == undefined then
				(
				messageBox "You didn't select a file.." title:"Bad Monkey.."
				return undefined
				)
				global cdf_location_name = (getFilenameFile cdf_location)
				destroyDialog assumeHelp
				rollout assumeHelp2 "Is this the bone the CDF is attached to?"
					(
					label assume01 "finding..." align:#center --Assumed from max file directory
					button yesBONE "Yes" align:#center offset: [-40,0]
					pickbutton noBONE "No, Pick Bone" align:#center offset:[30,-26]
					on yesBONE pressed do
						(
						global parent_bone = assume01.text
						destroyDialog assumeHelp2
						)
					on noBONE picked obj do
						(
						global parent_bone = obj.name
						destroyDialog assumeHelp2
						)
					on assumeHelp2 open do
						(
						assume01.text = $.parent.name
						)
					)
					createDialog assumeHelp2 300 50 bgcolor:black fgcolor:white
				)
				
			on assumeHelp open do
				(
				assume01.text = (maxFilePath + $.name + ".cdf")
				)
			)
			createDialog assumeHelp 600 65 bgcolor:black fgcolor:white
		)
	) --end DiagnosticTools
	
-------------------------------------------------------------------------------
--	Reactor Tools
-------------------------------------------------------------------------------
	rollout reactorTools "Reactor Tools"
	(
		pickbutton addRigid "Add Objs to Rigid Body"
		pickbutton addFracture "Add Objs to Fracture"
		
		on addFracture picked obj do
		(
			try
			(
				undo "addFracture" on
				(
					for i=1 to selection.count do
					(
						obj.addpiece selection[i]
					)
				)
			)
			catch()
		)
		on addRigid picked obj do
		(
			try
			(
				undo "addRigid" on
				(
					for i=1 to selection.count do
					(
						obj.addRigidBody selection[i]
					)
				)
			)
			catch()
		)
	)
	
-------------------------------------------------------------------------------
--	Smart Object Template Generator
-------------------------------------------------------------------------------
	rollout SmartObjects "Smart Object Tools"
	(
		group "Template Generator/Exporter"
		(
			button smartObjectManager "Smart Object Manager"
			button selectSmartObj "Get Smart Obj Geometry"
			button addStartStop "Add Start/Stop Locations" enabled:false
			--TEMPORARY DEACTIVATED button addEdge "Add Edge" offset:[-36,0] enabled:false
			--TEMPORARY DEACTIVATED button addEdgePoint "Add Point" offset:[36,-26] enabled:false
			checkbox projectOnGround "Project on Ground" enabled:false
			checkbox rotate180z "Flip Around Z Axis" enabled:false
			button exportSOdata "Export Smart Object Data" enabled:false
		)
		
		
		on smartObjectManager pressed do
		(
			try destroyDialog SOManagerRO catch()


			rollout SOManagerRO "Smart Object Manager"
			(
				local mergedSO = ""
				local rootSO = ""
				
				label labPath "Folder: " pos:[8,10]
				edittext edPath "" pos:[40,8] fieldWidth:330
				--label edPath "" pos:[55,10] width:330
				button btnBrowsePath "Browse" pos:[382,6] width:50 height:20
				
				groupbox gbList " Smart Object List " pos:[8,35] height:195 width:140
				listbox lbList "" pos:[12,50] height:13 width:132
				
				groupbox gbPreview " Preview " pos:[158,35] height:285 width:274
				bitmap bmpPreview "" pos:[162,50] height:266 width:266
				
				button btnRoot "Root" pos:[12,240] width:132 height:20 enabled:false
				button btnEdit "Edit" pos:[12,260] width:66 height:20 enabled:false
				button btnEditXML "XML" pos:[78,260] width:66 height:20 enabled:false
				button btnMerge "Merge" pos:[12,280] width:132 height:20 enabled:false
				button btnExport "Export to FBX" pos:[12,300] width:132 height:20 enabled:false
			
				
				function updateDialog =
				(
					try
					(
						if lbList.selection > 0 then
						(
							btnEditXML.enabled = true
							
							local tempFilename = (filterString lbList.items[lbList.selection] ".")[1]
							
							if (getFiles (edPath.text + "\\" + tempFilename + ".max")).count > 0 then
							(
								btnEdit.enabled = true
								btnMerge.enabled = true
								btnExport.enabled = true
								btnRoot.enabled = true
								
								if mergedSO == lbList.items[lbList.selection] then
								(
									if rootSO != "" then
										btnRoot.text = rootSO.name
									else
										btnRoot.text = "Root"
								)
								else
									btnRoot.text = "Root"
							)
							else
							(
								btnEdit.enabled = false
								btnMerge.enabled = false
								btnExport.enabled = false
								btnRoot.enabled = false
							)
							
							
							local SOPicture = (getFiles (edPath.text + "\\" + tempFilename + ".jpg"))[1]
							if SOPicture != undefined then
								try ( local tempBitmap = openBitMap (edPath.text + "\\" + tempFilename + ".jpg") )catch()
							else
								tempBitmap = bitmap 266 266 color:(color 190 190 190)
							
							bmpPreview.bitmap = tempBitmap
							
						)
						else
						(
							btnEdit.enabled = false
							btnEditXML.enabled = false
							btnMerge.enabled = false
							btnExport.enabled = false
						)
					)
					catch()
				)
				
				
				function updatePath initial:undefined =
				(
					try
					(
						local tempPath = cryTools.buildPathFull + "Game\\Libs\\SmartObjects\\ClassTemplates"
						if edPath.text != "" then
							tempPath = edPath.text
						
						if initial == undefined then
							local openPath = getSavePath caption:"Open .xml directory of smart objects" initialDir:tempPath
						else
							local openPath = tempPath
						
						if openPath != undefined then
						(
							local xmlList = getFiles (openPath + "\\*.xml")
							if xmlList.count == 0 then
								messageBox "No .xml files in the directory.\n\nPlease select a different folder" title:"Error in Smart Object Manager"
							else
							(
								edPath.text = openPath
								
								for i = 1 to xmlList.count do
								(
									local filterFilename = filterString xmlList[i] "\\"
									xmlList[i] = filterFilename[filterFilename.count]
								)
							
								lbList.items = xmlList
								if lbList.items.count > 0 then
								(
									lbList.selection = 1
									updateDialog()
								)
							)
						)
					)
					catch (messageBox "Error in updatePath function")
				)
				
				
				
				on SOManagerRO open do
				(
					updatePath initial:true
				)
				
				
				on btnBrowsePath pressed do
				(
					updatePath()
				)
				
				on lbList selected value do
				(
					updateDialog()
				)
				
				on btnRoot pressed do
				(
					local tempNode = selectByName title:"Select the Root Node of the Smart Object" showHidden:true single:true
					if tempNode != undefined then
					(
						btnRoot.text = tempNode.name
						rootSO = tempNode
					)
					
				)
				
				
				on btnEdit pressed do
				(
					try
					(
						local tempFilename = (filterString lbList.selected ".")[1]
						loadMaxFile (edPath.text + "\\" + tempFilename + ".max") 
						mergedSO = lbList.selected
						rootSO = getNodeByName ((cryTools.sortRootChildren (Objects as Array))[1])
						
						updateDialog()
					)catch()
				)
				
				
				on btnEditXML pressed do
				(
					cryTools.scmd ("notepad \"" + edPath.text + "\\" + lbList.selected + "\"") false
				)
				
				
				
				on btnMerge pressed do
				(
					try
					(
						local tempFilename = (filterString lbList.selected ".")[1]
						mergeMaxFile (edPath.text + "\\" + tempFilename + ".max") #select
						mergedSO = lbList.selected
						rootSO = getNodeByName ((cryTools.sortRootChildren (selection as Array))[1])
						
						clearSelection()
						updateDialog()
					)catch()
				)
				
				
				on btnExport pressed do
				(
					local tempFilename = (filterString lbList.selected ".")[1]
					
					if rootSO == "" then
					(
						if (queryBox "When continuing, the export will load the SO and overwrites the current scene.\n\nDo you want to continue?" title:"SO Manager") == true then
							btnEdit.pressed()
						else
							return false
					)
					
					local exportPath = getSaveFileName filename:(edPath.text + "\\" + tempFilename + ".fbx") caption:"Export Smart Object to FBX" types:"Autodesk (*.FBX)|*.fbx"
					
					if exportPath != undefined then
					(
						with redraw off
						(
							clearSelection()
							
							local SOArray = #(rootSO)
							join SOArray (cryTools.getChildren rootSO)
							selectMore SOArray
							
							exportFile exportPath #noPrompt selectedOnly:true using:FBXEXP
							
							clearSelection()
						)
					)
				)
			)

			createDialog SOManagerRO height:330 width:440
		)
		
		
		on selectSmartObj  pressed do
		(
			if $ == undefined then
			(
				messagebox "Please select an object to be defined as the Smart Object mesh/CGF."
				return undefined
			)
			
			t = $.pos
			if (t.x*t.x + t.y*t.y + t.z*t.z) > 0.0001 then
			(
				messagebox "ERROR: Object's pivot is not at origin (0,0,0)"
				return undefined
			)

			if selection.count != 1 then
			(
				messagebox "You can only export one mesh as a Smart Object CGF"
				return undefined
			)
			global smartObj_cryGlobal = $
			selectSmartObj.text = ($.name)
			addStartStop.enabled = true
			addEdge.enabled = true
			exportSOdata.enabled = true
			projectOnGround.enabled = true
			rotate180z.enabled = true
		)
		
		on SmartObjects open do
		(
			global numSO_cryGlobal = 0
			global startEndSO_cryGlobal = #()
			if initialDirSO_cryGlobal == undefined then
			(
				global initialDirSO_cryGlobal = crytools.BuildPathFull
			)
			projectOnGround.checked = true
			rotate180z.checked = true
		)
		
		on addStartStop pressed do
		(
			undo "add start/stop SO objects" on
			(
				numSO_cryGlobal += 1
				Circle radius:20 pos:[100,0,0] wirecolor:[0,255,0] name:("so_start_" + (numSO_cryGlobal as string)) isselected:true 
				setTransformLockFlags (getnodebyname ("so_start_" + (numSO_cryGlobal as string))) #{3}
				$.parent = smartObj_cryGlobal
				if projectOnGround.checked == true then
				(
					setUserPropBuffer $ "1"
				)
				else
				(
					setUserPropBuffer $ "0"
				)
				
				Circle radius:20 pos:[-100,0,0] wirecolor:[255,0,0] name:("so_end_" + (numSO_cryGlobal as string)) isselected:true
				setTransformLockFlags (getnodebyname ("so_end_" + (numSO_cryGlobal as string))) #{3}
				$.parent = smartObj_cryGlobal
				if projectOnGround.checked == true then
				(
					setUserPropBuffer $ "1"
				)
				else
				(
					setUserPropBuffer $ "0"
				)
				
				deselect $
			)
		)
		
		/*TEMPORARY DEACTIVATED
		on addEdge pressed do
		(
			local numberEdges = 0
			for obj in Objects do
				if (findString obj.name "so_startEdge") != 0 then
					try
					(
						if ((local extractNumber = (subString obj.name 13 2) as Integer)) > numberEdges then
							numberEdges = extractNumber
					)
					catch()
			
			
			
			numberEdges += 1
			
			print numberEdges
			
					
			undo "add edge SO object" on
			(
				local numberEdgesString = (if numberEdges < 10 then "0" else "") + numberEdges as String
				in coordsys world
				(
					local startEdge = dummy pos:[50,30,0] name:("so_startEdge"+ numberEdgesString) isselected:false
					local endEdge = dummy pos:[50,-30,0] name:("so_endEdge" + numberEdgesString) isselected:false
					
					--local edgeLine = line pos:[50,30,0] name:("so_lineEdge" + numberEdgesString) isselected:false
					
					
					local edgeSpline = splineShape pos:[0,50,0] wirecolor:[0,150,0] isselected:true rotation:(quat 0 0 0.707107 0.707107)
					--edgeSpline.rotation = 
					addnewSpline edgeSpline
					addKnot edgeSpline 1 #corner #line [50,30,0]
					addKnot edgeSpline 1 #corner #line [50,-30,0]
				)
			)
			
			
		)
		*/
		
		
		
		on exportSOdata pressed do
		(
			try
			(
				t = $.pos
				if (t.x*t.x + t.y*t.y + t.z*t.z) > 0.0001 then
				(
					messagebox "ERROR: Object's pivot is not at origin"
					return undefined
				)
				
				soSavePath = getSavePath caption:("Save " + (smartObj_cryGlobal.name + ".xml") + " to:") initialDir:initialDirSO_cryGlobal
				if soSavePath == undefined then
				(
					return undefined
				)
				if rotate180z.checked == true then
				(
					smartObj_cryGlobal.rotation = quat 0 0 1 0
				)
				initialDirSO_cryGlobal = soSavePath 
				
				soData = #("<sotemplate>",("\t<object model=\"" + smartObj_cryGlobal.name + ".cgf\" />"))
				startEndSO_cryGlobal = ($so_* as array)
				for obj in startEndSO_cryGlobal do
				(
					append soData ("\t<helper name=\"" + obj.name as string + "\" pos=\"" + (filterstring ((obj.pos/100) as string) "[]")[1] + "\" rot=\"" + (obj.rotation.w as string + "," + obj.rotation.x as string + "," + obj.rotation.y as string + "," + obj.rotation.z as string) + "\" radius=\"" + (obj.radius/100) as string + "\" projectOnGround=\"" + (getUserPropBuffer obj) +"\" />")
				)
				append soData "</sotemplate>"
				print (soSavePath + "\\" + (smartObj_cryGlobal.name + ".xml"))
				cryTools.writeOUT soData (soSavePath + "\\" + (smartObj_cryGlobal.name + ".xml")) echo:false
				if rotate180z.checked == true then
				(
					smartObj_cryGlobal.rotation = quat 0 0 1.50996e-007 1
				)
			)
			catch()
		)
	)
	
	
-------------------------------------------------------------------------------
--CGF Metadata Manager
-------------------------------------------------------------------------------
	
	rollout metadataManager "CGF Metadata Manager"
	(
		label updateLBL "Info will be updated here"
		button udpSetToSel "     Apply Settings to Selected     " height:25
		checkbox useMass "Mass:" offset:[-10,0]
		spinner objMass "" type:#float range:[0,99999,0] fieldWidth:50 offset:[-40,-20]
		checkbox useDensity "Density:" offset:[-10,0]
		spinner objDensity "" type:#float range:[0,99999,0] fieldWidth:50 offset:[-40,-20]
		--button udpGetFromSelBTN  "GET" offset:[67,-26]
		group "Force Primitives"
		(
			checkbox box1 "box"
			checkbox cylinder1 "cylinder" offset:[40,-20]
			checkbox sphere1 "sphere" offset:[100,-20]
			checkbox capsule1 "capsule"
		)
		
		group "Joints"
		(
			checkbox useJoint "Enable joint properties"
			spinner jointLimit "Limit:" type:#float fieldWidth:35 offset:[-84,0]
			spinner jointTwist "Twist:" type:#float fieldWidth:35 offset:[0,-21]
			spinner jointBend "Bend:" type:#float fieldWidth:35 offset:[-84,0]
			spinner jointPull "Pull:" type:#float fieldWidth:35 offset:[0,-21]
			spinner jointPush "Push:" type:#float fieldWidth:35 offset:[-84,0]
			spinner jointShift "Shift:" type:#float fieldWidth:35 offset:[0,-21]
		)
		group "General"
		(
			checkbox isPickable "pickable"
		)
		group "destroyableObjects"
		(
			checkbox isMain "main"
			checkbox isRemain "remain" offset:[47,-20]
			spinner destroyCount "Count:" type:#float fieldWidth:35 offset:[-75,0]
			label spawnLocTXT "__Spawn Location_____________"
			checkbox isEntity "entity"
			checkbox isBone "bone"
			edittext isBoneTXT "" offset:[45,-21] fieldWidth:60
			pickbutton pickBone "pick" offset:[62,-24]
			label rotAxesTXT "rot axes" offset:[-60,0]
			checkbutton xrot "x" offset:[-25,-21]
			checkbutton yrot "y" offset:[0,-26]
			checkbutton zrot "z" offset:[25,-26]
			spinner sizeVar "size var:" type:#float fieldWidth:35 offset:[-70,0]
		)
		button selectAllMatching "Select Objects Matching Details"
		button openMetaDataLister "Open CGF Metadata Lister"
		
		on metadataManager open do
		(
			isBoneTXT.enabled = false
			isBoneTXT.text = "bone name"
			objDensity.value = 1000
			objMass.value = 1
			objDensity.enabled = false
			objMass.enabled = false
			udpGetFromSel()
			
			--set callback for dynamic change based on selection
			callbacks.removeScripts id:#updateUDP
			callbacks.addScript #selectionSetChanged "udpGetFromSel()" id:#updateUDP
		)
		
		on useMass changed state do
		(
			if useMass.checked == true then
			(
				if useJoint.checked == true then
				(
					messagebox "Joints cannot have a mass or density!"
					useMass.checked = false
					return undefined
				)
				objMass.enabled = true
				objDensity.enabled = false
				useDensity.checked = false
			)
			else
			(
				objMass.enabled = false
			)
		)
		
		on useDensity changed state do
		(
			if useDensity.checked == true then
			(
				if useJoint.checked == true then
				(
					messagebox "Joints cannot have a mass or density!"
					useDensity.checked = false
					return undefined
				)
				objDensity.enabled = true
				objMass.enabled = false
				useMass.checked = false
			)
			else
			(
				objDensity.enabled = false
			)
		)
		
		on udpSetToSel pressed do
		(
			for obj in selection do
			(
				udpInARR = crytools.inFromUDP obj
				
				-- MASS OUTPUT ----------------------------------------
				if useMass.checked == true then
				(
					massVal = ("mass = " + objMass.value as string)
					foundMass = 1
					for obj in selection do
					(
						udpInARR = crytools.inFromUDP obj
						for i = 1 to udpInARR.count do
						(
							testFor = filterstring udpInARR[i] " ="
							if testFor[1] == "mass" then
							(
								foundMass = i
							)
							else
							(
								foundMass = (udpInARR.count + 1)
							)
							
							if testFor[1] == "density" then
							(
								deleteItem udpInARR i
							)
						)
	
						udpInARR[foundMass] = massVal
						crytools.outToUDP udpInARR obj false
						print massVal
					)
				)
				else
				(
					for i = 1 to udpInARR.count do
					(
						testFor = (filterstring udpInARR[i] " =")
						wordFilter = (crytools.forceLowerCase testFor[1])
						case wordFilter of
						(
							"mass":
							(
								deleteItem udpInARR i
								crytools.outToUDP udpInARR obj false
							)
						)
					)
				)
				
				-- DENSITY OUTPUT ----------------------------------------
				if useDensity.checked == true then
				(
					densityVal = ("density = " + objDensity.value as string)
					foundDensity = 1
					for obj in selection do
					(
						udpInARR = crytools.inFromUDP obj
						for i = 1 to udpInARR.count do
						(
							testFor = filterstring udpInARR[i] " ="
							if testFor[1] == "density" then
							(
								foundDensity = i
							)
							else
							(
								foundDensity = (udpInARR.count + 1)
							)
							
							if testFor[1] == "mass" then
							(
								deleteItem udpInARR i
							)
						)
	
						udpInARR[foundDensity] = densityVal
						crytools.outToUDP udpInARR obj false
						print densityVal
					)
				)
				else
				(
					for i = 1 to udpInARR.count do
					(
						testFor = (filterstring udpInARR[i] " =")
						wordFilter = (crytools.forceLowerCase testFor[1])
						case wordFilter of
						(
							"density":
							(
								deleteItem udpInARR i
								crytools.outToUDP udpInARR obj false
							)
						)
					)
				)
				
				-- FORCE PRIMITIVES OUTPUT ----------------------------------------
				if box1.checked == true then
				(
					for obj in selection do
					(
						addBox = 1
						udpInARR = crytools.inFromUDP obj
						for i = 1 to udpInARR.count do
						(
							testFor = (filterstring udpInARR[i] " =")
							wordFilter = (crytools.forceLowerCase testFor[1])
							if wordFilter == "box" then
							(
								addBox = i
							)
							else
							(
								addBox = (udpInARR.count + 1)
							)
						)
						udpInARR[addBox] = "box"
						crytools.outToUDP udpInARR obj false
					)
				)
				else
				(
					for obj in selection do
					(
						udpInARR = crytools.inFromUDP obj
						for i = 1 to udpInARR.count do
						(
							testFor = (filterstring udpInARR[i] " =")
							wordFilter = (crytools.forceLowerCase testFor[1])
							if wordFilter == "box" then
							(
								deleteItem udpInARR i
								crytools.outToUDP udpInARR obj false
							)
						)
					)
				)
				if sphere1.checked == true then
				(
					for obj in selection do
					(
						addsphere = 1
						udpInARR = crytools.inFromUDP obj
						for i = 1 to udpInARR.count do
						(
							testFor = (filterstring udpInARR[i] " =")
							wordFilter = (crytools.forceLowerCase testFor[1])
							if wordFilter == "sphere" then
							(
								addsphere = i
							)
							else
							(
								addsphere = (udpInARR.count + 1)
							)
						)
						udpInARR[addsphere] = "sphere"
						crytools.outToUDP udpInARR obj false
					)
				)
				else
				(
					for obj in selection do
					(
						udpInARR = crytools.inFromUDP obj
						for i = 1 to udpInARR.count do
						(
							testFor = (filterstring udpInARR[i] " =")
							wordFilter = (crytools.forceLowerCase testFor[1])
							if wordFilter == "sphere" then
							(
								deleteItem udpInARR i
								crytools.outToUDP udpInARR obj false
							)
						)
					)
				)
				if cylinder1.checked == true then
				(
					for obj in selection do
					(
						addcylinder = 1
						udpInARR = crytools.inFromUDP obj
						for i = 1 to udpInARR.count do
						(
							testFor = (filterstring udpInARR[i] " =")
							wordFilter = (crytools.forceLowerCase testFor[1])
							if wordFilter == "cylinder" then
							(
								addcylinder = i
							)
							else
							(
								addcylinder = (udpInARR.count + 1)
							)
						)
						udpInARR[addcylinder] = "cylinder"
						crytools.outToUDP udpInARR obj false
					)
				)
				else
				(
					for obj in selection do
					(
						udpInARR = crytools.inFromUDP obj
						for i = 1 to udpInARR.count do
						(
							testFor = (filterstring udpInARR[i] " =")
							wordFilter = (crytools.forceLowerCase testFor[1])
							if wordFilter == "cylinder" then
							(
								deleteItem udpInARR i
								crytools.outToUDP udpInARR obj false
							)
						)
					)
				)
				if capsule1.checked == true then
				(
					for obj in selection do
					(
						addcapsule = 1
						udpInARR = crytools.inFromUDP obj
						for i = 1 to udpInARR.count do
						(
							testFor = (filterstring udpInARR[i] " =")
							wordFilter = (crytools.forceLowerCase testFor[1])
							if wordFilter == "capsule" then
							(
								addcapsule = i
							)
							else
							(
								addcapsule = (udpInARR.count + 1)
							)
						)
						udpInARR[addcapsule] = "capsule"
						crytools.outToUDP udpInARR obj false
					)
				)
				else
				(
					for obj in selection do
					(
						udpInARR = crytools.inFromUDP obj
						for i = 1 to udpInARR.count do
						(
							testFor = (filterstring udpInARR[i] " =")
							wordFilter = (crytools.forceLowerCase testFor[1])
							if wordFilter == "capsule" then
							(
								deleteItem udpInARR i
								crytools.outToUDP udpInARR obj false
							)
						)
					)
				)
			)
		)
		
		on useJoint changed state do
		(
			if (classof $) != Dummy then
			(
				if (classof $) != Point then
				(
					messagebox "Only Helper objects can be joints\nThe selected objects are not all Dummy or Point helpers"
					useJoint.checked = false
					return undefined
				)
			)
			if useJoint.checked == true then
			(
				cryRiggingtools.rollouts[5].jointLimit.enabled = true
				cryRiggingtools.rollouts[5].jointTwist.enabled = true
				cryRiggingtools.rollouts[5].jointBend.enabled = true
				cryRiggingtools.rollouts[5].jointPull.enabled = true
				cryRiggingtools.rollouts[5].jointPush.enabled = true
				cryRiggingtools.rollouts[5].jointShift.enabled = true
			)
			else
			(
				cryRiggingtools.rollouts[5].jointLimit.enabled = false
				cryRiggingtools.rollouts[5].jointTwist.enabled = false
				cryRiggingtools.rollouts[5].jointBend.enabled = false
				cryRiggingtools.rollouts[5].jointPull.enabled = false
				cryRiggingtools.rollouts[5].jointPush.enabled = false
				cryRiggingtools.rollouts[5].jointShift.enabled = false
			)
		)
		
		on isPickable changed state do
		(
			if ispickable.checked == true then
			(
				for obj in selection do
				(
					
				)
			)
		)
		
		--on udpGetFromSelBTN pressed do
		--(
		--	udpGetFromSel()
		--)
		
		on isBone changed state do
		(
			if isBone.checked == true then
			(
				isBoneTXT.enabled = true
			)
			else
			(
				isBoneTXT.enabled = false
			)
		)
		
		--CGF Meta Data Lister
		on openMetaDataLister pressed do
		(
			rollout listview_rollout "CGF MetaData Lister"
			(	
				fn initListView lv =
				(
					lv.gridLines = true  
					lv.View = #lvwReport  
					lv.fullRowSelect = true 
					
					layout_def = #("Node","Mass","Density","Primitive","Joint","Limit,Twist,Bend,Pull,Push,Shift")
					
					for i in layout_def do
					(
						column = lv.ColumnHeaders.add() 
						column.text = I
						column.width = 1500
						if (column.text = I) == "Limit,Twist,Bend,Pull,Push,Shift" then
						(
							column.width = 4300
						)
					) 
				) 
				
				fn fillInSpreadSheet lv =
				(
					for o in selection do
					(
						li = lv.ListItems.add()
						li.text = o.name
						sub_li = li.ListSubItems.add()
						sub_li.text = (crytools.getUDP o "mass") as string
						sub_li = li.ListSubItems.add()
						sub_li.text = (crytools.getUDP o "density") as string
						sub_li = li.ListSubItems.add()
						sub_li.text = try((o.mesh.numfaces) as string)catch("--")
						sub_li = li.ListSubItems.add()
						sub_li.text = (o.material) as string
					)
				) 
				
				activeXControl lv_objects "MSComctlLib.ListViewCtrl" width:790 height:190 align:#center 
				on listview_rollout open do 
				(
					initListView lv_objects
					fillInSpreadSheet lv_objects
				) 
			)
			
			try(destroyDialog listview_rollout)catch()
			createDialog listview_rollout 800 200
		)
	)
	
	
	rollout bakeBones "Bake to Bones"
	(
		label instruct "Select verts in the deforming mesh"
		label instruct2 "these will generate bones"
		button bake2bones "Bake Deformation to Bones"
		button blendAllBones "Blend All Bones"
		
		on bake2bones pressed do
		(
			if $selection.count > 0 then
			(
				if selectedVerts == undefined or selectedVerts.count < 1 then
				(
					messagebox "No vertices selected"
					return undefined
				)
				
				for i = 1 to selectedVerts.count do
				(
					box name:("bakeBone_" + (selectedVerts[i] as string)) position:((obj1.modifiers[1].getVertex selectedVerts[i]) + [0,0,-.05]) length:1 width:1 height:1
				)
				
				for i = (animationrange.start as integer) to (animationrange.end as integer) do
				(
					with animate on
					(
						slidertime = i
						for m=1 to selectedVerts.count do
						(
							(execute ("$bakeBone_" + selectedVerts[m] as string)).position = ((obj1.modifiers[1].getVertex selectedVerts[m]) + [0,0,-.05])
						)
					)
				)
			)
			else
				messageBox "Select Node before." title:"Error"
		)
		
		on blendAllBones pressed do
		(
			try
			(
				for obj in selection do
				(
					undo "Blend Weights" on
					(
						modPanel.setCurrentObject obj.modifiers[#Skin]
						for i=1 to (skinOps.getNumberBones obj.skin) do
						(
							skinOps.SelectBone obj.modifiers[#Skin] i
							skinOps.blendSelected obj.modifiers[#Skin]
							print i
						)
					)
				)
			)
			catch()
		)
	)
	
	rollout rltSkinTools "Skinning Tools"(
		group "Average Weights"(
			button btnAverageWeightsExecute "Average Weights" width:110 height:16 align:#left offset:[0,-2] across:2
			button btnAverageWeightsRestore "Undo" enabled:false width:40 height:16 align:#right offset:[0,-2]
		)
		group "Blur Weights"(
			spinner spnBlurWeightsRadius "Radius" width:80 height:16 range:[0,1000,10] type:#float align:#right
			checkbutton ckbtnBlurWeightsOptimize "Optimize" width:100 height:16 align:#left offset:[0,-5] across:2 tooltip:"Click to refresh optimization data"
			checkBox chkBlurWeightsAutoOptimize "Auto" align:#right offset:[0,-5] checked:false tooltip:"Automatically refreshes on object change"
			button btnBlurWeightsExecute "Blur Weights" width:110 height:16 align:#left offset:[0,-5] across:2
			button btnBlurWeightsRestore "Undo" enabled:false width:40 height:16 align:#right offset:[0,-5]  tooltip:"Undo last blur"
		)
		group "Grab Weights from Backside"(
			radioButtons rdoGrabWeightsDirection width:100 height:16 labels:#("X", "Y", "Z", "N") columns:4 default:4 offset:[-8,-2]
			button btnGrabWeightsExecute "Grab from Backside" width:110 height:16 align:#left offset:[0,-5] across:2
			button btnGrabWeightsRestore "Undo" enabled:false width:40 height:16 align:#right offset:[0,-5]
		)
		group "Replace Bone"(
			button btnReplaceBoneExecute "Replace Bone" width:110 height:16 align:#left offset:[0,-2] across:2
			button btnReplaceBoneRestore "Undo" enabled:false width:40 height:16 align:#right offset:[0,-2]
		)
		
		--globals for blur skin weights
		local sMySkin = undefined --skin modifier of currently selected object
		local aSelectedVerts = #() --list of selected vertices
		local aOldVertWeights = #() --2D array storing old weights for all target verts
		local aOldVertBones = #() --2D array storing old bones for all target verts
		local oMyVertsOctreeRoot = undefined --octree root node
		local sMyObject = undefined
		local bBlurOptimize = false --true if optimization is active
		
		fn fnGetSelectedVerts sTempSkin =
		( --returns a list with the selected vertices
			local iVertCount = skinOps.GetNumberVertices sTempSkin
			local aTempArray = #()
			for i = 1 to iVertCount do(
				if skinOps.IsVertexSelected sTempSkin i == 1 do(
					append aTempArray i
				)
			)
			return aTempArray
		)

		fn fnFindBoneIDInWeightList sTempSkin iTempID iTempVert =
		( --returns the index of a certain bone in the weightlist of a certain vertex or 0 if it cant be found
			local iWeightCount = skinOps.GetVertexWeightCount sTempSkin iTempVert
			local iResult = 0
			for i = 1 to iWeightCount do(
				iBoneID = skinOps.GetVertexWeightBoneID sTempSkin iTempVert i
				if iBoneID == iTempID do(iResult = i)
			)
			return iResult
		)
		
		--defined octree node struct and its functions
		struct nVertsOctreeNode
		(
			sParent, --parent node of vertices
			p3Min, --bounding box minimum point
			p3Max, --bounding box maximum point
			aChildNodes = #(undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined), --children unless node is a leaf
			aNodeVerts = undefined, --vertices if node is a leaf
			iMaxVertsPerLeaf = 20, --maximum amount of vertices before subdivision
			
			fn fnIsVertInBoundingBox p3Min p3Max iVert =
			( --returns true if vertex is within the specified bounding box
				local p3VertexPosition = sParent.verts[iVert].pos
				if p3VertexPosition.x >= p3Min.x and p3VertexPosition.y >= p3Min.y and p3VertexPosition.z >= p3Min.z then
				(
					if p3VertexPosition.x <= p3Max.x and p3VertexPosition.y <= p3Max.y and p3VertexPosition.z <= p3Max.z then
						(
							true
						)
					else(false)
				)
				else(false)
			),
			
			fn fnPopulate =
			( --populates the octree with vertices
				if aNodeVerts.count > iMaxVertsPerLeaf then
				(
					local p3ParentSize = (p3Max - p3Min)*.5
					local p3CurMin
					local p3CurMax
					local aNewNodeVerts = #()
					
					for i = 0 to 1 do
					(
						for j = 0 to 1 do
						(
							for k = 0 to 1 do
							(
								p3CurMin = p3Min + [p3ParentSize.x*i, p3ParentSize.y*j, p3ParentSize.z*k]
								p3CurMax = p3Min + p3ParentSize + [p3ParentSize.x*i, p3ParentSize.y*j, p3ParentSize.z*k]
								local l = 1
								local end = aNodeVerts.count
								aNewNodeVerts = #()
								while l <= end do
								( --go through parent node verts and move them to child if inside current boundingbox
									if (fnIsVertInBoundingBox p3CurMin p3CurMax aNodeVerts[l]) then
									( --if current vertex is in current boundingbox then move it to new child node and remove from parent node
										append aNewNodeVerts aNodeVerts[l]
										deleteItem aNodeVerts l
										end -= 1
									)
									else
									( --else go to next vertex in parent node
										l += 1
									)
								)
								if aNewNodeVerts.count != 0 then
								(
									aChildNodes[i*4 + j*2 + k + 1] = nVertsOctreeNode sParent:sParent p3Min:p3CurMin p3Max:p3CurMax aNodeVerts:aNewNodeVerts iMaxVertsPerLeaf:iMaxVertsPerLeaf
									
									aChildNodes[i*4 + j*2 + k + 1].fnPopulate()
								)
							)
						)
					)
					aNodeVerts = undefined
				)
			),
			
			fn fnPrintStatistics iLevel iNodeNumber=
			( --print vertex count per node
				if iLevel == 0 then
				(
					print "### Vertex Octree Statistics ###"
				)
				sIndentation = ""
				for i = 0 to iLevel-1 do
				(
					sIndentation += "  "
				)
				if aNodeVerts == undefined then
				(
					print (sIndentation + "Level " + iLevel as string + "." + iNodeNumber as string + " has children:")
					for i = 1 to aChildNodes.count do
					(
						if aChildNodes[i] != undefined then
						(
							aChildNodes[i].fnPrintStatistics (iLevel+1) i
						)
					)
				)
				else
				(
					print (sIndentation + "Level " + iLevel as string + "." + iNodeNumber as string + " has " + aNodeVerts.count as string + " vertices.")
				)
			),
			
			fn fnGetAllVerts aArray =
			( --returns all vertices of octree
				if aNodeVerts == undefined then
				(
					for i = 1 to aChildNodes.count do
					(
						aChildNodes[i].fnGetAllVerts aArray
					)
				)
				else
				(
					for iVert in aNodeVerts do
					(
						append aArray iVert
					)
				)
			),
			
			fn fnSphereIntersectBox p3Min p3Max p3Center fRadius =
			( --returns true if the box and sphere intersect
 				local fDistanceOnAxis = 0
				local fDistanceSquared = 0
				for i = 1 to 3 do
				(
					local fVal = p3Center[i]
					if fVal < p3Min[i] then
					(
						fDistanceOnAxis = fVal - p3Min[i]
						fDistanceSquared += fDistanceOnAxis*fDistanceOnAxis
					)
					else if fVal > p3Max[i] then
					(
						fDistanceOnAxis = fVal - p3Max[i]
						fDistanceSquared += fDistanceOnAxis*fDistanceOnAxis
					)
				)
				fDistanceSquared <= fRadius*fRadius
			),
			
			fn fnFindVertsInSphere p3Center fRadius aVertArray aMultiplierArray =
			( --returns all vertices in the sphere and their weights according to the distance to the center
				if aNodeVerts == undefined then
				(
					for i = 1 to aChildNodes.count do
					(
						if aChildNodes[i] != undefined then
						(
							if (fnSphereIntersectBox aChildNodes[i].p3Min aChildNodes[i].p3Max p3Center fRadius) then
							(
								aChildNodes[i].fnFindVertsInSphere p3Center fRadius aVertArray aMultiplierArray
							)
						)
					)
				)
				else
				(
					for iVert in aNodeVerts do
					(
						fDist = distance sParent.verts[iVert].pos p3Center
						if fDist < fRadius then
						(
							append aVertArray iVert
							append aMultiplierArray (fRadius - fDist)
						)
					)
				)
			)
		)
		
		fn fnBlurOptimize iMaxNodesPerLeaf =
		(--generates octree and sets optimize flag
			try
			(
				sMyskin = modPanel.getCurrentObject()
				if sMySkin.name != "CrySkin" then
				(
					sMySkin = undefined
				)
			)
			catch
			(
				sMySkin = undefined
			)
			if sMySkin != undefined then
			(
				iOctreeTime = timeStamp()
				local iCurrentFrame = sliderTime
				local iMyRefFrame = sMySkin.ref_frame
				if iMyRefFrame < animationRange.start then
				(
					animationRange = interval iMyRefFrame animationRange.end
				)
				if iMyRefFrame > animationRange.end then
				(
					animationRange = interval animationRange.start iMyRefFrame
				)
				sliderTime = iMyRefFrame
				oMyVertsOctreeRoot = undefined
				local aAllVerts = #()
				for i = 1 to getNUmVerts sMyObject do(
					append aAllVerts i
				)
				aBoundingBox = in coordsys world nodeLocalBoundingBox sMyObject
				oMyVertsOctreeRoot = nVertsOctreeNode sParent:sMyObject p3Min:(aBoundingBox[1]*1.01) p3Max:(aBoundingBox[2]*1.01) aNodeVerts:aAllVerts iMaxVertsPerLeaf:iMaxNodesPerLeaf
				oMyVertsOctreeRoot.fnPopulate()
				bBlurOptimize = true
				ckbtnBlurWeightsOptimize.caption = "Optimization Active"
				ckbtnBlurWeightsOptimize.state = true
				sliderTime = iCurrentFrame
				print ("### Octree Generation took: " + formattedprint ((timeStamp() - iOctreeTime)/1000.0) format:".3f" + " seconds ###")
			)
		)
		
		fn fnBlurUnOptimize =
		(
			oMyVertsOctreeRoot = undefined
			bBlurOptimize = false
			ckbtnBlurWeightsOptimize.caption = "Optimize"
			ckbtnBlurWeightsOptimize.state = false
		)
		
		fn fnUpdateSelection =
		(
			sMyObject = $
			try
			(
				sMyskin = modPanel.getCurrentObject()
				if sMySkin.name != "CrySkin" then
				(
					sMySkin = undefined
				)
			)
			catch
			(
				sMySkin = undefined
			)
			if chkBlurWeightsAutoOptimize.state == true and sMyObject != undefined and sMyskin != undefined then
			(
				fnBlurOptimize 64
			)
			else
			(
				fnBlurUnOptimize()
			)
		)
		
		on rltSkinTools open do
		(
			if rltSkinTools.open then
			(
				fnUpdateSelection()
				callbacks.addScript #selectionSetChanged "CryRiggingTools.rollouts[8].fnUpdateSelection()" id:#updateSelection
				callbacks.addScript #modPanelObjPostChange "CryRiggingTools.rollouts[8].fnUpdateSelection()" id:#updateSelection
			)
		)
		
		on rltSkinTools rolledUp state do
		(
			if state then
			(
				fnUpdateSelection()
				callbacks.addScript #selectionSetChanged "CryRiggingTools.rollouts[8].fnUpdateSelection()" id:#updateSelection
				callbacks.addScript #modPanelObjPostChange "CryRiggingTools.rollouts[8].fnUpdateSelection()" id:#updateSelection
			)
			else
			(
				fnBlurUnOptimize()
				callbacks.removeScripts id:#updateSelection
			)
		)
		
		on rltSkinTools close do
		(
			fnBlurUnOptimize()
			callbacks.removeScripts id:#updateSelection
		)
		
		on ckbtnBlurWeightsOptimize changed state do
		(
			if state and sMyObject != undefined then
			(
				fnBlurOptimize 64
			)
			else
			(
				fnBlurUnOptimize()
			)
		)
		
		on btnAverageWeightsExecute pressed do
		(
			sMySkin = undefined
			if $ != undefined do(
				sMyskin = modPanel.getCurrentObject()
				if sMySkin.name != "CrySkin" do(
					sMySkin = undefined
				)
			)
			if sMySkin != undefined then(
				aSelectedVerts = fnGetSelectedVerts sMySkin
				aOldVertWeights = #()
				aOldVertBones = #()
				btnBlurWeightsRestore.enabled = false
				btnAverageWeightsRestore.enabled = false
				for iVert in aSelectedVerts do( --stores old weights for undo function
					aOldWeights = #()
					aOldBones = #()
					for i = 1 to skinOps.GetVertexWeightCount sMySkin iVert do(
						append aOldBones (skinOps.GetVertexWeightBoneID sMySkin iVert i)
						append aOldWeights (skinOps.GetVertexWeight sMySkin iVert i)
					)
					append aOldVertBones aOldBones
					append aOldVertWeights aOldWeights
				)
				aBoneIDList = #() --contains a list of all the bones that affect at least one of the selected vertices
				for iVert in aSelectedVerts do( --unnormalize vertices and create aBoneIDList
					skinOps.unNormalizeVertex sMySkin iVert true
					iWeightCount = skinOps.GetVertexWeightCount sMySkin iVert
					for i = 1 to iWeightCount do(
						iBoneID = skinOps.GetVertexWeightBoneID sMySkin iVert i
						if (findItem aBoneIDList iBoneID) == 0 do(
							append aBoneIDList iBoneID
						)
					)
				)
				for i = 1 to aBoneIDList.count do(
					local fSum = 0
					for iVert in aSelectedVerts do(
						aCurBoneInWeightList =  fnFindBoneIDInWeightList sMySkin aBoneIDList[i] iVert
						if aCurBoneInWeightList != 0 do(
							fSum += (skinOps.GetVertexWeight sMySkin iVert aCurBoneInWeightList)
						)
					)
					for iVert in aSelectedVerts do(
						skinOps.SetVertexWeights sMySkin iVert aBoneIDList[i] (fSum/aSelectedVerts.count)
					)
				)
				for iVert in aSelectedVerts do(
					skinOps.unNormalizeVertex sMySkin iVert false
				)
				btnAverageWeightsRestore.enabled = true
			)
			else(messageBox "Please select the CrySkin modifier of an object!")
		)
		
		on btnAverageWeightsRestore pressed do
		(
			for i = 1 to aSelectedVerts.count do
			(
				skinOps.unNormalizeVertex sMySkin aSelectedVerts[i] true
				skinOps.ReplaceVertexWeights sMySkin aSelectedVerts[i] aOldVertBones[i] aOldVertWeights[i]
				skinOps.unNormalizeVertex sMySkin aSelectedVerts[i] false
			)
			aSelectedVerts = #()
			aOldVertWeights = #()
			aOldVertBones = #()
			btnAverageWeightsRestore.enabled = false
		)
		
		on btnBlurWeightsExecute pressed do
		(
			sMySkin = undefined
			if $ != undefined then
			(
				try
				(
					sMyskin = modPanel.getCurrentObject()
					if sMySkin.name != "CrySkin" then
					(
						sMySkin = undefined
					)
				)
				catch
				(
					sMySkin = undefined
				)
			)
			if sMySkin != undefined then
			(
				iBlurTime = timeStamp()
				aSelectedVerts = fnGetSelectedVerts sMySkin
				
				if aSelectedVerts.count > 0 then
				(--if vertex selection is not empty proceed
					iFindVertsTimeSum = 0
					iGetWeightsTimeSum = 0
					iCurrentFrame = sliderTime
					aOldVertWeights = #() --2D array storing old weights for undo function
					aOldVertBones = #() --2D array storing old bones for undo function
					fBlurRadius = spnBlurWeightsRadius.value --blur radius
					aNewVertWeights = #() --2D array storing new weights for all target verts
					aNewVertBones = #() --2D array storing new bones for all target verts
					btnBlurWeightsRestore.enabled = false
					btnAverageWeightsRestore.enabled = false
					for iVert in aSelectedVerts do
					( --stores old weights for undo function
						aOldWeights = #()
						aOldBones = #()
						for i = 1 to skinOps.GetVertexWeightCount sMySkin iVert do
						(
							append aOldBones (skinOps.GetVertexWeightBoneID sMySkin iVert i)
							append aOldWeights (skinOps.GetVertexWeight sMySkin iVert i)
						)
						append aOldVertBones aOldBones
						append aOldVertWeights aOldWeights
					)
					
					if bBlurOptimize then
					(--if optimized then blur in base pose
						local iMyRefFrame = sMySkin.ref_frame
						if iMyRefFrame < animationRange.start then
						(
							animationRange = interval iMyRefFrame animationRange.end
						)
						if iMyRefFrame > animationRange.end then
						(
							animationRange = interval animationRange.start iMyRefFrame
						)
						sliderTime = iMyRefFrame
					)
					
					for iTargetVert in aSelectedVerts do
					( --unnormalize vertices and creates aNewVertWeights and aNewVertBones
						p3TargetPos = sMyObject.verts[iTargetVert].pos --position of target vertex
						aSourceVerts = #() --list of vertices within a certain proximity to selected vertex
						aSourceVertMultipliers = #() --list of multiplier for each source vertex based on proximity to target
						fSourceVertMultiplierSum = 0
						if bBlurOptimize then
						( --if optimization active then go find vertices in octree
							iFindVertsTime = timeStamp()
							oMyVertsOctreeRoot.fnFindVertsInSphere p3TargetPos fBlurRadius aSourceVerts aSourceVertMultipliers
							for fElement in aSourceVertMultipliers do
							(
								fSourceVertMultiplierSum+= fElement
							)
							iFindVertsTimeSum += timeStamp() - iFindVertsTime
						)
						else
						( --if not optimized go through all vertices in mesh
							iFindVertsTime = timeStamp()
							for iVert = 1 to (getNumVerts sMyObject) do( --creates aSourceVerts array
								fCurDist = distance p3TargetPos (sMyObject.verts[iVert].pos)
								if fCurDist < fBlurRadius then(
									append aSourceVerts iVert
									append aSourceVertMultipliers (fBlurRadius - fCurDist)
									fSourceVertMultiplierSum += (fBlurRadius - fCurDist)
								)
							)
							iFindVertsTimeSum += timeStamp() - iFindVertsTime
						)
						
						fSourceVertMultiplierSum = 1/fSourceVertMultiplierSum
						for i = 1 to aSourceVertMultipliers.count do
						( --normalizes aSourceVertMultipliers
							aSourceVertMultipliers[i] *= fSourceVertMultiplierSum
						)
						
						iGetWeightsTime = timeStamp()
						aNewBones = #() --new bones that target vertex will be skinned to
						aNewWeights = #() -- new weights for these bones
						for j = 1 to aSourceVerts.count do
						( --per vertex creates element for aNewVertWeights and aNewVertBones
							iWeightCount = skinOps.GetVertexWeightCount sMySkin aSourceVerts[j]
							for i = 1 to iWeightCount do
							(
								iBoneID = skinOps.GetVertexWeightBoneID sMySkin aSourceVerts[j] i
								--if bone not in list: append bone and weight, else add weight of bone to existing weights
								if (appendIfUnique aNewBones iBoneID) == true then
								(
									append aNewWeights ((skinOps.GetVertexWeight sMySkin aSourceVerts[j] i)*aSourceVertMultipliers[j])
								)
								else
								(
									aNewWeights[findItem aNewBones iBoneID] += (skinOps.GetVertexWeight sMySkin aSourceVerts[j] i)*aSourceVertMultipliers[j]
								)
							)
						)
						append aNewVertWeights aNewWeights
						append aNewVertBones aNewBones
						iGetWeightsTimeSum += timeStamp() - iGetWeightsTime
					)
					for i = 1 to aSelectedVerts.count do(
						/*
						--prints all the new weights
						sMyPrint = "vertID: " + aSelectedVerts[i] as string + " - "
						fSum = 0
						for j = 1 to aNewVertWeights[i].count do(
							sMyPrint += aNewVertBones[i][j] as string + "= " + aNewVertWeights[i][j] as string + "; "
							fSum += aNewVertWeights[i][j]
						)
						sMyPrint += "Sum= " + fSum as string
						print sMyPrint
						*/
						skinOps.unNormalizeVertex sMySkin aSelectedVerts[i] true
						skinOps.ReplaceVertexWeights sMySkin aSelectedVerts[i] aNewVertBones[i] aNewVertWeights[i]
						skinOps.unNormalizeVertex sMySkin aSelectedVerts[i] false
					)
					if bBlurOptimize then
					(
						sliderTime = iCurrentFrame
					)
					print ("### Blurtimes - Find Verts: " + formattedPrint (iFindVertsTimeSum/1000.0) format:".3f" + " - Get Weights: " + formattedPrint (iGetWeightsTimeSum/1000.0) format:".3f"  + " - Total: " + formattedPrint ((timeStamp() - iBlurTime)/1000.0) format:".3f" + " ###")
					btnBlurWeightsRestore.enabled = true
				)
			)
			else(messageBox "Please select the CrySkin modifier of an editable mesh!")
		)
		
		on btnBlurWeightsRestore pressed do(
			for i = 1 to aSelectedVerts.count do(
				skinOps.unNormalizeVertex sMySkin aSelectedVerts[i] true
				skinOps.ReplaceVertexWeights sMySkin aSelectedVerts[i] aOldVertBones[i] aOldVertWeights[i]
				skinOps.unNormalizeVertex sMySkin aSelectedVerts[i] false
			)
			aSelectedVerts = #()
			aOldVertWeights = #()
			aOldVertBones = #()
			btnBlurWeightsRestore.enabled = false
		)
	)
	
	
	if crytools.DOMAIN == "INTERN.CRYTEK.DE" then
	(
		addRollout InternalTools CryRiggingTools rolledup:true
	) 
	addRollout RiggingTools CryRiggingTools rolledup:true
	addRollout PhysicsSetup  CryRiggingTools rolledup:true
	addRollout Diagnostic CryRiggingTools rolledup:true
	addRollout SmartObjects CryRiggingTools rolledup:true
	--addRollout metadataManager CryRiggingTools rolledup:true
	addRollout reactorTools CryRiggingTools rolledup:true
	addRollout bakebones CryRiggingTools rolledup:true
	addRollout rltSkinTools CryRiggingTools --rolledup:true
) --end local scope

