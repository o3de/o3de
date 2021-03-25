-- 
-- This is a modified copy of ui\usermacros\Macro_SkinTools.mcr from 3DS MAX 2012 package.
-- 
/*

Skin Operations Macro Script File

 Created:  		Aug 6 2000
	
 Author :   Peter Watje
 Version:  3ds max 6

 	12 dec 2003, Pierre-Felix Breton, 
		added product switcher: this macro file can be shared with all Discreet products


*/
--***********************************************************************************************
-- MODIFY THIS AT YOUR OWN RISK
-- 

fn getSkinOps = (
	try (
		if(crySkinOps.isCrySkin(modPanel.GetcurrentObject())) then
			(crySkinOps)
		else
			(skinOps)
	)
	catch (
		(skinOps)
	)
)

MacroScript SkinLoopSelection
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SKINLOOPSELECTION_BUTTONTEXT~
Category:~SKINLOOPSELECTION_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SKINLOOPSELECTION_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)	
	on execute do
	(
		(getSkinOps()).loopSelection (modPanel.GetcurrentObject())
	)
)

MacroScript SkinRingSelection
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SKINRINGSELECTION_BUTTONTEXT~
Category:~SKINRINGSELECTION_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SKINRINGSELECTION_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)	
	on execute do
	(
		(getSkinOps()).ringSelection (modPanel.GetcurrentObject())
	)
)

MacroScript SkinGrowSelection
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SKINGROWSELECTION_BUTTONTEXT~
Category:~SKINGROWSELECTION_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SKINGROWSELECTION_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)	
	on execute do
	(
		(getSkinOps()).growSelection (modPanel.GetcurrentObject())
	)
)

MacroScript SkinShrinkSelection
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SKINSHRINKSELECTION_BUTTONTEXT~
Category:~SKINSHRINKSELECTION_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SKINSHRINKSELECTION_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)	
	on execute do
	(
		(getSkinOps()).shrinkSelection (modPanel.GetcurrentObject())
	)
)

MacroScript SkinSelectVerticesByBone
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SKINSELECTVERTICESBYBONE_BUTTONTEXT~
Category:~SKINSELECTVERTICESBYBONE_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SKINSELECTVERTICESBYBONE_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)	
	on execute do
	(
		(getSkinOps()).selectVerticesByBone (modPanel.GetcurrentObject())
	)
)



MacroScript WeightTable_Dialog
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~WEIGHTTABLE_DIALOG_BUTTONTEXT~
Category:~WEIGHTTABLE_DIALOG_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~WEIGHTTABLE_DIALOG_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isChecked return 
	(
		if (classof(modPanel.GetcurrentObject())) == Skin then
		(
			((getSkinOps()).isWeightTableOpen (modPanel.GetcurrentObject()) != 0)
		)
		else 
		(
			false
		)
	)
	
	on closeDialogs do 
	(
		(getSkinOps()).closeWeightTable  (modPanel.GetcurrentObject())
	) 
	
	on execute do
	(
		(getSkinOps()).WeightTable  (modPanel.GetcurrentObject())
	)
)
MacroScript BlendWeights
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~BLENDWEIGHTS_BUTTONTEXT~
Category:~BLENDWEIGHTS_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~BLENDWEIGHTS_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).blendSelected (modPanel.GetcurrentObject())
	)
)

MacroScript RemoveZeroWeights
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~REMOVEZEROWEIGHTS_BUTTONTEXT~
Category:~REMOVEZEROWEIGHTS_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~REMOVEZEROWEIGHTS_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).RemoveZeroWeights (modPanel.GetcurrentObject())
	)
)

MacroScript WeightTool_Dialog
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~WEIGHTTOOL_DIALOG_BUTTONTEXT~
Category:~WEIGHTTOOL_DIALOG_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~WEIGHTTOOL_DIALOG_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isChecked return 
	(
		if (classof(modPanel.GetcurrentObject())) == Skin then
		(
				((getSkinOps()).isWeightToolOpen (modPanel.GetcurrentObject()) != 0)
		)
		else
		(
			false
		)
	)	
	on closeDialogs do 
	(
		(getSkinOps()).closeWeightTool  (modPanel.GetcurrentObject())
	) 
	
	on execute do
	(
		(getSkinOps()).WeightTool  (modPanel.GetcurrentObject())
	)
)

MacroScript SetWeight_00
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SETWEIGHT_00_BUTTONTEXT~
Category:~SETWEIGHT_00_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SETWEIGHT_00_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.0
	)
)

MacroScript SetWeight_01
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SETWEIGHT_01_BUTTONTEXT~
Category:~SETWEIGHT_01_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SETWEIGHT_01_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.1
	)
)

MacroScript SetWeight_25
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SETWEIGHT_25_BUTTONTEXT~
Category:~SETWEIGHT_25_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SETWEIGHT_25_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.25
	)
)

MacroScript SetWeight_50
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SETWEIGHT_50_BUTTONTEXT~
Category:~SETWEIGHT_50_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SETWEIGHT_50_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.5
	)
)

MacroScript SetWeight_75
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SETWEIGHT_75_BUTTONTEXT~
Category:~SETWEIGHT_75_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SETWEIGHT_75_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.75
	)
)

MacroScript SetWeight_90
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SETWEIGHT_90_BUTTONTEXT~
Category:~SETWEIGHT_90_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SETWEIGHT_90_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(	
		(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 0.90
	)
)

MacroScript SetWeight_100
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SETWEIGHT_100_BUTTONTEXT~
Category:~SETWEIGHT_100_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SETWEIGHT_100_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) 1.0
	)
)

MacroScript SetWeight_Custom
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SETWEIGHT_CUSTOM_BUTTONTEXT~
Category:~SETWEIGHT_CUSTOM_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SETWEIGHT_CUSTOM_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		tmod = modPanel.GetcurrentObject()
		v = tmod.weightTool_weight
		(getSkinOps()).SetWeight  (modPanel.GetcurrentObject()) v
	)
)

MacroScript AddWeight
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~ADDWEIGHT_BUTTONTEXT~
Category:~ADDWEIGHT_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~ADDWEIGHT_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).AddWeight  (modPanel.GetcurrentObject()) 0.05
	)
)

MacroScript SubtractWeight
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SUBTRACTWEIGHT_BUTTONTEXT~
Category:~SUBTRACTWEIGHT_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SUBTRACTWEIGHT_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).AddWeight  (modPanel.GetcurrentObject()) -0.05
	)
)

MacroScript ScaleWeight_Custom
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SCALEWEIGHT_CUSTOM_BUTTONTEXT~
Category:~SCALEWEIGHT_CUSTOM_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SCALEWEIGHT_CUSTOM_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		tmod = modPanel.GetcurrentObject()
		v = tmod.weightTool_scale
		(getSkinOps()).ScaleWeight  (modPanel.GetcurrentObject()) v
	)
)

MacroScript ScaleWeight_Up
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SCALEWEIGHT_UP_BUTTONTEXT~
Category:~SCALEWEIGHT_UP_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SCALEWEIGHT_UP_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).ScaleWeight  (modPanel.GetcurrentObject()) 1.05
	)
)

MacroScript ScaleWeight_Down
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SCALEWEIGHT_DOWN_BUTTONTEXT~
Category:~SCALEWEIGHT_DOWN_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SCALEWEIGHT_DOWN_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).ScaleWeight  (modPanel.GetcurrentObject()) 0.95
	)
)

MacroScript CopyWeights
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~COPYWEIGHTS_BUTTONTEXT~
Category:~COPYWEIGHTS_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~COPYWEIGHTS_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).CopyWeights  (modPanel.GetcurrentObject())
	)
)

MacroScript PasteWeights
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~PASTEWEIGHTS_BUTTONTEXT~
Category:~PASTEWEIGHTS_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~PASTEWEIGHTS_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).PasteWeights  (modPanel.GetcurrentObject())
	)
)


MacroScript PasteWeightsByPos
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~PASTEWEIGHTSBYPOS_BUTTONTEXT~
Category:~PASTEWEIGHTSBYPOS_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~PASTEWEIGHTSBYPOS_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		tmod = modPanel.GetcurrentObject()
		v = tmod.weightTool_tolerance		
		(getSkinOps()).pasteWeightsByPos (modPanel.GetcurrentObject()) v
	)
)


MacroScript selectParent
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SELECTPARENT_BUTTONTEXT~
Category:~SELECTPARENT_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SELECTPARENT_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SelectParent  (modPanel.GetcurrentObject()) 
	)
)

MacroScript selectChild
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SELECTCHILD_BUTTONTEXT~
Category:~SELECTCHILD_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SELECTCHILD_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SelectChild  (modPanel.GetcurrentObject()) 
	)
)

MacroScript selectNextSibling
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SELECTNEXTSIBLING_BUTTONTEXT~
Category:~SELECTNEXTSIBLING_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SELECTNEXTSIBLING_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SelectNextSibling  (modPanel.GetcurrentObject()) 
	)
)

MacroScript selectPreviousSibling
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SELECTPREVIOUSSIBLING_BUTTONTEXT~
Category:~SELECTPREVIOUSSIBLING_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SELECTPREVIOUSSIBLING_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SelectPreviousSibling  (modPanel.GetcurrentObject()) 
	)
)




MacroScript backFaceCullVertices
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~BACKFACECULLVERTICES_BUTTONTEXT~
Category:~BACKFACECULLVERTICES_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~BACKFACECULLVERTICES_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on isChecked return 
	(
		if (classof(modPanel.GetcurrentObject())) == Skin then (
			(modPanel.GetcurrentObject()).backfacecull
		)
		else (
			false
		)
	)
	
	on execute do
	(
		if  (modPanel.GetcurrentObject()).backfacecull then
			(modPanel.GetcurrentObject()).backfacecull = false
		else  (modPanel.GetcurrentObject()).backfacecull = true
	)
)


MacroScript AddBonesFromView
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~ADDBONESFROMVIEW_BUTTONTEXT~
Category:~ADDBONESFROMVIEW_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~ADDBONESFROMVIEW_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		pushprompt ~ADDBONESFROMVIEW_PUSHPROMPT_CAPTION~
		(getSkinOps()).AddBoneFromViewStart  (modPanel.GetcurrentObject())
	)
)

MacroScript multiRemove
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~MULTIREMOVE_BUTTONTEXT~
Category:~MULTIREMOVE_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~MULTIREMOVE_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).MultiRemove  (modPanel.GetcurrentObject())
	)
)

MacroScript selectPrevious
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SELECTPREVIOUS_BUTTONTEXT~
Category:~SELECTPREVIOUS_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SELECTPREVIOUS_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SelectPreviousBone  (modPanel.GetcurrentObject()) 
	)
)

MacroScript selectNext
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SELECTNEXT_BUTTONTEXT~
Category:~SELECTNEXT_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SELECTNEXT_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).SelectNextBone  (modPanel.GetcurrentObject())
	)
)

MacroScript zoomToBone
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~ZOOMTOBONE_BUTTONTEXT~
Category:~ZOOMTOBONE_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~ZOOMTOBONE_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).ZoomToBone  (modPanel.GetcurrentObject()) FALSE
	)
)

MacroScript zoomToGizmo
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~ZOOMTOGIZMO_BUTTONTEXT~
Category:~ZOOMTOGIZMO_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~ZOOMTOGIZMO_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).ZoomToGizmo  (modPanel.GetcurrentObject()) FALSE
	)
)


MacroScript selectEndPoint
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SELECTENDPOINT_BUTTONTEXT~
Category:~SELECTENDPOINT_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SELECTENDPOINT_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on Execute do
	(
		(getSkinOps()).SelectEndPoint  (modPanel.GetcurrentObject())
	)
)

MacroScript selectStartPoint
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SELECTSTARTPOINT_BUTTONTEXT~
Category:~SELECTSTARTPOINT_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SELECTSTARTPOINT_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on Execute do
	(
		(getSkinOps()).SelectStartPoint  (modPanel.GetcurrentObject())
	)
)

MacroScript filterVertices
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~FILTERVERTICES_BUTTONTEXT~
Category:~FILTERVERTICES_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~FILTERVERTICES_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isChecked return 
	(
		if (classof(modPanel.GetcurrentObject())) == Skin then (
			(modPanel.GetcurrentObject()).filter_vertices
		)
		else (
			false
		)
	)
	
	on execute do
	(
		if  (modPanel.GetcurrentObject()).filter_vertices then
		(modPanel.GetcurrentObject()).filter_vertices = FALSE
		else  (modPanel.GetcurrentObject()).filter_vertices = TRUE
	)
)

MacroScript filterEnvelopes
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~FILTERENVELOPES_BUTTONTEXT~
Category:~FILTERENVELOPES_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~FILTERENVELOPES_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isChecked return 
	(
		if (classof(modPanel.GetcurrentObject())) == Skin then (
			(modPanel.GetcurrentObject()).filter_cross_sections
		)
		else (
			false
		)
	)	
	
	on execute do
	(
		if  (modPanel.GetcurrentObject()).filter_cross_sections then
			(modPanel.GetcurrentObject()).filter_cross_sections  = FALSE
		else  (modPanel.GetcurrentObject()).filter_cross_sections  = TRUE
	)
)

MacroScript filterCrossSections
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~FILTERCROSSSECTIONS_BUTTONTEXT~
Category:~FILTERCROSSSECTIONS_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~FILTERCROSSSECTIONS_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isChecked return 
	(
		if (classof(modPanel.GetcurrentObject())) == Skin then (
			(modPanel.GetcurrentObject()).filter_envelopes
		)
		else ( 
			false
		)
	)	
	
	on execute do
	(
		if  (modPanel.GetcurrentObject()).filter_envelopes then
		(modPanel.GetcurrentObject()).filter_envelopes  = false
		else  (modPanel.GetcurrentObject()).filter_envelopes  = true
	)
)

MacroScript excludeVerts
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~EXCLUDEVERTS_BUTTONTEXT~
Category:~EXCLUDEVERTS_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~EXCLUDEVERTS_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	
	on Execute do
	(
		(getSkinOps()).ButtonExclude  (modPanel.GetcurrentObject())
	)
)

MacroScript includeVerts
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~INCLUDEVERTS_BUTTONTEXT~
Category:~INCLUDEVERTS_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~INCLUDEVERTS_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	
	on Execute do
	(
		(getSkinOps()).ButtonInclude  (modPanel.GetcurrentObject())
	)
)

MacroScript selectIncludeVerts
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~SELECTINCLUDEVERTS_BUTTONTEXT~
Category:~SELECTINCLUDEVERTS_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~SELECTINCLUDEVERTS_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	
	on execute do
	(
		(getSkinOps()).ButtonSelectExcluded  (modPanel.GetcurrentObject())
	)
)

-- Added August 27 2000 Fred Ruff

MacroScript CopySelectedBone
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~COPYSELECTEDBONE_BUTTONTEXT~
Category:~COPYSELECTEDBONE_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~COPYSELECTEDBONE_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	
	on execute do
	(
		(getSkinOps()).copySelectedBone (modPanel.GetcurrentObject())
	)
)
MacroScript PasteToSelectedBone
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~PASTETOSELECTEDBONE_BUTTONTEXT~
Category:~PASTETOSELECTEDBONE_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~PASTETOSELECTEDBONE_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	
	on execute do
	(
		(getSkinOps()).PasteToSelectedBone (modPanel.GetcurrentObject())
	)
)
MacroScript PasteToAllBones
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~PASTETOALLBONES_BUTTONTEXT~
Category:~PASTETOALLBONES_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~PASTETOALLBONES_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	
	on execute do
	(
		(getSkinOps()).PasteToAllBones  (modPanel.GetcurrentObject())
	)
)
MacroScript AddCrossSection
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~ADDCROSSSECTION_BUTTONTEXT~
Category:~ADDCROSSSECTION_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~ADDCROSSSECTION_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin) 
	
	on execute do
	(
		(getSkinOps()).ButtonAddCrossSection  (modPanel.GetcurrentObject())
	)
)
MacroScript RemoveCrossSection
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~REMOVECROSSSECTION_BUTTONTEXT~
Category:~REMOVECROSSSECTION_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~REMOVECROSSSECTION_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin) 
	
	on execute do
	(
		(getSkinOps()).ButtonRemoveCrossSection  (modPanel.GetcurrentObject())
	)
)
MacroScript DrawEnvelopeOnTop
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~DRAWENVELOPEONTOP_BUTTONTEXT~
Category:~DRAWENVELOPEONTOP_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~DRAWENVELOPEONTOP_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on isChecked return 
	(
		if (classof(modPanel.GetcurrentObject())) == Skin then (
		 	(modPanel.GetcurrentObject()).envelopesAlwaysOnTop
		)
		else
		(
			false
		)
	)
	
	on execute do
	(
		if Selection[1].modifiers[#Skin].envelopesAlwaysOnTop then
		Selection[1].modifiers[#Skin].envelopesAlwaysOnTop = FALSE
		else Selection[1].modifiers[#Skin].envelopesAlwaysOnTop = TRUE
	)
)

MacroScript DrawCrossSectionsOnTop
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~DRAWCROSSSECTIONSONTOP_BUTTONTEXT~
Category:~DRAWCROSSSECTIONSONTOP_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~DRAWCROSSSECTIONSONTOP_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ((classof(modPanel.GetcurrentObject())) == Skin)
	on isChecked return 
	(
		if (classof(modPanel.GetcurrentObject())) == Skin then 
		(
			(modPanel.GetcurrentObject()).crossSectionsAlwaysOnTop
		)
		else 
		(
			false
		)
	)
	
	on execute do
	(
		if (modPanel.GetcurrentObject()).crossSectionsAlwaysOnTop then
			(modPanel.GetcurrentObject()).crossSectionsAlwaysOnTop = false
		else  (modPanel.GetcurrentObject()).crossSectionsAlwaysOnTop = true
	)
)

MacroScript GizmoResetRotationPlane
enabledIn:#("max") --pfb: 2003.12.12 added product switch
ButtonText:~GIZMORESETROTATIONPLANE_BUTTONTEXT~
Category:~GIZMORESETROTATIONPLANE_CATEGORY~ 
internalCategory:"Skin Modifier" 
Tooltip:~GIZMORESETROTATIONPLANE_TOOLTIP~ 
-- Needs Icon
(
	on isVisible return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on isEnabled return ( (classof(modPanel.GetcurrentObject())) == Skin)
	on execute do
	(
		(getSkinOps()).GizmoResetRotationPlane (modPanel.GetcurrentObject())
	)
)
