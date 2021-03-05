var Layout = function(table){
	this.table = table;
	this.mode = null;
};

//initialize layout system
Layout.prototype.initialize = function(layout){

	if(this.modes[layout]){
		this.mode = layout;
	}else{
		console.warn("Layout Error - invalid mode set, defaulting to 'fitData' : " + layout);
		this.mode = 'fitData';
	}

	this.table.element.setAttribute("tabulator-layout", this.mode);
};

Layout.prototype.getMode = function(){
	return this.mode;
};

//trigger table layout
Layout.prototype.layout = function(){
	this.modes[this.mode].call(this, this.table.columnManager.columnsByIndex);
};

//layout render functions
Layout.prototype.modes = {

	//resize columns to fit data the contain
	"fitData": function(columns){
		columns.forEach(function(column){
			column.reinitializeWidth();
		});

		if(this.table.options.responsiveLayout && this.table.modExists("responsiveLayout", true)){
			this.table.modules.responsiveLayout.update();
		}
	},

	//resize columns to fit data the contain
	"fitDataFill": function(columns){
		columns.forEach(function(column){
			column.reinitializeWidth();
		});

		if(this.table.options.responsiveLayout && this.table.modExists("responsiveLayout", true)){
			this.table.modules.responsiveLayout.update();
		}
	},

	//resize columns to fit
	"fitColumns": function(columns){
		var self = this;

		var totalWidth = self.table.element.clientWidth; //table element width
		var fixedWidth = 0; //total width of columns with a defined width
		var flexWidth = 0; //total width available to flexible columns
		var flexGrowUnits = 0; //total number of widthGrow blocks accross all columns
		var flexColWidth = 0; //desired width of flexible columns
		var flexColumns = []; //array of flexible width columns
		var fixedShrinkColumns = []; //array of fixed width columns that can shrink
		var flexShrinkUnits = 0; //total number of widthShrink blocks accross all columns
		var overflowWidth = 0; //horizontal overflow width
		var gapFill=0; //number of pixels to be added to final column to close and half pixel gaps

		function calcWidth(width){
			var colWidth;

			if(typeof(width) == "string"){
				if(width.indexOf("%") > -1){
					colWidth = (totalWidth / 100) * parseInt(width);
				}else{
					colWidth = parseInt(width);
				}
			}else{
				colWidth = width;
			}

			return colWidth;
		}

		//ensure columns resize to take up the correct amount of space
		function scaleColumns(columns, freeSpace, colWidth, shrinkCols){

			var oversizeCols = [],
			oversizeSpace = 0,
			remainingSpace = 0,
			nextColWidth = 0,
			gap = 0,
			changeUnits = 0,
			undersizeCols = [];

			function calcGrow(col){
				return (colWidth * (col.column.definition.widthGrow || 1));
			}

			function calcShrink(col){
				return  (calcWidth(col.width) - (colWidth * (col.column.definition.widthShrink || 0)))
			}

			columns.forEach(function(col, i){
				var width = shrinkCols ? calcShrink(col) : calcGrow(col);
				if(col.column.minWidth >= width){
					oversizeCols.push(col);
				}else{
					undersizeCols.push(col);
					changeUnits += shrinkCols ? (col.column.definition.widthShrink || 1) : (col.column.definition.widthGrow || 1);
				}
			});

			if(oversizeCols.length){
				oversizeCols.forEach(function(col){
					oversizeSpace += shrinkCols ?  col.width - col.column.minWidth : col.column.minWidth;
					col.width = col.column.minWidth;
				});

				remainingSpace = freeSpace - oversizeSpace;

				nextColWidth = changeUnits ? Math.floor(remainingSpace/changeUnits) : remainingSpace;

				gap = remainingSpace - (nextColWidth * changeUnits);

				gap += scaleColumns(undersizeCols, remainingSpace, nextColWidth, shrinkCols);
			}else{
				gap = changeUnits ? freeSpace - (Math.floor(freeSpace/changeUnits) * changeUnits) : freeSpace;

				undersizeCols.forEach(function(column){
					column.width = shrinkCols ? calcShrink(column) : calcGrow(column);
				});
			}

			return gap;
		}


		if(this.table.options.responsiveLayout && this.table.modExists("responsiveLayout", true)){
			this.table.modules.responsiveLayout.update();
		}

		//adjust for vertical scrollbar if present
		if(this.table.rowManager.element.scrollHeight > this.table.rowManager.element.clientHeight){
			totalWidth -= this.table.rowManager.element.offsetWidth - this.table.rowManager.element.clientWidth;
		}

		columns.forEach(function(column){
			var width, minWidth, colWidth;

			if(column.visible){

				width = column.definition.width;
				minWidth =  parseInt(column.minWidth);

				if(width){

					colWidth = calcWidth(width);

					fixedWidth += colWidth > minWidth ? colWidth : minWidth;

					if(column.definition.widthShrink){
						fixedShrinkColumns.push({
							column:column,
							width:colWidth > minWidth ? colWidth : minWidth
						});
						flexShrinkUnits += column.definition.widthShrink;
					}

				}else{
					flexColumns.push({
						column:column,
						width:0,
					});
					flexGrowUnits += column.definition.widthGrow || 1;
				}
			}
		});


		//calculate available space
		flexWidth = totalWidth - fixedWidth;

		//calculate correct column size
		flexColWidth = Math.floor(flexWidth / flexGrowUnits)

		//generate column widths
		var gapFill = scaleColumns(flexColumns, flexWidth, flexColWidth, false);

		//increase width of last column to account for rounding errors
		if(flexColumns.length && gapFill > 0){
			flexColumns[flexColumns.length-1].width += + gapFill;
		}

		//caculate space for columns to be shrunk into
		flexColumns.forEach(function(col){
			flexWidth -= col.width;
		})

		overflowWidth = Math.abs(gapFill) + flexWidth;


		//shrink oversize columns if there is no available space
		if(overflowWidth > 0 && flexShrinkUnits){
			gapFill = scaleColumns(fixedShrinkColumns, overflowWidth, Math.floor(overflowWidth / flexShrinkUnits), true);
		}

		//decrease width of last column to account for rounding errors
		if(fixedShrinkColumns.length){
			fixedShrinkColumns[fixedShrinkColumns.length-1].width -= gapFill;
		}


		flexColumns.forEach(function(col){
			col.column.setWidth(col.width);
		});

		fixedShrinkColumns.forEach(function(col){
			col.column.setWidth(col.width);
		});

	},
};


Tabulator.prototype.registerModule("layout", Layout);