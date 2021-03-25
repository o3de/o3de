/*
[DESCRIPTION]
FBX to Bip conversion script.

[USAGE]
With a merged in FBX bone structure and a matching biped in the scene run this script.

It will go frame by frame and align the Bip structure to the Bone structure.

One limitation, the script is hardcoded to work with the Bone structure names with "_Bip01" as a prefix and the matching bip being named "Bip01"

This will be adressed in the future.

[CREATION INFO]
Author:Paul Hormis
Last Updated: July 17,  2006

[VERSION HISTORY]
v1.00	Created

Copyright (C) 2006  Paul Hormis
*/



Global FBXtoBipXFer, AnimXferProgress

Struct FBXtoBipXFerStruct
(
	PelvisRef = undefined,
	PelvisRot = undefined,
	PelvisRotFinal = undefined,
	LThighRef = undefined,
	RThighRef = undefined,
	BipPartStep = 100.0 / 51.0,

	function DoCreatePointAtPivot ObjectForNull: =
	(
		try
		(
			tempRet = undefined
			for MSobj in ObjectForNull do
				with animate off
				(
					PivotPointHelper = Point pos:(MSobj.transform.pos) isSelected:off size:3 centermarker:off axistripod:off cross:on Box:off constantscreensize:off drawontop:off wirecolor:yellow
					PivotPointHelper.name = ((MSobj.name as string) + "_PivotPoint")

					in coordsys world PivotPointHelper.rotation = inverse(MSobj.transform.rotationPart)
					in coordsys world PivotPointHelper.pos = MSobj.transform.pos
					tempRet = PivotPointHelper
				)
		
			return tempRet
		)catch()
	),
			
	function DoPosRotConst SelectedObj PARENTOBJ =
	(
		SelectedObj.position.controller = position_constraint()
		SelectedObj.rotation.controller = orientation_constraint()
		
		SelectedObj.position.controller.appendTarget PARENTOBJ 100
		SelectedObj.rotation.controller.appendTarget PARENTOBJ 100
	
		SelectedObj.position.controller.relative =  false
		SelectedObj.rotation.controller.relative =  false
	),
			
	fn DoRotConst SelectedObj PARENTOBJ =
	(
		SelectedObj.rotation.controller = orientation_constraint()
		SelectedObj.rotation.controller.appendTarget PARENTOBJ 100
		SelectedObj.rotation.controller.relative =  false
	),
			
	function SetBipPosAndRot BipTarget: BoneSource: step: =
	(
		biped.setTransform BipTarget #pos BoneSource.pos true
		biped.setTransform BipTarget #rotation BoneSource.transform.rotation true
	),

	function SetBipRot BipTarget: BoneSource: step: =
	(
		biped.setTransform BipTarget #rotation BoneSource.transform.rotation true
	),

	function SetBipPos BipTarget: BoneSource: step: =
	(
		biped.setTransform BipTarget #pos BoneSource.pos true
	),			

	
	function DoFBXtoBipXFer =
	(
		
		FBXtoBipXFer.LThighRef = (FBXtoBipXFerStruct.DoCreatePointAtPivot ObjectForNull:#($'_Bip01 L Thigh'))
		FBXtoBipXFer.RThighRef = (FBXtoBipXFerStruct.DoCreatePointAtPivot ObjectForNull:#($'_Bip01 R Thigh'))
		
		FBXtoBipXFer.LThighRef.parent = $'_Bip01 L Thigh'
		FBXtoBipXFer.RThighRef.parent = $'_Bip01 R Thigh'
		
		FBXtoBipXFer.PelvisRef = point name:"PelvisReferenceLocation" size:3 
		
		FBXtoBipXFerStruct.DoPosRotConst FBXtoBipXFer.PelvisRef FBXtoBipXFer.LThighRef
		FBXtoBipXFerStruct.DoPosRotConst FBXtoBipXFer.PelvisRef FBXtoBipXFer.RThighRef
		
		FBXtoBipXFer.PelvisRot = point pos:(FBXtoBipXFer.PelvisRef.transform.pos) name:"PelvisReferenceRotation" size:3 isSlected:off cross:off box:on
		FBXtoBipXFer.PelvisRot.parent = FBXtoBipXFer.PelvisRef
		
		FBXtoBipXFerStruct.DoRotConst FBXtoBipXFer.PelvisRot $'_Bip01 Pelvis'
		
		FBXtoBipXFer.PelvisRotFinal = (FBXtoBipXFerStruct.DoCreatePointAtPivot ObjectForNull:FBXtoBipXFer.PelvisRot)

		in coordsys local rotate FBXtoBipXFer.PelvisRotFinal (angleaxis 90 [0,1,0])
		in coordsys local rotate FBXtoBipXFer.PelvisRotFinal (angleaxis 90 [0,0,1])
		
		FBXtoBipXFer.PelvisRotFinal.parent = FBXtoBipXFer.PelvisRot
	
		setCommandPanelTaskMode mode:#create  --  Sets the command panel to create.  Motion panel slows down processing.
		cui.commandPanelOpen = false  --  hides the command panel
		clearSelection()  --  clears the selection
		
		if (viewport.getlayout()) != #layout_1 do (max tool maximize)  --  If the viewport is not maximized then it will do so.
		
		StartFrame = animationRange.start.frame as integer
		EndFrame = animationRange.end.frame as integer

		frameCount = EndFrame - StartFrame

		progressFrameSteps = undefined
		progressFrameSteps = 100.0 / frameCount
		
		
		
		rollout AnimXferProgress "AnimationTransfer Progress" width:525 height:32
			(
				label ProgressInfo "Processing Animation Transfer" pos:[10,2] width:300 height:15
				label CurrentFrameLabel "Frame:" pos:[400,2] width:35 height:15
				label CurrentFrame "" pos:[440,2] width:80 height:15
				progressBar SubProgress "" pos:[10,17] width:505 height:7 color:blue
				progressBar MainProgress "" pos:[10,23] width:505 height:7 color:green
			)
		createdialog AnimXferProgress 

		animButtonState = true
		
		for x = 0 to frameCount do
			with redraw off
			(
				slidertime = (StartFrame + x)
		
				AnimXferProgress.CurrentFrame.text = (slidertime.frame as integer) as string
				AnimXferProgress.MainProgress.value = (progressFrameSteps * x + 1)  --  I commented this out for speed
		
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01' BoneSource:FBXtoBipXFer.PelvisRotFinal

				biped.setTransform $'Bip01 Pelvis' #rotation $'Bip01 Pelvis'.transform.rotation true
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 Spine' BoneSource:$'_Bip01 Spine' step:2
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 Spine1' BoneSource:$'_Bip01 Spine1' step:3
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 Spine2' BoneSource:$'_Bip01 Spine2' step:3
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 Spine3' BoneSource:$'_Bip01 Spine3' step:3
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 Neck' BoneSource:$'_Bip01 Neck' step:4
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 Head' BoneSource:$'_Bip01 Head' step:5
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Clavicle' BoneSource:$'_Bip01 L Clavicle' step:6
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L UpperArm' BoneSource:$'_Bip01 L UpperArm' step:7
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Forearm' BoneSource:$'_Bip01 L Forearm' step:8
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Hand' BoneSource:$'_Bip01 L Hand' step:9
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger0' BoneSource:$'_Bip01 L Finger0' step:10
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger01' BoneSource:$'_Bip01 L Finger01' step:11
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger02' BoneSource:$'_Bip01 L Finger02' step:12
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger1' BoneSource:$'_Bip01 L Finger1' step:13
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger11' BoneSource:$'_Bip01 L Finger11' step:14
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger12' BoneSource:$'_Bip01 L Finger12' step:15
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger2' BoneSource:$'_Bip01 L Finger2' step:16
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger21' BoneSource:$'_Bip01 L Finger21' step:17
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger22' BoneSource:$'_Bip01 L Finger22' step:18
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger3' BoneSource:$'_Bip01 L Finger3' step:19
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger31' BoneSource:$'_Bip01 L Finger31' step:20
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger32' BoneSource:$'_Bip01 L Finger32' step:21
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger4' BoneSource:$'_Bip01 L Finger4' step:22
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger41' BoneSource:$'_Bip01 L Finger41' step:23
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Finger42' BoneSource:$'_Bip01 L Finger42' step:24
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Clavicle' BoneSource:$'_Bip01 R Clavicle' step:25
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R UpperArm' BoneSource:$'_Bip01 R UpperArm' step:26
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Forearm' BoneSource:$'_Bip01 R Forearm' step:27
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Hand' BoneSource:$'_Bip01 R Hand' step:28
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger0' BoneSource:$'_Bip01 R Finger0' step:29
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger01' BoneSource:$'_Bip01 R Finger01' step:30
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger02' BoneSource:$'_Bip01 R Finger02' step:31
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger1' BoneSource:$'_Bip01 R Finger1' step:31
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger11' BoneSource:$'_Bip01 R Finger11' step:33
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger12' BoneSource:$'_Bip01 R Finger12' step:34
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger2' BoneSource:$'_Bip01 R Finger2' step:35
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger21' BoneSource:$'_Bip01 R Finger21' step:36
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger22' BoneSource:$'_Bip01 R Finger22' step:37
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger3' BoneSource:$'_Bip01 R Finger3' step:38
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger31' BoneSource:$'_Bip01 R Finger31' step:39
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger32' BoneSource:$'_Bip01 R Finger32' step:40
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger4' BoneSource:$'_Bip01 R Finger4' step:41
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger41' BoneSource:$'_Bip01 R Finger41' step:42
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Finger42' BoneSource:$'_Bip01 R Finger42' step:43
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Thigh' BoneSource:$'_Bip01 L Thigh' step:44
				--FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Calf' BoneSource:$'_Bip01 L Calf' step:45
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Foot' BoneSource:$'_Bip01 L Foot' step:46
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 L Toe0' BoneSource:$'_Bip01 L Toe0' step:47
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Thigh' BoneSource:$'_Bip01 R Thigh' step:48
				--FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Calf' BoneSource:$'_Bip01 R Calf' step:49
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Foot' BoneSource:$'_Bip01 R Foot' step:50
				FBXtoBipXFer.SetBipPosAndRot BipTarget:$'Bip01 R Toe0' BoneSource:$'_Bip01 R Toe0' step:51
				gc()  --  I added this garbage collection to try to speed up the script.
		)
				
		delete FBXtoBipXFer.PelvisRef
		delete FBXtoBipXFer.PelvisRot
		delete FBXtoBipXFer.PelvisRotFinal
		delete FBXtoBipXFer.LThighRef
		delete FBXtoBipXFer.RThighRef
		animButtonState = false
		destroyDialog AnimXferProgress
		cui.commandPanelOpen = true
	)
)

FBXtoBipXFer = FBXtoBipXFerStruct()


FBXtoBipXFer.DoFBXtoBipXFer()
