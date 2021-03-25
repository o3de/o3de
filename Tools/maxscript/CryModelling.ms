struct CModelling(
	GetNrstEdg, NxtEdg, IsSameLoop, SelLimELoop, NxtEdgRing, IsSameRing, SelLimERing, VertsInSameLoop, SelVertLoop,
	SelVertRing, IsQuadPoly, SelPolyLoop
)

CryModelling = cmodelling()

-------------------------Limited Edge Loop---------------------------

	CryModelling.GetNrstEdg = fn GetNrstEdg Edg1 Edg2 Target =
	(
		Edg1Vrts = (polyop.getvertsusingedge $ Edg1) as array
		Edg2Vrts = (polyop.getvertsusingedge $ Edg2) as array
		TrgtVrts = (polyop.getvertsusingedge $ Target) as array
		mP1 = (((polyop.getvert $ Edg1Vrts[1]) + (polyop.getvert $ Edg1Vrts[2]))/2)
		mP2 = (((polyop.getvert $ Edg2Vrts[1]) + (polyop.getvert $ Edg2Vrts[2]))/2)
		TmP = (((polyop.getvert $ TrgtVrts[1]) + (polyop.getvert $ TrgtVrts[2]))/2)
		if (distance  Tmp mP1) > (distance  Tmp mP2) then Edg2 else Edg1
	)

	CryModelling.NxtEdg = fn NxtEdg edg trg =
	(
		EdgsSel = polyop.getedgeselection $
		Verts = (polyop.getEdgeVerts $ edg) as array
		Tmp = #()
		NEdg = #()
		VEdgs = undefined
		for vert in Verts do
		(
			VEdgs = (((polyop.getedgesUsingVert $ vert) - #{edg}) * EdgsSel) as array
			--print VEdgs
			for x in VEdgs do
			(
				append Tmp x
			)
		)
		--print Tmp.count
		if Tmp.count == 1 then append NEdg Tmp[1] 
		else
		(
			append NEdg (CryModelling.GetNrstEdg Tmp[1] Tmp[2] trg)
		)
		NEdg[1]
	)

	CryModelling.IsSameLoop = fn IsSameLoop = 
	(
		with undo off
		(
			EdgSel = (polyop.getedgeselection $) as array
			polyop.setEdgeSelection $ EdgSel[1]
			$.buttonOp #selectEdgeLoop
			NSel = polyop.getedgeselection $
			polyop.setEdgeSelection $ EdgSel
			if ((NSel * (EdgSel as bitarray)) as array).count != 2 then false else true
		)
	)

	CryModelling.SelLimELoop = fn SelLimELoop =
	(
		try
		(
			EdgeSel = (polyop.getEdgeSelection $) as array
			if CryModelling.IsSameLoop() == true then
			(
				with undo on
				(
					with redraw off
					(
						TargEdg = EdgeSel[2]
						SEdg = EdgeSel[1]
						NEdg = EdgeSel[1]
						FinalSel = #(EdgeSel[1], EdgeSel[2])
						$.buttonOp #selectEdgeLoop
						while NEdg != EdgeSel[2] do
						(
							Edg = CryModelling.NxtEdg NEdg TargEdg
							append FinalSel Edg
							NEdg = Edg
						)
					)
				)
				polyop.setEdgeSelection  $  FinalSel
			)
			else
			(
				$.buttonOp #selectEdgeLoop
			)
		)
		catch (print "The tool works only on EditablePoly Objects.")
	)

	-------------------------Limited Edge Ring---------------------------

	CryModelling.NxtEdgRing = fn NxtEdgRing edg trg =
	(
		EdgsSel = polyop.getedgeselection $
		Verts = polyop.getEdgeVerts $ edg
		Edg1 = polyop.getedgesusingvert $ Verts[1]
		Edg2 = polyop.getedgesusingvert $ Verts[2]
		EdgFaces = (polyop.getEdgeFaces $ edg) as array
		NEdgs = #()
		if EdgFaces.count == 1 then
		(
			Edgs = polyop.getFaceEdges $ EdgFaces[1] as bitarray
			if Edgs.numberset == 4 do
			(
				(((Edgs - Edg1) - Edg2) as array)[1]
			)
		)
		else
		(
			FEdgs1 = (polyop.getFaceEdges $ EdgFaces[1]) as bitarray
			FEdgs2 = (polyop.getFaceEdges $ EdgFaces[2]) as bitarray
			if FEdgs1.numberset == 4 do
			(
				append NEdgs (((FEdgs1 - Edg1) - Edg2) as array)[1]
			)
			if FEdgs2.numberset == 4 do
			(
				append NEdgs (((FEdgs2 - Edg1) - Edg2) as array)[1]
			)
			CryModelling.GetNrstEdg NEdgs[1] NEdgs[2] trg
		)
	)

	CryModelling.IsSameRing = fn IsSameRing = 
	(
		with undo off
		(
			EdgSel = (polyop.getedgeselection $) as array
			polyop.setEdgeSelection $ EdgSel[1]
			$.buttonOp #selectEdgeRing
			NSel = polyop.getedgeselection $
			polyop.setEdgeSelection $ EdgSel
			if ((NSel * (EdgSel as bitarray)) as array).count != 2 then false else true
		)
	)

	CryModelling.SelLimERing = fn SelLimERing =
	(
		try
		(
			EdgeSel = (polyop.getEdgeSelection $) as array
			if CryModelling.IsSameRing() == true then
			(
				with undo on
				(
					with redraw off
					(
						TargEdg = EdgeSel[2]
						SEdg = EdgeSel[1]
						NEdg = EdgeSel[1]
						FinalSel = #(EdgeSel[1], EdgeSel[2])
						$.buttonOp #selectEdgeRing
						while NEdg != EdgeSel[2] do
						(
							Edg = CryModelling.NxtEdgRing NEdg TargEdg
							append FinalSel Edg
							NEdg = Edg
						)
					)
				)
				polyop.setEdgeSelection  $  FinalSel
			)
			else
			(
				$.buttonOp #selectEdgeRing
			)
		)
		catch (print "The tool works only on EditablePoly Objects.")
	)

	-------------------------Limited Vert Loop----------------------------
	CryModelling.VertsInSameLoop = fn VertsInSameLoop =
	(
		Verts = polyop.GetVertSelection $
		Edgs = #()
		for x in Verts do
		(
			Edges = polyop.GetEdgesUsingVert $ x
			polyop.setEdgeSelection $ Edges
			$.ButtonOp #SelectEdgeLoop
			Tmp = polyop.GetEdgeSelection $
			append Edgs Tmp
		)
		if (Edgs[1]*Edgs[2]).numberset != 0 then true else false
	)

	CryModelling.SelVertLoop = fn SelVertLoop =
	(
		try
		(
			if CryModelling.VertsInSameLoop() == true then
			(
				with undo on
				(
					with redraw off
					(
						Verts = polyop.GetVertSelection $
						Edgs = #()
						for x in Verts do
						(
							Edges = polyop.GetEdgesUsingVert $ x
							polyop.setEdgeSelection $ Edges
							$.ButtonOp #SelectEdgeLoop
							Tmp = polyop.GetEdgeSelection $
							append Edgs Tmp
						)
						LoopEdgs = Edgs[1]*Edgs[2]
						V1Edges = (polyop.GetEdgesUsingVert $ (Verts as array)[1]) * LoopEdgs
						V2Edges = (polyop.GetEdgesUsingVert $ (Verts as array)[2]) * LoopEdgs
						LoopArr = #()
						for i = 1 to 2 do
						(
							VEdgs = polyop.setEdgeSelection $ #{(V1Edges as array)[i], (V2Edges as array)[i]}
							CryModelling.SelLimELoop()
							append LoopArr (polyop.getEdgeSelection $)
						)
						FEdgs = LoopArr[1]*LoopArr[2]
						FVerts = #{}
						for x in FEdgs do
						(
							FVerts = FVerts + (polyop.GetVertsUsingEdge $ x)
						)
						polyop.setVertSelection $ FVerts
					)
					redrawViews()
				)
			)
			else
			(
				print "Verts are not in the same loop!"
			)
		)
		catch (print "The tool works only on EditablePoly Objects.")
	)

	-------------------------Limited Vert Ring------------------------------
	CryModelling.SelVertRing = fn SelVertRing =
	(
		try
		(
			with undo on
			(
				with redraw off
				(
					VertSel = polyop.GetVertSelection $
					UserEdgs = polyop.GetEdgeSelection $
					Edgs = #{}
					FEdgs = #{}
					for v in VertSel do
					(
						Vedgs = polyop.GetEdgesUsingVert $ v
						if Edgs.numberset != 0 then
						(
							Inter = Edgs * Vedgs
							if Inter.numberset != 0 then
							(
								FEdgs = FEdgs + Inter
							)
							else
							(
								Edgs = Edgs + Vedgs
							)
						)
						else
						(
							Edgs = Edgs + Vedgs
						)
					)
					polyop.SetEdgeSelection $ FEdgs
					SelLimERing()
					ESel = polyop.GetEdgeSelection $
					FVerts = #{}
					for edg in ESel do
					(
						FVerts = FVerts + (polyop.GetVertsUsingEdge $ edg)
					)
					polyop.setEdgeSelection $ UserEdgs
					polyop.setVertSelection $ FVerts
				)
				redrawviews()
			)
		)
		catch (print "The tool works only on EditablePoly Objects.")
	)

	-------------------------Limited Poly Loop------------------------------
	CryModelling.IsQuadPoly = fn IsQuadPoly =
	(
		Arr = #()
		for x in $.selectedfaces do
		(
			a = (polyop.getFaceEdges $ x.index) as array
			if a.count == 4 then
			(
				append Arr x
			)
		)
		if Arr.count == $.selectedfaces.count then true else false
	)

	CryModelling.SelPolyLoop = fn SelPolyLoop = 
	(
		try
		(
			if CryModelling.IsQuadPoly() == true then
			(
				EdgeCount = #{}
				for x in $.selectedFaces do
				(
					a = (polyop.GetFaceEdges $ x.index) as array
					for x in a do
					(
						append EdgeCount x
					)
				)
				if (EdgeCount as array).count == ($.selectedFaces.count * 4) then
				(
					if $.selectedFaces.count == 1 then
					(
						with undo on
						(
							with redraw off
							(
								Sel = #{}
								dg = polyop.SetEdgeSelection  $ ((polyop.GetFaceEdges $ $.selectedFaces[1].index) as array)
								$.buttonOp #selectEdgeRing
								for x in $.selectedEdges do
								(
									a = (polyop.getEdgeFaces $ x.index) as array
									for x in a do
									(
										if  ((polyop.getFaceEdges $ x)as array).count == 4 then
										(
											append Sel x
										)
									)
								)
								polyop.SetFaceSelection $ Sel
							)
							redrawviews()
						)
					)
					else
					(
						with undo on
						(
							with redraw off
							(
								Face1 = $.selectedFaces[1].index
								Face2 = $.selectedFaces[2].index
								Edgs1 = polyop.GetEdgesUsingFace $ Face1
								Edgs2 = polyop.GetEdgesUsingFace $ Face2
								UserSel = polyop.getedgeselection $
								polyop.setEdgeSelection $ Edgs1
								$.ButtonOp #SelectEdgeRing
								NSel = (polyop.GetEdgeSelection $) - Edgs1
								CrosSel = NSel * Edgs2
								if CrosSel.numberset != 0 then
								(
									polyop.setEdgeSelection $ CrosSel
									$.ButtonOp #SelectEdgeRing
									NSel = (polyop.GetEdgeSelection $) - Edgs2
									CrosSel2 = NSel * Edgs1
									FinalEdgs = #{}
									for i = 1 to 2 do
									(
										SelEdg = polyop.setEdgeSelection $ #((CrosSel as array)[i], (CrosSel2 as array)[i])
										CryModelling.SelLimERing ()
										FinalEdgs = FinalEdgs + (polyop.getedgeselection $)
									)
									polyop.setEdgeSelection $ UserSel
									FinalFaces = #{}
									for x in FinalEdgs do
									(
										Faces = polyop.getFacesUsingEdge $ x
										for f in Faces do
										(
											if ((polyop.GetEdgesUsingFace $ f)*FinalEdgs).numberset == 2 then
											(
												append FinalFaces f
											)
										)
									)
									polyop.setFaceSelection $ FinalFaces
								)
								redrawViews()
							)
						)
					)
				)
				else
				(
					if $.selectedFaces.count == 2 then
					(
						with undo on
						(
							with redraw off
							(
								Ar1 = #()
								for x in $.selectedFaces do
								(
									Edg = polyop.getEdgesUsingFace $ x.index
									append Ar1 Edg
								)
								FEdg = (Ar1[1]*Ar1[2])
								polyop.setedgeselection $ FEdg
								$.buttonOp #selectEdgeRing
								Sel = #{}
								for x in $.selectededges do
								(
									a = (polyop.getEdgeFaces $ x.index) as array
									for x in a do
									(
										if  ((polyop.getFaceEdges $ x)as array).count == 4 then
										(
											append Sel x
										)
									)
								)
								polyop.SetFaceSelection $ Sel
							)
							redrawviews ()
						)
					)
				)
			)
		)
		catch (print "The tool works only on EditablePoly Objects.")
	)