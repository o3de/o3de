(
	global mocapTools
	if mocapTools != undefined do ( closerolloutfloater mocapTools )
	mocapTools = newrolloutfloater "facialMocapTools" 200 780
	
	local markerSetArray = #()
	local markerOrigLoc = #()
	local mocapSetArray = #()
	local boneArray = #()
	local stabMarkers = #()
	
	rollout markerData "Marker Data"
	(
		checkbutton markerSet "Define Marker Set"
		spinner markerSize "Resize Marker Set  " width:100 offset:[65,0] enabled: false
		button convertPoints "Convert Set to Points" enabled:false
		checkbox posOnly "position" offset:[10,0] checked:true
		checkbox rotOnly "rotation" offset:[90,-20]
		button rexerXforms "Reset Marker Transforms"
		group "Reference Markers"
		(
			checkbutton spheresOn "Simulate Ref Markers" offset:[-27,0]
			checkbox threeMM "3mm" offset:[120,-25]
			checkbox fiveMM "5mm" offset:[120,0] checked:true
			checkbox onRef "Ref Markers Only" offset:[-5,-15]
		)
		group "Stabilization Markers"
		(
			checkbutton stabMarkersBtn "Define Stabilization Markers"
			button testThresh "Test Integrity" offset:[-35,0] enabled:false
			spinner threshSize "mm: " width:60 offset:[27,-22]
			checkbox stabLog "output detailed log" align:#center
		)
		label nameWarn "UI below requires template naming.."
		button colorMarkers "Color Markers" offset:[-25,0]
		checkbutton hideMarkers "Hide" offset:[45,-26]
		button markerSymmetry "Update Marker Symmetry"
		checkbox RL "Right->Left"
		checkbox LR "Left->Right" offset:[90,-20] checked:true
		group "Bone Tools"
		(
			button genBones "Generate Bones" offset:[-40,0]
			spinner boneSize "Size: " width:60 offset:[27,-22]
			button saveBones "Save Bone Influences" offset:[-26,0]
			checkbutton hideBones "Hide" offset:[62,-26]
			button loadBones "Load Bone Influences" offset:[-26,0]
		)
	
	local markerSetArray = #()
	local markerOrigLoc = #()
	local stabArray = #()
	local stabMarkers = #()
	
		on markerData open do
		(
			markerSize.value = 1.0
			boneSize.value = 1.0
			isHidden = false
			try
			(
				for obj in $*_bone do
				(
					if obj.ishidden == true then (isHidden = true)
				)
			)
			catch()
			if isHidden == true then (hideBones.checked = true)
		)
		
		on markerSet changed state do
		(
			if state == true then
			(
				markersetarray = #()
				convertPoints.enabled = true
				markerSize.enabled = true
				
				if $ == undefined then
				(
					if (queryBox "No markers selected. Generate marker set\n by pair matching existing C3D data?") == true then
					(
						textOut = "Markers Found and Added to Marker Set:\n"
						n = 0
						testFor = ""
						for obj in $C3D* do
						(
							if (getnodebyname((filterstring obj.name ":")[2])) != undefined then
							(
								append markersetarray (getnodebyname((filterstring obj.name ":")[2]))
								textOut += (obj.name + " - " + (obj.position as string) + "\n")
								n+=1
							)
							else
							(
								append testFor (obj.name + "\n")
							)
						)
						
						if testFor != "" then
						(
							
							messagebox ("C3D data present with no matched markers:\n-----------------------------------------------------\n" + testFor)
						)
						
						textOut2 = (n as string + " ")
						append textOut2 textOut
						messagebox textOut2
					)
					else
					(
						messagebox "No markers selected/found \n('C3D:' prefix search)"
						markerSet.checked = false
						return undefined
					)
				)
				
				for obj in selection do
				(
					append markerSetArray obj
					append markerOrigLoc obj.pos
				)
				markerSet.text = ("Marker Set Defined: " + markersetarray.count as string + " Markers")	
				
				--check for points
				pcheck = true
				for obj in markersetarray do
				(
					if obj.classid[1] != 8211 then
					(
						pcheck = false
					)
				)
				if pcheck == true then
				(
					convertpoints.enabled = false
				)
			)
			else
			(
				markerSet.text = "Define Marker Set"
				convertPoints.enabled = false
				markerSetArray = #()
				markerSize.enabled = false
			)
		)
		
		on markerSize changed val do
		(
			undo "resizeMarkers" on
			(
				for obj in markerSetArray do
				(
					if obj.classid[1] == 8211 then
					(
						obj.size = val
					)
					if obj.classid[1] == 8872500 then
					(
						obj.boxsize = [val,val,val]
					)
				)
			)
		)
		
		on boneSize changed val do
		(
			for obj in selection do
			(
				try
				(
					obj.radius = val
					origNode = (filterString obj.name "_")
					if orignode[3] != "bone" then
					(
						print "error: non genBone"
						return undefined
					)
					
					if origNode[1] == "R" then
					(
						mirrorNode = (getnodebyname ("L_" + origNode[2] + "_bone"))
						mirrorNode.radius = val
					)
					if origNode[1] == "L" then
					(
						mirrorNode = (getnodebyname ("R_" + origNode[2] + "_bone"))
						mirrorNode.radius = val
					)
				)
				catch()
			)
		)
		
		on convertPoints pressed do
		(
			undo "convertPoints" on
			(
				newMarkerSetArray = #()
				local newpoint = ""
				for obj in markerSetArray do
				(
					if obj.classid[1] != 8211 then
					(
						newPoint = point name:obj.name
						newPoint.wireColor = (color 255 255 255)
						if posOnly.checked == true then
						(
							newPoint.position.controller = obj.position.controller
						)
						if rotOnly.checked == true then
						(
							newPoint.rotation.controller = obj.rotation.controller
						)
						delete obj
						append newMarkerSetArray newPoint
					)
				)
				markerSetArray = newMarkerSetArray
				convertPoints.enabled = false
			)
		)
		
		on rexerXforms pressed do
		(
			try
			(
				for obj in markerSetArray do
				(
					resetXform obj
					collapseStack obj
				)
			)
			catch()
		)
		
		on spheresOn changed state do
		(
			if spheresOn.checked == true then
			(
				undo "refMarkers" on
				(
					if markersetarray.count == 0 then
					(
						messagebox "No marker set defined"
						return undefined
					)
					for obj in markersetarray do
					(
						new = sphere position:obj.position name:(obj.name + "_testSphere")
						new.wirecolor = color 255 255 255
						if threeMM.checked == true then (new.radius = .15)
						else (new.radius = .25)
						if onRef.checked == true then
						(
							new.pos = obj.pos
							new.parent = obj
						)
						else
						(
							try
							(
								if (filterstring obj.name ":" )[1] != "C3D" then
								(
									new.pos = (getnodebyname ("C3D:" + obj.name)).pos
									new.parent = (getnodebyname ("C3D:" + obj.name))
								)
								else
								(
									new.parent = obj
								)
							)
							catch
							(
								print ("no C3D:" + obj.name + " present.")
							)
						)
					)
				)
			)
			else
			(
				delete $*testSphere
				--max hide helper toggle
			)
		)
		
		on stabMarkersBtn changed state do
		(
			if state == true then
			(
				if selection.count != 0 then
				(
					stabMarkers = #()
					for obj in selection do
					(
						append stabMarkers obj
					)
					stabMarkersBtn.text = (stabMarkers.count as string + " STAB Markers Defined")
					testThresh.enabled = true
				)
			)
			else
			(
				stabMarkersBtn.text = "Define Stabilization Markers"
				stabMarkers = #()
				testThresh.enabled = false
			)
		)
		
		on testThresh pressed do
		(
			errorText = ""
			errorTextArr = #()
			stabOut = ""
			for obj1 in stabMarkers do
			(
				for obj2 in stabMarkers do
				(
					test = (crytools.distDiff obj1 obj2 detail:true logOut:true)
					append stabOut (obj1.name + ">" + obj2.name + "\n")
					append stabOut test[4]
					if test[1] > (threshSize.value/10) then
					(
						append errorTextArr ((test[1]*10) as string + "mm  |  Min: " + test[2] as string + " Max: " + test[3] as string + "  |  " + obj1.name + " <---> " + obj2.name + "\n")
					)
				)
			)
			
			if errortextArr != #() then
			(
				fn compareFN v1 v2 =
				(
					local t1 = ((crytools.cutstring (filterstring v1 "|")[1] "mm  ") as integer)
					local t2 = ((crytools.cutstring (filterstring v2 "|")[1] "mm  ") as integer)
					local d = t1-t2
					case of
					(
						(d < 0.): -1
						(d > 0.): 1
						default: 0
					)
				)
				
				qsort errorTextArr compareFN
				
				errorText = ""
				for element in errorTextArr do errorText += element
				
				errortext = (append ("------------------------------------------------------------------\n") errortext)
				errortext = (append ("Error Threshold: " + threshSize.value as string + "mm       Frames Tested:" + ((((animationrange.end as integer)/TicksPerFrame)-((animationrange.start as integer)/TicksPerFrame)) as string) + "\n") errortext)
				messagebox errortext title:("Stabilization Marker Threshold Warning!")
				
				if stabLog.checked == true then
				(
					savePath = getSaveFileName initialDir:crytools.buildPathFull caption:"Where to dump log:"
					if savePath == undefined then
					(
						return undefined
					)
					else
					(
						if (out = createFile savePath) != undefined then
						(
							format stabOut to:out
						)
					)
				)
			)
		)
		
		on genBones pressed do
		(
			undo "genBones" on
			(
				boneExist = #()
				for obj in $* do
				(
					test = (filterstring obj.name "_")
					if test[test.count] == "bone" then (append boneExist obj)
				)
				
				if boneExist.count != 0 then
				(
					textOut = ("There are " + boneExist.count as string + " existing facial bones.\nRebuild bone definitions from existing bones?")
					if (queryBox textOut) == true then
					(
						boneArray = boneExist
						newFound = ""
						for obj in boneArray do (newFound += (obj.name + "\n"))
						tellUser = ("Bone Info Rebuilt from " + bonearray.count as string + " Nodes:\n")
						append tellUser newFound
						messagebox tellUser
						return undefined
					)
				)
				
				if markersetarray.count == 0 then
				(
					messagebox "No defined marker set"
					return undefined
				)
				
				for obj in markersetarray do
				(
					new = sphere position:obj.position name:(obj.name + "_bone")
					new.wirecolor = color 255 255 255
					new.xray = true
					new.radius = 1
					new.parent = obj
					obj.ishidden = true
				)
			)
		)
		
		on saveBones pressed do
		(
			textOut = #()
			for obj in $*_bone do
			(
				append textOut (obj.name + "--" + obj.radius as string)
			)
			savePath = getSaveFileName initialDir:crytools.buildPathFull caption:"Please select a folder to dump bone info:"
			if savePath == undefined then
			(
				return undefined
			)
			crytools.writeOUT textOut savePath
		)
		
		on colorMarkers pressed do
		(
			try
			(
				$C_NOSTIP.wireColor = color 255 0 0
				lips = #($R_LIPCR,$R_LIPUP,$C_LIPUP,$L_LIPUP,$L_LIPLO,$C_LIPLO,$R_LIPLO,$L_LIPCR)
				for obj in lips do
				(
					obj.wireColor = color 0 0 255
				)
			)
			catch()
		)
		
		on markerSymmetry pressed do
		(
			if markerSetArray == #() then
			(
				messagebox "No marker set defined"
				return undefined
			)
			
			undo "markerSymmetry" on
			(
				for obj in markerSetArray do
				(
					origNode = (filterString obj.name "_")
					if RL.checked == true and origNode[1] == "R" then
					(
						mirrorNode = (getnodebyname ("L_" + origNode[2]))
						mirrorNode.position = [(obj.transform.position.x * -1),obj.transform.position.y,obj.transform.position.z]
					)
					if LR.checked == true and origNode[1] == "L" then
					(
						mirrorNode = (getnodebyname ("R_" + origNode[2]))
						mirrorNode.position = [(obj.transform.position.x * -1),obj.transform.position.y,obj.transform.position.z]
					)
				)
			)
		)
		
		on RL changed state do
		(
			if RL.checked == true then
			(
				LR.checked = false
			)
		)
		
		on LR changed state do
		(
			if LR.checked == true then
			(
				RL.checked = false
			)
		)
		
		on threeMM changed state do
		(
			if threeMM.checked == true then
			(
				fiveMM.checked = false
			)
		)
		
		on fiveMM changed state do
		(
			if fiveMM.checked == true then
			(
				threeMM.checked = false
			)
		)
		
		on hideBones changed state do
		(
			if hideBones.checked == true then
			(
				for obj in $*_bone do
				(
					obj.ishidden = true
				)
			)
			else
			(
				for obj in $*_bone do
				(
					obj.ishidden = false
				)
			)
		)
		
		on hideMarkers changed state do
		(
			if hideMarkers.checked == true then
			(
				if markersetarray.count == 0 then
				(
					messagebox "No marker set defined"
					hidemarkers.checked = false
				)
				for obj in markerSetArray do
				(
					obj.ishidden = true
				)
			)
			else
			(
				for obj in markerSetArray do
				(
					obj.ishidden = false
				)
			)
		)
		
	)
	
	rollout motionData "Motion Data"
	(
		local stabMarkersL = #()
		local stabMarkersR = #()
		local stabMarkersC = #()
		local faceMarkers = #()
		local c3dorigloc = #()
		
		button colorMocap "Prep Incoming C3D Mocap"
		button resetMocap	"ResetXform Marker Cloud"
		group "Stabilization"
		(
			checkbutton stabMarkersBtnC "Center Markers"
			checkbutton stabMarkersBtnL "  Left Markers" offset:[-45,0]
			checkbutton stabMarkersBtnR "Right Markers" offset:[45,-26]
			checkbutton markersToSTAB "Markers to Stabilize"
			button stabilizeMarkers "Stabilize" offset:[-43,0]
			checkbox leaveOriginal "Leave Original" checked:true offset:[70,-23]
		)
		checkbutton connectMocap "Connect Mocap" offset:[-25,0]
		checkbutton hideMocap "Hide" offset:[50,-26]
		checkbox gethead "Drive head" checked:false offset:[5,0] enabled:false
		checkbox getjaw "Drive Jaw" checked:true offset:[85,-20]
		button reduceKeysC3d "ReduceKeys on C3D (0.05)"
		
		on colorMocap pressed do
		(
			--check for root
			if $root == undefined then
			(
				point name:"root" position:$'C3D:C_NOSTIP'.position
			)
			for obj in $C3D* do
			(
				if obj.parent != $root then
				(
					obj.parent = $root
					format "% parented to root" obj.name
				)
			)
			
			--create layer and color C3D
			if (LayerManager.getLayerFromName "C3D") == undefined then
			(
				LayerManager.newLayerFromName "C3D"
			)
			LM = LayerManager.getLayerFromName "C3D"
			for obj in $C3D* do
			(
				obj.wirecolor = color 0 255 0
				LM.addnode obj
			)
			LM.addnode $root
			
			--check bone layer
			if (LayerManager.getLayerFromName "bip") == undefined then
			(
				LayerManager.newLayerFromName "bip"
			)
			LM = LayerManager.getLayerFromName "bip"
			for obj in $* do
			(
				test = (filterstring obj.name "_")
				if test[test.count] == "bone" then (LM.addnode obj)
			)
			LM.addnode $C_HDB_lookOffset
		)
		
		on hideMocap changed state do
		(
			if hideMocap.checked == true then
			(
				for obj in $*C3D* do
				(
					obj.ishidden = true
				)
			)
			else
			(
				for obj in $*C3D* do
				(
					obj.ishidden = false
				)
			)
		)
		
		on resetMocap pressed do
		(
			try
			(
				for obj in $C3D* do
				(
					resetXform obj
					collapseStack obj
					print (obj.name + " reset")
				)
			)
			catch()
		)
	
		on stabMarkersBtnL changed state do
		(
			if stabMarkersBtnL.checked == true then
			(
				if selection.count == 0 then
				(
					messagebox "Select one or more STAB markers"
					stabMarkersBtnL.checked = false
					return undefined
				)
				else
				(
					stabmarkersL = #()
					for obj in selection do (append stabMarkersL obj)
					stabMarkersBtnL.text = (stabMarkersL.count as string + " L Markers")
				)
			)
			else
			(
				stabMarkersL = #()
				stabMarkersBtnL.text = "  Left Markers "
			)
		)
		
		on stabMarkersBtnR changed state do
		(
			if stabMarkersBtnR.checked == true then
			(
				if selection.count == 0 then
				(
					messagebox "Select one or more STAB markers"
					stabMarkersBtnR.checked = false
					return undefined
				)
				else
				(
					stabmarkersR = #()
					for obj in selection do (append stabMarkersR obj)
					stabMarkersBtnR.text = (stabMarkersR.count as string + " R Markers")
				)
			)
			else
			(
				stabMarkersR = #()
				stabMarkersBtnR.text = " Right Markers "
			)
		)
		
		on stabMarkersBtnC changed state do
		(
			if stabMarkersBtnC.checked == true then
			(
				if selection.count == 0 then
				(
					messagebox "Select one or more STAB markers"
					stabMarkersBtnC.checked = false
					return undefined
				)
				else
				(
					stabmarkersC = #()
					for obj in selection do (append stabMarkersC obj)
					stabMarkersBtnC.text = (stabMarkersC.count as string + " C Markers")
				)
			)
			else
			(
				stabMarkers = #()
				stabMarkersBtnC.text = "Center Markers"
			)
		)
		
		on markersToSTAB changed state do
		(
			if markersToSTAB.checked == true then
			(
				if selection.count == 0 then
				(
					messagebox "Nothing selected"
					return undefined
				)
				else
				(
					faceMarkers = #()
					for obj in selection do (append faceMarkers obj)
					
					/*for obj in stabMarkers do
					(
						if (finditem faceMarkers obj) != 0 then
						(
							messagebox (obj.name + "is a STAB marker and cannot be a faceMarker")
							markersToSTAB.checked = false
							return undefined
						)
					)*/
					
					markersToSTAB.text = (faceMarkers.count as string + " Markers to Stabilize")
				)
			)
			else
			(
				markersToSTAB.text = "Select Markers to Stabilize"
				faceMarkers = #()
				stabilizeMarkers.enabled = false
			)
		)
		
		on stabilizeMarkers pressed do
		(
			undo "stabilizeMarkers" on
			(
				local left
				local right
				local center
				
				if $faceAttach != undefined then (delete $faceAttach)
				new = point name:"faceAttach" size:2
				new.wirecolor = color 176 26 26
				new.cross = off
				new.box = on
				new.axistripod = on
				
				tempMarkers = #()
				
				for obj in faceMarkers do
				(
					new = point name:(obj.name + "_fmTemp") size:2
					new.wirecolor = color 240 234 79
					new.transform = obj.transform
					new.cross = off
					new.box = on
					new.axistripod = off
					append tempMarkers new
					new.parent = $faceAttach
				)
				
				for i=animationrange.start to animationrange.end do
				(
					with animate on
					(
						at time i
						(
							left = (crytools.avgArray stabMarkersL nodePos:true)
							right = (crytools.avgArray stabMarkersR nodePos:true)
							center = (crytools.avgArray stabMarkersC nodePos:true)
							$faceAttach.pos = ((left + right)/2)
							trans = $faceattach.transform
							trans[1] = -(normalize (center-right))
							trans[3] = -(normalize (cross (normalize (center - $faceattach.pos)) trans[1]))
							trans[2] = -(normalize (cross trans[1] trans[3]))
							$faceAttach.transform = trans
							
							for obj in tempMarkers do
							(
								in coordsys $faceAttach obj.pos = in coordsys $faceAttach (getnodebyname (crytools.cutstring obj.name "_fmTemp")).pos
							)
						)
					)
				)
				if leaveOriginal.checked != true then
				(
					for obj in facemarkers do
					(
						delete obj
					)
					for obj in $*_fmTemp do
					(
						obj.name = (crytools.cutstring obj.name "_fmTemp")
					)
				)
			)
		)
		
		on connectMocap changed state do
		(
			mocapSetArray = #()
			c3dorigLoc = #()
			
			if connectMocap.checked == true then
			(
				connectMocap.text = "Disconnect Mocap"
				hideMocap.checked = true
				mPair = ""
				unFound = ""
				msetdef = true
				if mocaptools.rollouts[1].markersetarray.count == 0 then (msetdef = false)
				if mocaptools.rollouts[1].markerOrigLoc.count == 0 then (msetdef = false)
				
				for obj in $C3D* do
				(
					append c3dorigLoc obj.pos
					if (getnodebyname(((filterstring obj.name ":")[2]) + "_bone")) == undefined then
					(
						unFound += (obj.name + "\n")
					)
				)
				if unfound != "" then (messagebox ("C3D data present with no matched markers/bones:\n-----------------------------------------------------\n" + unFound))
				
				undo "connectMocap" on
				(
					for obj in $*C3D* do
					(
						if getnodebyname((filterstring obj.name ":")[2]) != undefined then
						(
							mPair = getnodebyname((filterstring obj.name ":")[2])
							
							--rebuild marker info if needed
							if msetdef == false then
							(
								append markersetarray mPair
								append markerOrigLoc mPair.pos
							)
							mocaptools.rollouts[1].markerSet.text = ("Marker Set Defined: " + markersetarray.count as string + " Markers")
							mocaptools.rollouts[1].markerSet.checked = true
							
							move obj (mPair.position - obj.position)
							append mocapSetArray obj
						)
					)
					for obj in markersetarray do (obj.ishidden = true)
				)
				
				--build constraints
				undo "buildConstraints" on
				(
					unfound = ""
					for obj in $C3D* do
					(
						if (getnodebyname ((crytools.cutstring obj.name "C3D:") + "_bone")) == undefined then (unFound += obj.name + "\n")
					)
					if unfound != "" then (messagebox ("C3D data present with no matched markers/bones:\n-----------------------------------------------------\n" + unFound))
						
					for obj in $C3D* do
					(
						if (getnodebyname ((crytools.cutstring obj.name "C3D:") + "_bone")) != undefined then
						(
							new = (getnodebyname ((crytools.cutstring obj.name "C3D:") + "_bone"))
							new.pos.controller = position_list()
							new.pos.controller.Available.controller = Position_Constraint()
							new.pos.controller.Position_Constraint.constraints.appendTarget obj 100
						)
					)
				)							
				
				/*--connect head
				if getHead.checked == true then
				(
					undo "righead" on
					(
						if $'Bip01 Head Target' == undefined then (messagebox "Cannot find 'Bip01 Head Target' mesh.")
						head = $'Bip01 Head Target'
						
						--create aim target
						if $C_HDB_lookOffset != undefined then (delete $C_HDB_lookOffset)
						new = point rotation:head.rotation position:head.position name:"C_HDB_lookOffset"
						new.size = 4.0
						new.wirecolor = (color 14 255 2)
						in coordsys local new.position.x = in coordsys new $'C3D:C_HDB'.position.x
						new.parent = $'C3D:C_HDB'
						
						--create aim constraints
						head.rotation.controller = rotation_list()
						head.rotation.controller.Available.controller = LookAt_Constraint()
						head.rotation.controller.LookAt_Constraint.target_axis = 0
						head.rotation.controller.LookAt_Constraint.upnode_world = false
						head.rotation.controller.LookAt_Constraint.pickUpNode = $'C3D:C_NOSBRI'
						head.rotation.controller.LookAt_Constraint.controller.lookat_vector_length = 14
						head.rotation.controller.LookAt_Constraint.controller.upnode_ctrl = 0
						head.rotation.controller.LookAt_Constraint.appendTarget $'C_HDB_lookOffset' 100.00
						head.rotation.controller.LookAt_Constraint.controller.StoUP_axis = 1
					)
				)*/
						
				--connect jaw
				if getJaw.checked == true then
				(
					undo "rigJaw" on
					(
						--create targets
						mandible = $'Bip01 Mandible target'
						
						if $mandibleOrig != undefined then delete ($mandibleOrig)
						new = point rotation:mandible.rotation position:mandible.position name:"mandibleOrig"
						new.parent = $'Bip01 Head Target'
						mandible.parent = new
						new.size = 2
						
						if $CHIN_origOffset != undefined then (delete $CHIN_origOffset)
						new = point rotation:mandible.rotation position:mandible.position name:"CHIN_origOffset"
						in coordsys mandible new.position.x = in coordsys mandible $'C3D:C_CHIN'.position.x
						new.parent = $mandibleOrig
						new.size = 2
						
						if $CHIN_lookOffset != undefined then (delete $CHIN_lookOffset)
						new = point rotation:mandible.rotation position:mandible.position name:"CHIN_lookOffset"
						in coordsys mandible new.position.x = in coordsys mandible $'C3D:C_CHIN'.position.x
						new.parent = $'C3D:C_CHIN'
						new.size = 2
						
						--constrain jaw
						jaw = $'Bip01 Mandible target'
						if (jaw.rotation.controller as string) != "Controller:Rotation_List" then (jaw.rotation.controller = rotation_list())
						
						lookAtController = false
						for c in (getsubanimnames jaw.rotation.controller) do
						(
							if c == #lookat_controller then (lookatcontroller = true)
						)
						
						if lookatcontroller == false then (jaw.rotation.controller.Available.controller = LookAt_Constraint())
						jaw.rotation.controller.LookAt_Constraint.target_axis = 0
						jaw.rotation.controller.LookAt_Constraint.upnode_world = true
						jaw.rotation.controller.LookAt_Constraint.pickUpNode = $'C3D:C_NOSBRI'
						jaw.rotation.controller.LookAt_Constraint.controller.lookat_vector_length = 14
						jaw.rotation.controller.LookAt_Constraint.controller.upnode_ctrl = 0
						jaw.rotation.controller.LookAt_Constraint.appendTarget $'CHIN_lookOffset' 100.00
						jaw.rotation.controller.LookAt_Constraint.controller.StoUP_axis = 1
						jaw.rotation.controller.LookAt_Constraint.controller.StoUP_axisFlip = true
					)
				)
			)
			else
			(
				c3d = #()

				for obj in $C3D* do
				(
					append c3d obj
					if (getnodebyname ((crytools.cutstring obj.name "C3D:") + "_bone")) != undefined then
					(
						(getnodebyname ((crytools.cutstring obj.name "C3D:") + "_bone")).pos.controller = Position_XYZ()
					)
				)
				print c3dorigLoc
				for i=1 to c3dorigLoc.count do
				(
					c3d[i].position = c3dOrigLoc[i]
				)
				
				--remove jaw rig
				for i=1 to ($'Bip01 Mandible target'.rotation.controller.getCount()) do
				(
					if ($'Bip01 Mandible target'.rotation.controller[i] as string) == "SubAnim:LookAt_Constraint" then
					(
						$'Bip01 Mandible target'.rotation.controller.delete i
					)
				)
				try (delete $'CHIN_lookOffset')catch(print "no CHIN_lookOffset found (delete)")
				try(delete $'CHIN_origOffset')catch(print "no CHIN_origOffset found (delete)")
				$'Bip01 Mandible target'.rotation.controller = Euler_XYZ()
				connectMocap.text = "Connect Mocap"
				for obj in markersetarray do (obj.ishidden = false)
			)
		)
		
		on reduceKeysC3d pressed do
		(
			for obj in $C3D* do
			(
				reduceKeys obj.pos.controller 0.05 1f
			)
		)
	)
	addRollout markerData mocapTools --rolledup:true
	addRollout motionData mocapTools
)