-------------------------------------------------------------------------------
--	CryAnimationTools.ms
--	Version 2.2 External
--	by Christopher Evans
-------------------------------------------------------------------------------

global CryAnimationTools
(
	if CryAnimationTools != undefined do ( closerolloutfloater CryAnimationTools )
	CryAnimationTools = newrolloutfloater "CryAnimationTools" 190 800 --450
	
	-------------------------------------------------------------------------------
	--	GeneralTools
	-------------------------------------------------------------------------------
	rollout generalTools "Baking Animation"
	(
		group "CGA Prep"
		(
			button makeTCB "Change Rotation to TCB"
			button makeTCBpos "Change Position to TCB"
		)
		button bakeMotion "Bake Motion to Keys"
		group "Bake Rigid Objects to Character"
		(
			edittext simStart_txt "Start" fieldWidth:30
			button bakeObjectsToCHR "Bake Objects" offset:[27,-23]
			progressbar bakeToChr_prog color:red
		)
		
		on generalTools open do
		(
			simStart_txt.text = animationrange.start.frame as string
		)
		
		on makeTCB pressed do
		(
			for obj in selection do
			(
				obj.rotation.controller = tcb_rotation ()
			)
		)
		
		on makeTCBpos pressed do
		(
			for obj in selection do
			(
				obj.position.controller = tcb_position ()
			)
		)
			
		on bakeMotion pressed do
		(
			undo "bakeMotion" on
			(
				for t = animationrange.start to animationrange.end do
				(
					for obj in selection do
					(
						sliderTime = t
						max set key on selected
					)
				)
			)
		)
		
		on bakeObjectsToCHR pressed do
		(
			try
			(
				undo "bake to chr" on
				(
					--with redraw off
					--(
						animationRange = interval -1f animationRange.end
						slidertime = -1
						i = 0
						macros.run "Modifier Stack" "Convert_to_Mesh"
						meshObjs = selection as array
						proxyObjs = #()
						proxyObj = ""
						offset = 0
						simStart = (simStart_txt.text as float)
						origPos = 0
						origRot = 0
						proxyObj = 0
						
						maxops.cloneNodes selection cloneType:#copy newnodes:&copies
			
						for obj in selection do
						(
							--crytools.objTrajectoryToSpline()
							newPivot = [0,0,0]
							with animate on
							(
								origPos = obj.pos
								obj.pos = [0,0,0]
								origRot = obj.rotation
			
								obj.rotation = quat 0 0 0 1
			
								bbox = (obj.max - obj.min)
								newPivot = (obj.pivot - obj.min)
								box length:bbox.y width:bbox.x height:bbox.z name:(obj.name + "_bprox")
								offset += (20*(obj.max - obj.min).y)
							)
							proxyObj = (getnodebyname (obj.name + "_bprox"))
							proxyObj.pivot = proxyObj.min + newPivot
							proxyObj.pos += (obj.pivot - proxyObj.pivot)
							proxyobj.xray = true
							
							proxyObj.pos.controller = copy obj.pos.controller
							proxyObj.rotation.controller = copy obj.rotation.controller
							
							obj.pos.y = offset
							--(getnodebyname (obj.name + "_bprox")).pos.x = ((i*200))
			
							append proxyObjs proxyObj
							
							proxyObj.pos.controller = tcb_position ()
							
							with animate on
							(
								proxyObj.rotation = obj.rotation 
								proxyObj.pos = obj.pos
								
								slidertime = 0
								proxyObj.rotation = origRot
								proxyObj.pos = origPos
							)
							slidertime = -1
							
							deleteKeys obj #allkeys
							
							i+=1
							bakeToChr_prog.value += (100.0/selection.count)
						)
						
						--collapse all objs into one mesh
						for obj in meshObjs do
						(
							if obj != meshObjs[1] then
							(
								attach meshObjs[1] obj
							)
						)
						meshObjs[1].name = "ExportCHR"
						
						--apply skin modifier to object
						select $ExportCHR
						slidertime = 0
						modPanel.addModToSelection (Skin ()) ui:on
						$ExportCHR.modifiers[#Skin].ref_frame = -1
						slidertime = -1
						
						if $root == undefined then
						(
							dummy name:"root" pos:[0,0,0] boxsize:[1,1,1]
						)
						
						
						for obj in proxyObjs do
						(
							skinOps.addBone $ExportCHR.modifiers[#Skin] obj 1
						)
						
						bakeToChr_prog.value = 0
						bakeToChr_prog.color = blue
						with redraw off
						(
							with animate on
							(
								for n = 1 to proxyObjs.count do
								(
									for i = 1 to proxyObjs[n].pos.controller.keys.count do
									(
										slidertime = proxyObjs[n].pos.controller.keys[i].time
										proxyObjs[n].transform = copies[n].transform
									)
								bakeToChr_prog.value += (100.0/proxyObjs.count)
								)
							)
						)
						
						for obj in copies do
						(
							delete obj
						)
						
						
						
						slidertime = simStart
						$root.pos = $ExportCHR.center
						animationrange = (interval simStart animationrange.end)
						
						for obj in proxyObjs do
						(
							obj.parent = $root
						)
						bakeToChr_prog.value = 0
						bakeToChr_prog.color = red
					--)
				)
			)catch()
		)
	)
	
	-------------------------------------------------------------------------------
	--	CryPlantKey
	-------------------------------------------------------------------------------
	rollout plantKey "CryPlantKey 1.4"
	(
	dropdownlist key_mode "Key Type:" items:#("Planted","Sliding", "Free")
	checkbox existingOnly "Only for existing keys"
	button begin_btn "Begin" offset:[-55,0]
	button end_btn "End  " offset:[-55,0]
	editText begin_txt fieldWidth:30 offset:[43,-50]
	editText end_txt fieldWidth:30 offset:[43,3]
	button specific "Use Begin/End" align:#center offset:[0,5]
	button range "Use Animation Range" align:#center
	
	on begin_btn pressed do
		(
		escapeenable = true
		begin_txt.text = (((currentTime as integer)/TicksPerFrame) as string)
		)
	
	on end_btn pressed do
		(
		end_txt.text = (((currentTime as integer)/TicksPerFrame) as string)
		)
	
	on range pressed do
		(
		
		undo "CryPlantKey" on
		(
			if existingOnly.checked == false then
			(
				if selection.count > 0 then
				(
					for t = animationrange.start to animationrange.end do
					(	
						sliderTime = t
									
						for i = 1 to selection.count do
						(
							try
							(
								case key_mode.selection of
								(
									1: biped.setPlantedKey selection[i]
									2: biped.setSlidingKey selection[i]
									3: biped.setFreeKey selection[i]
								)
							)
							catch()
							
							try
							(
								case key_mode.selection of
								(
									1: biped.setPlantedKey selection[i]
									2: biped.setSlidingKey selection[i]
									3: biped.setFreeKey selection[i]
								)
							)	
							catch()	 
						)
					)
				)
			)
		)
		
		if existingOnly.checked == true then
		(
			if selection.count > 0 then
			(	
				undo "cryPlantKey" on
				(
					for obj in selection do
					(
						for i = 1 to obj.transform.controller.keys.count do
						(
							sliderTime = obj.transform.controller.keys[i].time
							case key_mode.selection of								
							(
								1: biped.setPlantedKey obj
								2: biped.setSlidingKey obj
								3: biped.setFreeKey obj
							)
						)
					)
				)
			)
		)
	)
	
	on specific pressed do
	(
		try
		(
			start = begin_txt.text as float
			stop = end_txt.text as float
			
			old_start = animationrange.start
			old_stop = animationrange.end
			animationrange = interval start stop
		
			if selection.count > 0 then
			(	
				if existingOnly.checked != true then
				(
					undo "cryPlantKey" on
					(
						for t = animationrange.start to animationrange.end do
						(
								
							sliderTime = t
										
							for i = 1 to selection.count do
							(
								try
								(
								case key_mode.selection of
								(
									1: biped.setPlantedKey selection[i]
									2: biped.setSlidingKey selection[i]
									3: biped.setFreeKey selection[i]
								)
								)
								catch()
								
								try
								(
									case key_mode.selection of
									(
										1: biped.setPlantedKey selection[i]
										2: biped.setSlidingKey selection[i]
										3: biped.setFreeKey selection[i]
									)
								)	
								catch()	 
							)
						)
					)
				)
			)
			if existingOnly.checked == true then
			(
				if selection.count > 0 then
				(
					undo "cryPlantKey" on
					(
						for obj in selection do
						(
							for i = 1 to obj.transform.controller.keys.count do
							(
								sliderTime = obj.transform.controller.keys[i].time
								case key_mode.selection of								
								(
									1: biped.setPlantedKey obj
									2: biped.setSlidingKey obj
									3: biped.setFreeKey obj
								)
							)
						)
					)
				)
			)
			animationrange = interval old_start old_stop
		)catch()
	)
	
	--	on plantkey open
			on plantkey open do
			(
			--get animation tools location
				local tempVar = cryTools.inFromINI "CryTools" "anim_pos"
				if tempVar == "" then tempVar = [0,77]
				else
				(
					try
					(
						tempVar = execute tempVar
						cryanimationtools.pos = tempVar
					)
					catch
					(
						cryTools.outToINI "CryTools" "anim_pos" "[0,77]"
						cryanimationtools.pos = [0,77]
					)
				)
			)
	
	-- on plantkey closed
			on plantkey close do
			(
				cryTools.outToINI "CryTools" "anim_pos" (cryanimationtools.pos as String)
			)
	)
	
	
	-------------------------------------------------------------------------------
	--	BipedTools
	-------------------------------------------------------------------------------
	rollout bipedtools "BipedTools 1.5"
	(
		dropdownlist bipSelect "" items:#("") width:130 offset:[-5,0]
		button refreshBipTools "<" offset:[65,-27] tooltip:"Refresh"
		button bipmotion "Bip Motion Menu" offset:[-35,0]
		button LoadBIP "Load BIP" offset:[47,-26]
		checkbutton figuremode "Figure Mode" offset:[-39,0]
		checkbutton hidebiped "Hide Biped" offset:[39,-26]
		checkbutton inplace "In Place Mode" offset:[-30,0]
		checkbutton inplacex "X" offset:[29,-26]
		checkbutton inplacey "Y" offset:[57,-26]
		button selectAllBip "Select Only Biped Bones"
		button clampTimeline "Clamp Timeline->" tooltip:"Clamp timeline at current key" offset:[-35,0]
		button clampTimelineKey "At Last Key" tooltip:"Clamp timeline at last keyframe" offset:[50,-26]
		button reverseAnim "Reverse Animation to FBX Skel"
		spinner start "Start " range:[-10000,10000,0] type:#integer scale:1 fieldWidth:40 offset:[-80,0]
		spinner end "End "  range:[-10000,10000,100] type:#integer scale:1 fieldWidth:40 offset:[0,-20]
		button FBXBIP "Attach Biped to FBX Skeleton"
		button deleteFBX "       Delete FBX Skeleton       "
		
		group "Collections"
		(
			button loadCollections "Load Pose Collections" enabled:false
			checkbox getLatest "Latest (P4)" enabled:false
			checkbox overwriteCollections "Overwrite" offset:[80,-20] enabled:false
		)
	
	-- on bipselect selected do
		on bipSelect selected s do
		(
			print (bipSelect.items[s] + " selected.")
			selectedBip = ("$'" + bips[s].name + "'")
			global biped_ctrl = (execute (selectedBip + ".controller"))
		)
	
	--on refreshbiptools pressed
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
				
				start.value = animationrange.start
				end.value = animationrange.end
			)catch()
		)
	
	--	on biped tools open
			on bipedtools open do
			(
			if $bip01 != undefined then
				(
				bip_layer = LayerManager.getLayerFromName "Bip"
				
				if bip_layer != undefined then
				(
				if bip_layer.ishidden == true then
						(
						hideBiped.checked = true
						)
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
				)
				
				overwritecollections.checked = true
				getlatest.checked = true
				
				--callbacks
				figureSet = "if $bip01.controller.figureMode == true then (CryAnimationTools.rollouts[2].figureMode.checked = true) else (CryAnimationTools.rollouts[2].figureMode.checked = false)"
				callbacks.addscript #filePostOpen figureSet id:#figureMode
				
				BIPhideSet = "bip_layer = LayerManager.getLayerFromName \"Bip\" ; if bip_layer.ishidden == true then (CryAnimationTools.rollouts[2].hideBiped.checked = true) else (CryAnimationTools.rollouts[2].hideBiped.checked = false)"
				callbacks.addscript #filePostOpen BIPhideSet id:#bipHide
				
				--get bips
				if $Bip01 != undefined then
				(
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
				
				start.value = animationrange.start
				end.value = animationrange.end
			)
	
	--	load bip
			on LoadBIP pressed do
			(
				LoadBIP_file = getOpenFileName types:"BIP(*.bip)"
				if LoadBIP_file == undefined then
				(
					return undefined
				)
				animationrange = (interval 0f 1f)
				biped_ctrl= (execute (selectedBip + ".controller"))
				biped.loadBipFile biped_ctrl LoadBIP_file
			)
	
	--	bip motion menu button
			on bipMotion pressed do
			(
			try
			(
				select (execute (selectedBip))
				max motion mode
			)
			catch
				( 
					messageBox "Please load a biped." title:"Error"
					return undefined
				)
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
					return undefined
					)
				)
				
			-- in place mode button
			on inPlace changed state do
				(
				try
					if (inPlace.checked == true) then 
					(
					biped_ctrl=(execute (selectedBip + ".controller"))
					biped_ctrl.inPlaceMode = true
					)
					else
					(
					biped_ctrl=(execute (selectedBip + ".controller"))
					biped_ctrl.inPlaceMode = false
					biped_ctrl.inplacexmode = false
					biped_ctrl.inplaceymode = false
					inplacex.checked = false
					inplacey.checked = false
					)
					catch
					( 
					messageBox "Please load  a biped, or take the biped out of figure mode." title:"Error"
					return undefined
					)
				)
			
			on inplacex changed state do
			(
				try
				(
					if (inPlacex.checked == true) then 
					(
						if inplace.checked == false then
						(
							inplace.checked = true
						)
						if inplacey.checked == true then
						(
							inplacey.checked = false
						)
						biped_ctrl=(execute (selectedBip + ".controller"))
						biped_ctrl.inPlacexMode = true
					)
					else
					(
						biped_ctrl=(execute (selectedBip + ".controller"))
						biped_ctrl.inPlacexMode = false
						if inplace.checked == true then
						(
							biped_ctrl=(execute (selectedBip + ".controller"))
							biped_ctrl.inPlaceMode = true
						)
					)
				)catch()
			)
				
			on inplacey changed state do
			(
				try
				(
					if (inPlacey.checked == true) then 
					(
						if inplace.checked == false then
						(
							inplace.checked = true
						)
						if inplacex.checked == true then
						(
							inplacex.checked = false
						)
						biped_ctrl=(execute (selectedBip + ".controller"))
						biped_ctrl.inPlaceyMode = true
					)
					else
					(
						biped_ctrl=(execute (selectedBip + ".controller"))
						biped_ctrl.inPlaceyMode = false
						if inplace.checked == true then
						(
							biped_ctrl=(execute (selectedBip + ".controller"))
							biped_ctrl.inPlaceMode = true
						)
					)
				)catch()
			)
			
		-- select all biped bones
		
		on selectAllBIP pressed do
		(
			try
			(
				if selectedBip != "$'Bip01'" then
				(
					messagebox "sorry, only works with bip01 for now!"
					return undefined
				)
				select $Bip01
				selectmore $'Bip01 Pelvis'
				selectmore $'Bip01 L Thigh'
				selectmore $'Bip01 L Calf'
				selectmore $'Bip01 L Foot'
				selectmore $'Bip01 L Toe0'
				selectmore $_Bip01LToeHelper
				selectmore $'Bip01 R Thigh'
				selectmore $'Bip01 R Calf'
				selectmore $'Bip01 R Foot'
				selectmore $'Bip01 R Toe0'
				selectmore $_Bip01RToeHelper
				selectmore $'Bip01 Spine'
				selectmore $'Bip01 Spine1'
				selectmore $'Bip01 Spine2'
				selectmore $'Bip01 Spine3'
				selectmore $'Bip01 Neck'
				selectmore $'Bip01 Head'
				selectmore $'Bip01 L Clavicle'
				selectmore $'Bip01 L UpperArm'
				selectmore $'Bip01 L Forearm'
				selectmore $'Bip01 L Hand'
				selectmore $'Bip01 L Finger0'
				selectmore $'Bip01 L Finger01'
				selectmore $'Bip01 L Finger02'
				selectmore $'_Bip01 L Finger0Nub'
				selectmore $'Bip01 L Finger1'
				selectmore $'Bip01 L Finger11'
				selectmore $'Bip01 L Finger12'
				selectmore $'_Bip01 L Finger1Nub'
				selectmore $'Bip01 L Finger2'
				selectmore $'Bip01 L Finger21'
				selectmore $'Bip01 L Finger22'
				selectmore $'_Bip01 L Finger2Nub'
				selectmore $'Bip01 L Finger3'
				selectmore $'Bip01 L Finger31'
				selectmore $'Bip01 L Finger32'
				selectmore $'_Bip01 L Finger3Nub'
				selectmore $'Bip01 L Finger4'
				selectmore $'Bip01 L Finger41'
				selectmore $'Bip01 L Finger42'
				selectmore $'_Bip01 L Finger4Nub'
				selectmore $'Bip01 L ForeTwist'
				selectmore $'Bip01 L ForeTwist1'
				selectmore $'Bip01 L ForeTwist2'
				selectmore $'Bip01 R Clavicle'
				selectmore $'Bip01 R UpperArm'
				selectmore $'Bip01 R Forearm'
				selectmore $'Bip01 R Hand'
				selectmore $'Bip01 R Finger0'
				selectmore $'Bip01 R Finger01'
				selectmore $'Bip01 R Finger02'
				selectmore $'_Bip01 R Finger0Nub'
				selectmore $'Bip01 R Finger1'
				selectmore $'Bip01 R Finger11'
				selectmore $'Bip01 R Finger12'
				selectmore $'_Bip01 R Finger1Nub'
				selectmore $'Bip01 R Finger2'
				selectmore $'Bip01 R Finger21'
				selectmore $'Bip01 R Finger22'
				selectmore $'_Bip01 R Finger2Nub'
				selectmore $'Bip01 R Finger3'
				selectmore $'Bip01 R Finger31'
				selectmore $'Bip01 R Finger32'
				selectmore $'_Bip01 R Finger3Nub'
				selectmore $'Bip01 R Finger4'
				selectmore $'Bip01 R Finger41'
				selectmore $'Bip01 R Finger42'
				selectmore $'_Bip01 R Finger4Nub'
				selectmore $'Bip01 R ForeTwist'
				selectmore $'Bip01 R ForeTwist1'
				selectmore $'Bip01 R ForeTwist2'
				selectmore $'_Bip01 HeadNub'
			)catch()
		)
			
		-- on clampTimeline pressed
		on clampTimeline pressed do
		(
			try ( animationrange = interval animationrange.start slidertime ) catch()
		)
	
		-- clampTimelineKey 
		on clampTimelineKey  pressed do
		(
			if $ != undefined then
			(
				try
				(
					animationrange = interval animationrange.start $.controller.keys[$.controller.keys.count].time
				)
				catch
				(
					messagebox "Max returned a -1 value and Biped does not support this.\nPlease select another object."
				)
			)
			else
			(
				messagebox "select an animated object"
			)
		)
			
			
		--	FBX to BIP
		on FBXBIP pressed do
		(
			filein (crytools.BuildPathFull + "Tools\\maxscript\\FBX_Bones_to_Bip.ms")
		)
		
		on loadCollections pressed do
		(	
			if getLatest.checked == true then
			(
			p4Update = ("p4 sync J:\\" + crytools.project_name + "_Production\\Art\\Animation\\Human\\Resources\\poses\\...")
			crytools.scmd p4Update true
			)
			try
			(
				if $bip01 != undefined then
				(
					biped_ctrl = (execute (selectedBip + ".controller"))
					if overwritecollections.checked == true then
					(
						biped.deleteallcopycollections biped_ctrl
					)
					biped.loadCopyPasteFile biped_ctrl "J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male.cpy"
					biped.loadCopyPasteFile biped_ctrl "J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_female.cpy"
					biped.loadCopyPasteFile biped_ctrl "J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male_combat.cpy"
					biped.loadCopyPasteFile biped_ctrl "J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male_crouch.cpy"
					biped.loadCopyPasteFile biped_ctrl "J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male_prone.cpy"
					biped.loadCopyPasteFile biped_ctrl "J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male_relaxed.cpy"
					biped.loadCopyPasteFile biped_ctrl "J:/Game02_Production/Art/Animation/Human/Resources/poses/crysis_male_stealth.cpy"
				)
			)
			catch
			(
				messagebox "Cannot locate pose files"
			)
		)
		
		on reverseAnim pressed do
		(
			try
			(
				original = #()
				newBones =#()
				disablesceneredraw()
				select (execute (selectedBip))
				biped_ctrl = $.controller
				if biped_ctrl.figureMode == true then
				(
					messagebox "Please leave FigureMode"
				)
				
				select (execute (selectedBip + "...*"))
				selectionSets ["original"] = selection
				
				for i in selection do
				(
					append original i
					b = snapshot i name:("_" + (i.name as string))
					b.parent = undefined
					b.transform = i.transform
					b.position.track = bezier_position()
					b.rotation.track = euler_xyz()
					append newBones b
				)
				selectionSets ["newBones"] = newBones
				
				for i in 1 to newBones.count do
				(
					 newBones[i].position.track = bezier_position()
					 newBones[i].rotation.track = euler_xyz()
				)
				animate on
				undo "match animation" on
				(
					for t in start.value to end.value do at time t
					(
						for i in 1 to newBones.count do
						(
							newBones[i].transform = original[i].transform
							--reverseTime newBones[i].transform.controller start.value end.value #incLeft #incRight
							--scaleTime newBones[i].transform.controller start.value end.value -1
							--movekeys newBones[i].transform.controller ((animationrange.end) - (animationrange.start))
							--newBones[i].transform.controller.keys.time += ((start.value) - (end.value))
						)
					)
					for i in 1 to newBones.count do
					(
						scaleTime newBones[i].transform.controller start.value end.value -1
						movekeys newBones[i].transform.controller ((animationrange.end) - (animationrange.start))
					)
				)
				
				/*undo "reparent" on
				(
					for i in 1 to newBones.count do
					(
						try
						(
							newBones[i].parent = execute ("$'_" + original[i].parent.name + "'")
						)
						catch()
					)
				)*/
				enablesceneredraw()
				redrawviews()
			)catch()
		)
					
		on deleteFBX pressed do
		(
			try
			(
				undo "remove FBX skel" on
				(
					for i in 1 to selectionSets[#newbones].count do
					(
						selectmore selectionSets[#newbones][i]
						--max delete
					)
					max delete
				)
			)catch()
		)
		
		on bipedTools close do
		(
			callbacks.removescripts #filePostOpen id:#figureMode
			callbacks.removescripts #filePostOpen id:#bipHide
		)
		
	)
	
	-------------------------------------------------------------------------------
	--	Rig Navigator
	-------------------------------------------------------------------------------
	rollout bipSelect "Crysis Rig Navigator"
	(
		activeXControl crysisBipHtml "http://www.crytek.com" width:168 height:430 pos:[2,3]
	
		on bipSelect open do
		(
			crysisBipHtml.navigate (crytools.BuildPathFull + "Tools\\maxscript\\bipSelect\\bipselect.html")
		)
		on crysisBipHtml titlechange txt do
		(
			case txt of
			(
				"Bip01 Head":(  try (select (biped.getNode biped_ctrl 11 link:1)) ; catch() )
				"Bip01 L ForeArm":(  try (select ( biped.getNode biped_ctrl 1 link:3 )) ; catch() )
				"Bip01 R Forearm":  (select ( biped.getNode biped_ctrl  2 link:3)) --; catch() )
				"Bip01 R UpperArm":(  try (select ( biped.getNode biped_ctrl  2 link:2)) ; catch() )
				"Bip01 L Thigh":(  try (select ( biped.getNode biped_ctrl 5)) ; catch() )
				"Bip01 R Thigh":(  try (select ( biped.getNode biped_ctrl 6 )) ; catch() )
				"Bip01 R Knee":(  try (select $'Bip01 R Knee') ; catch() )
				"Bip01 L Knee":(  try (select $'Bip01 L Knee') ; catch() )
				"Bip01":(  try (select (execute(selectedBip))) ; catch() )
				"Bip01 Pelvis":(  try (select ( biped.getNode biped_ctrl 12)) ; catch() )
				"Bip01 L UpperArm":(  try (select ( biped.getNode biped_ctrl  1 link:2 ) ) ; catch() )
				"Bip01 Spine":(  try (select (biped.getNode biped_ctrl 9 link:1)) ; catch() )
				"Bip01 Spine3":(  try (select (biped.getNode biped_ctrl 9 link:4)) ; catch() )
				"Bip01 Spine1":(  try (select (biped.getNode biped_ctrl 9 link:2)) ; catch() )
				"Bip01 R Calf":(  try (select ( biped.getNode biped_ctrl 6 link:2)) ; catch() )
				"Bip01 L Calf":(  try (select ( biped.getNode biped_ctrl 5 link:2)) ; catch() )
				"weapon_bone":(  try (select $'weapon_bone') ; catch() )
				"alt_weapon_bone01":(  try (select $'alt_weapon_bone01') ; catch() )
				"Bip01 Spine2":(  try (select (biped.getNode biped_ctrl  9 link:3)) ; catch() )
				"Bip01 L Foot":(  try (select ( biped.getNode biped_ctrl 5 link:3)) ; catch() )
				"Bip01 R Foot":(  try (select ( biped.getNode biped_ctrl 6 link:3)) ; catch() )
				"Bip01 L Hand":(  try (select ( biped.getNode biped_ctrl 1 link:4 )) ; catch() )
				"Bip01 R Hand":(  try (select ( biped.getNode biped_ctrl 2 link:4)) ; catch() )
				"Bip01 R clavicular deltoid01":(  try (select $'Bip01 R clavicular deltoid01') ; catch() )
				"Bip01 L clavicular deltoid01":(  try (select $'Bip01 L clavicular deltoid01') ; catch() )
			)
		)
	)
	
	-------------------------------------------------------------------------------
	--	RangeView
	-------------------------------------------------------------------------------
	rollout rangeview "RangeView 1.6"
	(
	
	dropdownlist range_sel "Sub Range:" items:#("No tagged ranges found!") height:25
	
		on rangeview open do
		(
			global ntags = FrameTagManager.GetTagCount()
			global tagNameArray = #()
			if ntags == 0 then
			(
			tagNameArray = #("No tagged ranges found!")
			range_sel.enabled = false
			)
			else
			(
				for i = 1 to ntags do
				(
					id = FrameTagManager.GetTagID i
					name_ = FrameTagManager.GetNameByID id
					get_ = filterstring name_ "_"
					if get_[get_.count] == "start" do
					(
						tagName = #(FrameTagManager.GetNameByID id)
						join tagNameArray tagName
					)
				)
			all_add = #("All Frames")
			join tagNameArray all_add
			)
			range_sel.items = tagNameArray
		)
		
		on range_sel selected n do 
		(
			if n == (tagNameArray.count) then
				(
				animationRange = interval (FrameTagManager.GetTimeByID 1) (FrameTagManager.GetTimeByID ntags)
				)
			else
			(
			for i = 1 to ntags do
				(
				if tagNameArray[n] == (FrameTagManager.GetNameByID i) then
					(
					animationRange = interval (FrameTagManager.GetTimeByID i) (FrameTagManager.GetTimeByID (i+1))
					)
				)
			)
		)
	)
	
	
	-------------------------------------------------------------------------------
	--	MirrorArms
	-------------------------------------------------------------------------------
	rollout mirrorarms "MirrorArms 1.4"
	(
		dropdownlist mirrorRL "Mirror Animated Arms:" items:#("Right Arm Anim to Left", "Left Arm Anim to Right") height:25
		checkbox new_root "copy\mirror root anim" pos:[25,50] checked:true
		progressbar mirror_prog color:red
		button mirrorarmsbtn "   Mirror   "
		
		on mirrorarmsbtn pressed do
		(
			try
			(
				if mirrorRL.selection == 2 do
				(
									--	Define left and right arms
					left_arm = #($upperarm_NEW_L, $rootL, $upperarm_L, $forearm_L, $hand_L, $thumb_L01, $thumb_L02, $thumb_L03, $thumb_L04, $indexfinger_L01, $indexfinger_L02, $indexfinger_L03, $indexfinger_L04, $middlefinger_L01, $middlefinger_L02, $middlefinger_L03, $middlefinger_L04, $ringfinger_L01, $ringfinger_L02, $ringfinger_L03, $ringfinger_L04, $pinky_L01, $pinky_L02, $pinky_L03, $pinky_L04)
					right_arm = #($upperarm_NEW_R, $rootR, $upperarm_R, $forearm_R, $hand_R, $thumb_R01, $thumb_R02, $thumb_R03, $thumb_R04, $indexfinger_R01, $indexfinger_R02, $indexfinger_R03, $indexfinger_R04, $middlefinger_R01, $middlefinger_R02, $middlefinger_R03, $middlefinger_R04, $ringfinger_R01, $ringfinger_R02, $ringfinger_R03, $ringfinger_R04, $pinky_R01, $pinky_R02, $pinky_R03, $pinky_R04)
					
					left_arm_only_rot = #($forearm_L, $hand_L, $thumb_L01, $thumb_L02, $thumb_L03, $thumb_L04, $indexfinger_L01, $indexfinger_L02, $indexfinger_L03, $indexfinger_L04, $middlefinger_L01, $middlefinger_L02, $middlefinger_L03, $middlefinger_L04, $ringfinger_L01, $ringfinger_L02, $ringfinger_L03, $ringfinger_L04, $pinky_L01, $pinky_L02, $pinky_L03, $pinky_L04)
					right_arm_only_rot = #($forearm_R, $hand_R, $thumb_R01, $thumb_R02, $thumb_R03, $thumb_R04, $indexfinger_R01, $indexfinger_R02, $indexfinger_R03, $indexfinger_R04, $middlefinger_R01, $middlefinger_R02, $middlefinger_R03, $middlefinger_R04, $ringfinger_R01, $ringfinger_R02, $ringfinger_R03, $ringfinger_R04, $pinky_R01, $pinky_R02, $pinky_R03, $pinky_R04)
					
					if new_root.checked == true then
					(
						dummy name:"new_root" pos:($root.pos) boxsize:[1,1,1]
						$new_root.rotation = $root.rotation
						$new_root.rotation.controller = tcb_rotation ()
						$root.rotation.controller = tcb_rotation ()
						for i = 1 to $root.rotation.controller.keys.count do
						(
							with animate on
							(
								slidertime = $root.rotation.controller.keys[i].time
								$new_root.pos.x = -$root.pos.x
								$new_root.pos.y = $root.pos.y
								$new_root.pos.z = $root.pos.z
								$new_root.dir = ([-1,1,1] * $root.dir)
								rot_90 = eulerangles 0 0 -90
								in coordsys local rotate $new_root rot_90
							)
						)
					)
				
					--	Remove right arm keys, copy over the controller info, and set TCB (quaternion) rotations
					for i=1 to right_arm.count do
						(
						deleteKeys right_arm[i].rotation.controller.keys #allkeys
						deleteKeys right_arm[i].position.controller.keys #allkeys
						deleteKeys right_arm[i].scale.controller.keys #allkeys
						right_arm[i].position.controller = copy right_arm[i].position.controller
						right_arm[i].rotation.controller = copy right_arm[i].rotation.controller
						right_arm[i].rotation.controller = tcb_rotation ()
						left_arm[i].rotation.controller = tcb_rotation ()
						)
					
					--	Mirror rotation data L->R
					for i=1 to right_arm.count do
					(
						for r = 1 to left_arm[i].rotation.controller.keys.count do
						(
						with animate on
							(			
							slidertime = left_arm[i].rotation.controller.keys[r].time
							crytools.MirrorObjs right_arm[i] left_arm[i] $root #x #x
							in coordsys local rotate right_arm[i] (angleaxis 180 [0,1,0])
							)
						--right_arm[i].rotation.controller.keys[r].tension = left_arm[i].rotation.controller.keys[r].tension
						--right_arm[i].rotation.controller.keys[r].continuity = left_arm[i].rotation.controller.keys[r].continuity
						--right_arm[i].rotation.controller.keys[r].bias = left_arm[i].rotation.controller.keys[r].bias
						)
					mirror_prog.value = 100.*i/right_arm.count
				)
			)
	
				if mirrorRL.selection == 1 do
				(
					--	Define left and right arms
					left_arm = #($upperarm_NEW_L, $rootL, $upperarm_L, $forearm_L, $hand_L, $thumb_L01, $thumb_L02, $thumb_L03, $thumb_L04, $indexfinger_L01, $indexfinger_L02, $indexfinger_L03, $indexfinger_L04, $middlefinger_L01, $middlefinger_L02, $middlefinger_L03, $middlefinger_L04, $ringfinger_L01, $ringfinger_L02, $ringfinger_L03, $ringfinger_L04, $pinky_L01, $pinky_L02, $pinky_L03, $pinky_L04)
					right_arm = #($upperarm_NEW_R, $rootR, $upperarm_R, $forearm_R, $hand_R, $thumb_R01, $thumb_R02, $thumb_R03, $thumb_R04, $indexfinger_R01, $indexfinger_R02, $indexfinger_R03, $indexfinger_R04, $middlefinger_R01, $middlefinger_R02, $middlefinger_R03, $middlefinger_R04, $ringfinger_R01, $ringfinger_R02, $ringfinger_R03, $ringfinger_R04, $pinky_R01, $pinky_R02, $pinky_R03, $pinky_R04)
					
					left_arm_only_rot = #($forearm_L, $hand_L, $thumb_L01, $thumb_L02, $thumb_L03, $thumb_L04, $indexfinger_L01, $indexfinger_L02, $indexfinger_L03, $indexfinger_L04, $middlefinger_L01, $middlefinger_L02, $middlefinger_L03, $middlefinger_L04, $ringfinger_L01, $ringfinger_L02, $ringfinger_L03, $ringfinger_L04, $pinky_L01, $pinky_L02, $pinky_L03, $pinky_L04)
					right_arm_only_rot = #($forearm_R, $hand_R, $thumb_R01, $thumb_R02, $thumb_R03, $thumb_R04, $indexfinger_R01, $indexfinger_R02, $indexfinger_R03, $indexfinger_R04, $middlefinger_R01, $middlefinger_R02, $middlefinger_R03, $middlefinger_R04, $ringfinger_R01, $ringfinger_R02, $ringfinger_R03, $ringfinger_R04, $pinky_R01, $pinky_R02, $pinky_R03, $pinky_R04)
					
					if new_root.checked == true then
					(
						dummy name:"new_root" pos:($root.pos) boxsize:[1,1,1]
						$new_root.rotation = $root.rotation
						$new_root.rotation.controller = tcb_rotation ()
						$root.rotation.controller = tcb_rotation ()
						for i = 1 to $root.rotation.controller.keys.count do
						(
							with animate on
							(
								slidertime = $root.rotation.controller.keys[i].time
								$new_root.pos.x = -$root.pos.x
								$new_root.pos.y = $root.pos.y
								$new_root.pos.z = $root.pos.z
								$new_root.dir = ([-1,1,1] * $root.dir)
								rot_90 = eulerangles 0 0 -90
								in coordsys local rotate $new_root rot_90
							)
						)
					)
				
					--	Remove left arm keys, copy over the controller info, and set TCB (quaternion) rotations
					for i=1 to left_arm.count do
						(
						deleteKeys left_arm[i].rotation.controller.keys #allkeys
						deleteKeys left_arm[i].position.controller.keys #allkeys
						deleteKeys left_arm[i].scale.controller.keys #allkeys
						left_arm[i].position.controller = copy right_arm[i].position.controller
						left_arm[i].rotation.controller = copy right_arm[i].rotation.controller
						right_arm[i].rotation.controller = tcb_rotation ()
						left_arm[i].rotation.controller = tcb_rotation ()
						)
					
					--	Mirror rotation data R->L
					for i=1 to left_arm.count do
					(
						for r = 1 to right_arm[i].rotation.controller.keys.count do
						(
						with animate on
							(			
							slidertime = right_arm[i].rotation.controller.keys[r].time
							crytools.MirrorObjs left_arm[i] right_arm[i] $root #x #x
							in coordsys local rotate left_arm[i] (angleaxis 180 [0,1,0])
							)
						left_arm[i].rotation.controller.keys[r].tension = right_arm[i].rotation.controller.keys[r].tension
						left_arm[i].rotation.controller.keys[r].continuity = right_arm[i].rotation.controller.keys[r].continuity
						left_arm[i].rotation.controller.keys[r].bias = right_arm[i].rotation.controller.keys[r].bias
						)
					mirror_prog.value = 100.*i/left_arm.count
				)
				mirror_prog.value = 0
				--	Remove left arm keys, copy over the controller info, and set TCB (quaternion) rotations
				for i=1 to left_arm_only_rot.count do
					(
					deleteKeys left_arm_only_rot[i].position.controller.keys #allkeys
					deleteKeys left_arm_only_rot[i].scale.controller.keys #allkeys
					)
				)
			)catch()
		)
	)
	
	-------------------------------------------------------------------------------
	--	BIP Batch Exporter
	-------------------------------------------------------------------------------
	rollout batchBIP "Batch Export 2.0"
	(
	
	fn safeTime =
	(
		time_string = localTime
		while True do
		(
			a = findString time_string "/"
			if a == undefined then
				exit
			time_string = replace time_string a 1 "-"
		)
		while True do
		(
			a = findString time_string ":"
			if a == undefined then
				exit
			time_string = replace time_string a 1 "."
		)
		return time_string
	)
	
	fn endframe =
	(
		animEnd = (animationrange.end as string)
		animEndArray = filterString animEnd "f"
		return (animEndArray[1] as float)
	)
	
	group "specify modes"
	(
		checkbutton BIPmode "BIP Files" offset:[-45,0] enabled:false
		checkbutton XAFmode "XAF Files" offset:[20,-26] enabled:false
		dropdownlist character "" items:#("trooper","hunter","scout","fp_hands") enabled:false
	)
	group "batch"
	(
		button batchpath "Batch Folder" pos:[10,110]
		button savepath "Save Folder " pos:[10,140]
		button addlist "Add to List" pos:[10,170] enabled:false
		label batchfolderTXT "NONE" align:#left pos:[90,115]
		label savefolderTXT "NONE" align:#left pos:[90,144]
		label addlistTXT "DISABLED" align:#left pos:[90,174] enabled:false
		checkbox multiFolder "Multiple folders  " pos:[10,195]
		checkbox createCAL "Generate CAL " pos:[10,215] checked:true
		checkbox createLOG "Log To File" pos:[10,235] checked:true
		checkbox flipAnimY "Flip Animations on Z axis" pos:[10,255] checked:false
		checkbox rawCAF "Export RAW CAF data" pos:[10,275] checked:false
		checkbox saveBIP "Save BIP/XAF Files" pos:[10,295] enabled:false
		checkbox onlySaveBIP "Only Save, No Export" offset:[5,0] enabled:false
		button dobatch "-- No Files to Process --" enabled:false
		progressbar doit_prog color:red
		listbox exportlist "Folders to Process in List:" fieldWidth:152 height:10 offset:[-1,0] enabled:false
		button savelist "Save" enabled:true offset:[-30,0]
		button loadlist "Load" enabled:true offset:[25,-26]
	)
	
	on batchBIP open do
	(
		global oldsavepath = crytools.BuildPathFull
		global oldloadpath = crytools.BuildPathFull
		BIPmode.checked = true
		hiddenFolderArray = #()
		feedIn = #()
		selectArray = #()
		hiddenFolderArray = #()
	)
	
	on XAFmode changed state do
	(
		BIPmode.checked = false
		character.enabled = true
	)
	
	on BIPmode changed state do
	(
		XAFmode.checked = false
		character.enabled = false
	)
	
	on saveBIP changed state do
	(
		if saveBIP.checked == true then
		(
			onlySaveBip.enabled = true
		)
		else
		(
			onlySaveBip.enabled = false
			onlySaveBip.checked = false
		)
	)
	
	--	multiple folders checkbox
	-------------------------------------------------------------------------------
	on multiFolder changed state do
	(
		if multiFolder.checked == true then
		(
		exportlist.enabled = true
		addlist.enabled = true
		--saveQue.enabled = true
		--loadQue.enabled = true
		addlistTXT.enabled = true
		addlistTXT.text = ""
		)
		else
		(
		addlist.enabled = false
		exportlist.enabled = false
		addlistTXT.enabled = false
		saveQue.enabled = false
		loadQue.enabled = false
		addlistTXT.text = "QUE DISABLED"
		)
	)
	
	--	Get batch path
	-------------------------------------------------------------------------------
	on batchpath pressed do
	(
		global oldloadpath
		global batch_folder = getSavePath initialDir:oldloadpath caption:"Please select a folder of BIP files\nto be batch processed:"
		if batch_folder == undefined do (return undefined)
		oldloadpath = batch_folder
		
		if BIPmode.checked == true then
		( 
			global batch_files = getFiles (batch_folder + "\\*.bip")
		)
		
		if XAFmode.checked == true then
		( 
			global batch_files = getFiles (batch_folder + "\\*.xaf")
		)
		
		global batch_folder_name = (getFilenameFile batch_folder)
		global numfiles = batch_files.count
		if numfiles == 0 then 
		(
			messageBox "There are no BIP/XAF files in the selected folder." title:"Error: No BIP/XAF Files Found!"
		)
		else
		(
			dobatch.enabled = true
		)
		batchfolderTXT.text = batch_folder_name
		savefolderTXT.text = batch_folder_name
	
		if BIPmode.checked == true then
		( 
			dobatch.text = ("Export " + (numfiles as string) + " BIPs from \\" + batch_folder_name)
		)
		
		if XAFmode.checked == true then
		( 
			dobatch.text = ("Export " + (numfiles as string) + " XAFs from \\" + batch_folder_name)
		)
	
		global save_folder = batch_folder
		global save_folder_name = (getFilenameFile save_folder)
		global oldsavepath = batch_folder
	)
	
	
	--	Get save path
	-------------------------------------------------------------------------------
	on savepath pressed do
	(
		global save_folder = getSavePath initialDir:oldsavepath caption:"Please select a folder to save the CAF files:"
		if save_folder == undefined do (return undefined)
		oldsavepath = save_folder
		save_folder_name = (getFilenameFile save_folder)
		print save_folder
		savefolderTXT.text = save_folder_name
	)
	
	--	add to que button
	-------------------------------------------------------------------------------
	on addlist pressed do
	(
	save_folder_array = #(batch_folder_name)
	if exportlist.items == undefined then
		(
			exportlist.items = save_folder_array
		)
	temp = (join exportlist.items save_folder_array)
	exportlist.items = temp
	feed = #((batch_folder as string),(save_folder as string))
	if feedIN == undefined then
		(
			feedIN = #()
		)
	feedIN = feed
	--execme = (batch_folder_name + " = #()")
	--execute execme
	if hiddenFolderArray == undefined then
		(
			hiddenFolderArray = feedIn
		)
	else
		(
			append hiddenFolderArray feedin
		)
	--execme = (batch_folder_name + " = feedIn")
	--execute execme
	addlistTXT.text = ((exportlist.items.count as string) + " Folders")
	)
	
	on exportlist selected m do
	(
		--selectArray = hiddenFolderArray[m]
		batch_folder = hiddenFolderArray[m][1]
		save_folder = hiddenFolderArray[m][2]
		batchfolderTXT.text = (getFilenameFile batch_folder)
		savefolderTXT.text = (getFilenameFile save_folder)
	)
	
	on exportlist doubleClicked itm do 
	(
	temp = exportlist.items 
	deleteItem temp itm
	deleteitem hiddenFolderArray itm
	exportlist.items = temp
	addlistTXT.text = ((exportlist.items.count as string) + " Folders")
	)
	
	--	Save/load the folder que
	-------------------------------------------------------------------------------
	
	-- save the list
	on savelist pressed do
	(
		folderQue = getSaveFileName	types:"Que(*.que)|*.que" initialDir:save_folder
		outputQUE = createFile folderQue
		outputQUE = openFile folderQue mode:"w"
		format ((hiddenFolderArray as string) + "*" + (exportlist.items as string)) to: outputQUE
		close
	)
	
		
	-- import the list
	on loadlist pressed do
	(
		dobatch.enabled = true
		dobatch.text = "Batch Loaded Folders"
		import_queFile = getOpenFileName getSaveFileName types:"Que(*.que)|*.que" initialDir:crytools.BuildPathFull
		import_que = openFile import_queFile mode:"r"
		while not eof import_que do
		(
			importedList = readline import_que
			importedListBreak = (filterstring importedList "*")
		)
		close import_que
		--exportlist.items = (("\"" + importedlistbreak[2] as array))
		execme = ("cryanimationtools.rollouts[6].exportlist.items = " + importedlistbreak[2])
		execute execme
		
		execme = ("hiddenFolderArray = " + importedlistbreak[1])
		print execme
		execute execme
	)
	
	--	Process batch export
	-------------------------------------------------------------------------------
	on dobatch pressed do
	(	
		undo off
		(
			-- allow esc to break from batching
			escapeenable = true
	
			-- check to make sure the twist bones are parented correctly
			if $'Bip01 R ForeTwist' != undefined then
			(
				try
				(
					if $'Bip01 R ForeTwist'.parent.name != "Bip01 R Forearm" then
					(
						$'Bip01 R ForeTwist'.parent = $'Bip01 R Forearm'
						print "Forearms reparented!"
					)
				)
				catch
				(
					$'Bip01 R ForeTwist'.parent = $'Bip01 R Forearm'
					print "Forearms reparented!"
				)
			)
			
			--log to file
			--test = localtime
			--timeArray = (filterstring test "/ :")
			logfilename =  ("BIP_export_" + batch_folder_name + "_" + safeTime())			
			
			
			try ( biped_ctrl=$bip01.controller )
			catch
			(
				messageBox "I cannot find a biped in the scene." title:"Error: No Biped Found!"
				return undefined
			)
			
			biped_ctrl.figureMode = false
			
			fileList = undefined
			if multiFolder.checked == false then
				fileList = numfiles
			else
				fileList = exportlist.items.count
			
			
			for i = 1 to fileList do
			(
				animationrange = interval 0 1
				utilitypanel.openutility csexport
				caf_name = (getFilenameFile batch_files[i])
				print ("Exporting " + caf_name)
			
				biped_ctrl=$bip01.controller
				
				checkImport = biped.LoadBipFile biped_ctrl batch_files[i]
				while (checkImport != true) do ()
				
				--	rotate 180 deg on Z
				if flipAnimY.checked == true then
				(
					Dummy name: "rot_help" pos:[0,0,0] isSelected:off
					$Bip01.parent = $rot_help
					rot =  eulerangles 0 0 180
					rotate $rot_help rot
					select $Bip01
					max unlink
					select $rot_help
					max delete
				)
				
				
				if saveBIP.checked = true then					
					biped.SaveBipFile biped_ctrl batch_files[i]
					
				--	Export
				if rawCAF.checked == true then
					checkExport = csexport.export.export_anim (save_folder + "\\" + caf_name + ".ivo")					
				else
					checkExport = csexport.export.export_anim (save_folder + "\\" + caf_name + ".caf")

				while (checkExport != OK) do ()						
					
					
				--	create logfile
				if createLOG.checked == true then
				(					
					if ( outputLOG = openFile (save_folder + "\\" + logfilename + ".log") mode:"a" ) == undefined then
					(
						outputLOG = createFile (save_folder + "\\" + logfilename + ".log")
						outputLOG = openFile (save_folder + "\\" + logfilename + ".log") mode:"a"
						format "#filepath = animations\human\male\n\n" to: outputCAL
						--format (localTime + " --- BATCH EXPORTER STARTED PARSING FOLDER " + batch_folder + "\n") to: outputLOG
					)
					format (localTime + " --- Exporting " + caf_name + "\n") to: outputLOG
					close outputLOG
				)
				
				--	create CAL file
				if createCAL.checked == true then
				(
					if ( outputCAL = openFile (save_folder + "\\" + batch_folder_name + "_export_test.cal") mode:"a" ) == undefined then
					(						
						outputCAL = createFile (save_folder + "\\"  + batch_folder_name + "_export_test.cal")
						outputCAL = openFile (save_folder + "\\" + batch_folder_name + "_export_test.cal") mode:"a"
					)
					format (caf_name + "    = " + caf_name +".caf\n") to: outputCAL
					close outputCAL
				)
								
				doit_prog.value = 100.*i/numfiles
				gc()
			)
			doit_prog.value = 0
		)		
	)
)			
	
	-------------------------------------------------------------------------------
	-- INTERNAL TOOLS
	-------------------------------------------------------------------------------
	rollout internalTools "Internal Tools/Fixes"
	(
		group "Animation Setup"
		(
		dropdownlist chooseWeapon items:#("none","pistol","rifle","mg","law")
		)
		
		button addTrooperWeapon "Add Trooper Weapon Bone"
		
		group "XAF Import/Export"
		(
			button exportXAF "Export XAF (Trooper)" enabled:false
			checkbox animTracks "animTracks" enabled:false
			checkbox constraints "constraints" enabled:false offset:[80,-20]
			checkbox keyable "keyableTracks" offset:[35,0] enabled:false
			checkbox XAFuseRange "use current time range" enabled:false
			dropdownlist XAFcharacterExport "" items:#("trooper", "hunter","scout","fp_arms") enabled:false
			button importXAF "Import XAF (Trooper)" enabled:false
			checkbox XAFrelative "Relative" enabled:false
			checkbox XAFinsert "Insert" offset:[65,-20] enabled:false
			dropdownlist XAFcharacterImport "" items:#("trooper", "hunter","scout","fp_arms") enabled:false
		)
		
		on internalTools open do
		(
		
			if $bip01 != undefined then
			(
				undo "import_weapons" on
				(
					if $hurricane_tp == undefined then
					(
						if (doesfileexist (crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max")) != false then
						(
							mergeMAXfile (crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max") #("hurricane_tp","scar_L-C_tp","law_tp","socom_tp") #alwaysReparent 
						)
						else
						(
							messagebox ("Cannot find " + crytools.BuildPathFull + "\\tools\\maxscript\\ref\\weapon_positions.max")
						)
					)
				)
			)
		
		animTracks.checked = true
		--constraints.enabled = true
		constraints.checked = true
		XAFuseRange.checked = true
		)
		
		on chooseWeapon selected sel do
		(	
			undo "select_weapon_setup" on
			(
				biped_ctrl=$bip01.controller
				if biped_ctrl.figureMode == true then
				(
					messagebox "Please leave figure mode first"
					return default
				)
				case sel of
				(
					1:
					(
						hateMaxRots= (in coordsys world $weapon_bone.pos)
						in coordsys parent (about $weapon_bone $weapon_bone.rotation = (quat -0.0298008 -0.00344252 0.731658 -0.681011))
						$weapon_bone.pos = hateMaxRots
						hide $socom_tp
						hide $hurricane_tp
						hide $'scar_L-C_tp'
						hide $law_tp
					)
					
					2:
					(
						hateMaxRots= (in coordsys world $weapon_bone.pos)
						in coordsys parent (about $weapon_bone $weapon_bone.rotation = (quat -0.0298008 -0.00344252 0.731658 -0.681011))
						$weapon_bone.pos = hateMaxRots
						unhide $socom_tp
						hide $hurricane_tp
						hide $'scar_L-C_tp'
						hide $law_tp
					)
					
					3:
					(
						hateMaxRots= (in coordsys world $weapon_bone.pos)
						in coordsys parent (about $weapon_bone $weapon_bone.rotation = (quat -0.0298008 -0.00344252 0.731658 -0.681011))
						$weapon_bone.pos = hateMaxRots
						unhide $'scar_L-C_tp'
						hide $hurricane_tp
						hide $socom_tp
						hide $law_tp
					)
					
					4:
					(
						hateMaxRots = (in coordsys world $weapon_bone.pos)
						in coordsys parent (about $weapon_bone $weapon_bone.rotation = (quat -0.251497 0.143304 0.510626 -0.809614))
						$weapon_bone.pos = hateMaxRots
						unhide $hurricane_tp
						hide $'scar_L-C_tp'
						hide $socom_tp
						hide $law_tp
					)
					
					5:
					(
						hateMaxRots= (in coordsys world $weapon_bone.pos)
						in coordsys parent (about $weapon_bone $weapon_bone.rotation = (quat 0.681166 -0.400702 0.386398 -0.475549))
						$weapon_bone.pos = hateMaxRots
						unhide $law_tp
						hide $socom_tp
						hide $hurricane_tp
						hide $socom_tp
					)
				)
			)
		)
		
		on addTrooperWeapon pressed do
		(
			if (doesfileexist (crytools.BuildPathFull + "Game\\Objects\\Characters\\alien\\trooper\\trooper.max")) != false then
				(
					mergeMAXfile (crytools.BuildPathFull + "Game\\Objects\\Characters\\alien\\trooper\\trooper.max") #("weapon_bone") #alwaysReparent 
				)
			else
			(
				messagebox ("Cannot find " + crytools.BuildPathFull + "Game\\Objects\\Characters\\alien\\trooper\\trooper.max")
			)
		)
		
		on XAFcharacterExport selected sel do
		(
			case sel of
			(
				1: exportXAF.text = "Export XAF (Trooper)"
				2: exportXAF.text = "Export XAF (Hunter)"
			)
		)
		
		on XAFcharacterImport selected sel do
		(
			case sel of
			(
				1: importXAF.text = "Import XAF (Trooper)"
				2: importXAF.text = "import XAF (Hunter)"
			)
		)
		
		-- animated tracks changed state
		on animTracks changed state do
		(
			if animTracks.checked == true then
			(
			constraints.enabled = true
			)
			else
			(
			constraints.checked = false
			constraints.enabled = false
			)
		)
		
		on exportXAF pressed do
		(
			outputNameArr = (filterstring maxFilename ".")
			outputName = outputNameArr[1]
			XAFoldsavepath = crytools.BuildPathFull
			
			global XAFsave_file = getSaveFileName initialDir:XAFoldsavepath filename:outputname caption:"Please select a folder to save the XAF file in:"
			if XAFsave_file == undefined do
			(
				return undefined
			)
			XAFoldsavepath = XAFsave_file
			
			-- trooper export
			if XAFcharacterExport.selection == 1 then
			(
			
				if $R_rear_hand03 == undefined then
				(
					messagebox "There is no trooper in the scene."
					return undefined
				)
			
				select $MASTER
				
				if $R_mid_tent_spik02 != undefined then
				(
					$R_mid_tent_spik02.name = "_R_mid_tent_spik02"
					$R_mid_tent_spik03.name = "_R_mid_tent_spik03"
					$R_mid_tent_spik04.name = "_R_mid_tent_spik04"
					$L_mid_tent_spik04.name = "_L_mid_tent_spik04"
					$L_mid_tent_spik03.name = "_L_mid_tent_spik03"
					$L_mid_tent_spik02.name = "_L_mid_tent_spik02"
					$R_rear_tent_spik02.name = "_R_rear_tent_spik02"
					$R_rear_tent_spik03.name = "_R_rear_tent_spik03"
					$R_rear_tent_spik04.name = "_R_rear_tent_spik04"
					$L_rear_tent_spik04.name = "_L_rear_tent_spik04"
					$L_rear_tent_spik03.name = "_L_rear_tent_spik03"
					$L_rear_tent_spik02.name = "_L_rear_tent_spik02"
					$L_rear_tent_stretch.name = "R_rear_tent_stretch"
				)
				
				selectmore $_R_mid_tent_spik02
				selectmore $_R_mid_tent_spik03
				selectmore $_R_mid_tent_spik04
				selectmore $_L_mid_tent_spik04
				selectmore $_L_mid_tent_spik03
				selectmore $_L_mid_tent_spik02
				selectmore $_R_rear_tent_spik02
				selectmore $_R_rear_tent_spik03
				selectmore $_R_rear_tent_spik04
				selectmore $_L_rear_tent_spik04
				selectmore $_L_rear_tent_spik03
				selectmore $_L_rear_tent_spik02
				selectmore $root
				selectmore $spine01
				selectmore $spine02
				selectmore $spine03
				selectmore $pelvis
				selectmore $null_rope03_connect
				selectmore $'Rope3 Seg01'
				selectmore $'Rope3 Seg02'
				selectmore $'Rope3 Seg03'
				selectmore $'Rope3 Seg04'
				selectmore $'Rope3 Seg05'
				selectmore $'Rope3 Seg06'
				selectmore $'Rope3 Seg07'
				selectmore $'Rope3 Seg08'
				selectmore $'Rope3 Seg09'
				selectmore $R_rear_hand01
				selectmore $R_rear_hand01_end
				selectmore $R_rear_hand02
				selectmore $R_rear_hand02_end
				selectmore $R_rear_hand03
				selectmore $R_rear_hand03_end
				selectmore $null_rope04_connect
				selectmore $'Rope4 Seg01'
				selectmore $'Rope4 Seg02'
				selectmore $'Rope4 Seg03'
				selectmore $'Rope4 Seg04'
				selectmore $'Rope4 Seg05'
				selectmore $'Rope4 Seg06'
				selectmore $'Rope4 Seg07'
				selectmore $'Rope4 Seg08'
				selectmore $'Rope4 Seg09'
				selectmore $L_rear_hand01
				selectmore $L_rear_hand01_end
				selectmore $L_rear_hand02
				selectmore $L_rear_hand02_end
				selectmore $L_rear_hand03
				selectmore $L_rear_hand03_end
				selectmore $R_rear_tent_spik01
				selectmore $L_rear_tent_spik01
				selectmore $R_mid_tent01
				selectmore $null_rope02_connect
				selectmore $'Rope2 Seg01'
				selectmore $'Rope2 Seg02'
				selectmore $'Rope2 Seg03'
				selectmore $'Rope2 Seg04'
				selectmore $'Rope2 Seg05'
				selectmore $'Rope2 Seg06'
				selectmore $'Rope2 Seg07'
				selectmore $'Rope2 Seg09'
				selectmore $R_mid_hand01
				selectmore $R_mid_hand01_end
				selectmore $R_mid_hand02
				selectmore $R_mid_hand02_end
				selectmore $R_mid_hand03
				selectmore $R_mid_hand03_end
				selectmore $R_mid_tent_spik01
				selectmore $L_mid_tent01
				selectmore $null_rope01_connect
				selectmore $'Rope1 Seg01'
				selectmore $'Rope1 Seg02'
				selectmore $'Rope1 Seg03'
				selectmore $'Rope1 Seg04'
				selectmore $'Rope1 Seg05'
				selectmore $'Rope1 Seg06'
				selectmore $'Rope1 Seg07'
				selectmore $'Rope1 Seg09'
				selectmore $L_mid_hand01
				selectmore $L_mid_hand01_end
				selectmore $L_mid_hand02
				selectmore $L_mid_hand02_end
				selectmore $L_mid_hand03
				selectmore $L_mid_hand03_end
				selectmore $L_mid_tent_spik01
				selectmore $chest
				selectmore $R_humerus
				selectmore $R_forearm
				selectmore $R_front_hand
				selectmore $R_front_hand01_01
				selectmore $R_front_hand01_02
				selectmore $R_front_hand01_02_end
				selectmore $R_front_hand02_01
				selectmore $R_front_hand02_02
				selectmore $R_front_hand02_02_end
				selectmore $R_front_hand03_01
				selectmore $R_front_hand03_02
				selectmore $R_front_hand03_02_end
				selectmore $R_weapon_bone
				selectmore $L_humerus
				selectmore $L_forearm
				selectmore $L_front_hand
				selectmore $L_front_hand01_01
				selectmore $L_front_hand01_02
				selectmore $L_front_hand01_02_end
				selectmore $L_front_hand02_01
				selectmore $L_front_hand02_02
				selectmore $L_front_hand02_02_end
				selectmore $L_front_hand03_01
				selectmore $L_front_hand03_02
				selectmore $L_front_hand03_02_end
				selectmore $L_weapon_bone
				selectmore $L_booster
				selectmore $L_booster_end
				selectmore $R_booster
				selectmore $R_booster_end
				selectmore $'Bip01 Neck'
				selectmore $'Bip01 Head'
				selectmore $eye_left_bone
				selectmore $eye_right_bone
				
				selectmore $Slider01
				selectmore $Slider02
				selectmore $Slider03
				selectmore $Slider04
				selectmore $Slider05
				selectmore $Slider06
				selectmore $Slider07
				selectmore $Slider08
				selectmore $L_front_tent_stretch
				selectmore $R_front_tent_stretch
				selectmore $L_rear_tent_stretch
				selectmore $R_rear_tent_stretch
				
				XAFnodeExport = (selection as array)
			)
			
			-- save the XAF file
			if XAFuseRange.checked == true then
			(
				loadsaveanimation.saveanimation (XAFsave_file + ".xaf") XAFnodeExport #() #() animatedTracks:animTracks.checked includeConstraints:constraints.checked keyableTracks:keyable.checked savesegment:true segInterval:animationrange
			)
			else
			(
				loadsaveanimation.saveanimation (XAFsave_file + ".xaf") XAFnodeExport #() #() animatedTracks:animTracks.checked includeConstraints:constraints.checked keyableTracks:keyable.checked
			)
		)
		
		
		-- import trooper
		on importXAF pressed do
		(				
			if XAFoldsavepath == undefined then
			(
				XAFoldsavepath = crytools.BuildPathFull
			)
			
			global XAFopen_file = getOpenFileName initialDir:XAFoldsavepath caption:"Please select a folder to save the XAF file in:"
			if XAFopen_file == undefined do
			(
				return undefined
			)
			XAFoldsavepath = XAFopen_file
			
			-- trooper export
			if XAFcharacterImport.selection == 1 then
			(
				if $R_rear_hand03 == undefined then
				(
					messagebox "There is no trooper in the scene."
					return undefined
				)		
				select $MASTER
				
				if $R_mid_tent_spik02 != undefined then
				(
					$R_mid_tent_spik02.name = "_R_mid_tent_spik02"
					$R_mid_tent_spik03.name = "_R_mid_tent_spik03"
					$R_mid_tent_spik04.name = "_R_mid_tent_spik04"
					$L_mid_tent_spik04.name = "_L_mid_tent_spik04"
					$L_mid_tent_spik03.name = "_L_mid_tent_spik03"
					$L_mid_tent_spik02.name = "_L_mid_tent_spik02"
					$R_rear_tent_spik02.name = "_R_rear_tent_spik02"
					$R_rear_tent_spik03.name = "_R_rear_tent_spik03"
					$R_rear_tent_spik04.name = "_R_rear_tent_spik04"
					$L_rear_tent_spik04.name = "_L_rear_tent_spik04"
					$L_rear_tent_spik03.name = "_L_rear_tent_spik03"
					$L_rear_tent_spik02.name = "_L_rear_tent_spik02"
					$L_rear_tent_stretch.name = "R_rear_tent_stretch"
				)
				
				selectmore $_R_mid_tent_spik02
				selectmore $_R_mid_tent_spik03
				selectmore $_R_mid_tent_spik04
				selectmore $_L_mid_tent_spik04
				selectmore $_L_mid_tent_spik03
				selectmore $_L_mid_tent_spik02
				selectmore $_R_rear_tent_spik02
				selectmore $_R_rear_tent_spik03
				selectmore $_R_rear_tent_spik04
				selectmore $_L_rear_tent_spik04
				selectmore $_L_rear_tent_spik03
				selectmore $_L_rear_tent_spik02
				selectmore $root
				selectmore $spine01
				selectmore $spine02
				selectmore $spine03
				selectmore $pelvis
				selectmore $null_rope03_connect
				selectmore $'Rope3 Seg01'
				selectmore $'Rope3 Seg02'
				selectmore $'Rope3 Seg03'
				selectmore $'Rope3 Seg04'
				selectmore $'Rope3 Seg05'
				selectmore $'Rope3 Seg06'
				selectmore $'Rope3 Seg07'
				selectmore $'Rope3 Seg08'
				selectmore $'Rope3 Seg09'
				selectmore $R_rear_hand01
				selectmore $R_rear_hand01_end
				selectmore $R_rear_hand02
				selectmore $R_rear_hand02_end
				selectmore $R_rear_hand03
				selectmore $R_rear_hand03_end
				selectmore $null_rope04_connect
				selectmore $'Rope4 Seg01'
				selectmore $'Rope4 Seg02'
				selectmore $'Rope4 Seg03'
				selectmore $'Rope4 Seg04'
				selectmore $'Rope4 Seg05'
				selectmore $'Rope4 Seg06'
				selectmore $'Rope4 Seg07'
				selectmore $'Rope4 Seg08'
				selectmore $'Rope4 Seg09'
				selectmore $L_rear_hand01
				selectmore $L_rear_hand01_end
				selectmore $L_rear_hand02
				selectmore $L_rear_hand02_end
				selectmore $L_rear_hand03
				selectmore $L_rear_hand03_end
				selectmore $R_rear_tent_spik01
				selectmore $L_rear_tent_spik01
				selectmore $R_mid_tent01
				selectmore $null_rope02_connect
				selectmore $'Rope2 Seg01'
				selectmore $'Rope2 Seg02'
				selectmore $'Rope2 Seg03'
				selectmore $'Rope2 Seg04'
				selectmore $'Rope2 Seg05'
				selectmore $'Rope2 Seg06'
				selectmore $'Rope2 Seg07'
				selectmore $'Rope2 Seg09'
				selectmore $R_mid_hand01
				selectmore $R_mid_hand01_end
				selectmore $R_mid_hand02
				selectmore $R_mid_hand02_end
				selectmore $R_mid_hand03
				selectmore $R_mid_hand03_end
				selectmore $R_mid_tent_spik01
				selectmore $L_mid_tent01
				selectmore $null_rope01_connect
				selectmore $'Rope1 Seg01'
				selectmore $'Rope1 Seg02'
				selectmore $'Rope1 Seg03'
				selectmore $'Rope1 Seg04'
				selectmore $'Rope1 Seg05'
				selectmore $'Rope1 Seg06'
				selectmore $'Rope1 Seg07'
				selectmore $'Rope1 Seg09'
				selectmore $L_mid_hand01
				selectmore $L_mid_hand01_end
				selectmore $L_mid_hand02
				selectmore $L_mid_hand02_end
				selectmore $L_mid_hand03
				selectmore $L_mid_hand03_end
				selectmore $L_mid_tent_spik01
				selectmore $chest
				selectmore $R_humerus
				selectmore $R_forearm
				selectmore $R_front_hand
				selectmore $R_front_hand01_01
				selectmore $R_front_hand01_02
				selectmore $R_front_hand01_02_end
				selectmore $R_front_hand02_01
				selectmore $R_front_hand02_02
				selectmore $R_front_hand02_02_end
				selectmore $R_front_hand03_01
				selectmore $R_front_hand03_02
				selectmore $R_front_hand03_02_end
				selectmore $R_weapon_bone
				selectmore $L_humerus
				selectmore $L_forearm
				selectmore $L_front_hand
				selectmore $L_front_hand01_01
				selectmore $L_front_hand01_02
				selectmore $L_front_hand01_02_end
				selectmore $L_front_hand02_01
				selectmore $L_front_hand02_02
				selectmore $L_front_hand02_02_end
				selectmore $L_front_hand03_01
				selectmore $L_front_hand03_02
				selectmore $L_front_hand03_02_end
				selectmore $L_weapon_bone
				selectmore $L_booster
				selectmore $L_booster_end
				selectmore $R_booster
				selectmore $R_booster_end
				selectmore $'Bip01 Neck'
				selectmore $'Bip01 Head'
				selectmore $eye_left_bone
				selectmore $eye_right_bone
				
				selectmore $Slider01
				selectmore $Slider02
				selectmore $Slider03
				selectmore $Slider04
				selectmore $Slider05
				selectmore $Slider06
				selectmore $Slider07
				selectmore $Slider08
				selectmore $L_front_tent_stretch
				selectmore $R_front_tent_stretch
				selectmore $L_rear_tent_stretch
				selectmore $R_rear_tent_stretch
				
				XAFnodeImport = (selection as array)
			)
			-- save the XAF file
			loadsaveanimation.loadanimation XAFopen_file XAFnodeImport relative:XAFrelative.checked insert:XAFinsert.checked
		)
	)
addRollout plantKey CryAnimationTools
addRollout bipedtools CryAnimationTools
addRollout bipSelect CryAnimationTools rolledup:true
--addRollout internalTools CryAnimationTools
addRollout generalTools CryAnimationTools rolledup: true
addRollout rangeview CryAnimationTools rolledup:true
addRollout mirrorarms CryAnimationTools rolledup:true
addRollout batchBIP CryAnimationTools rolledup:true
) --End roulloutfloater
