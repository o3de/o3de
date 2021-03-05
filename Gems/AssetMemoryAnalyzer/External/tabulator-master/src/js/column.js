
//public column object
var ColumnComponent = function (column){
	this._column = column;
	this.type = "ColumnComponent";
};

ColumnComponent.prototype.getElement = function(){
	return this._column.getElement();
};

ColumnComponent.prototype.getDefinition = function(){
	return this._column.getDefinition();
};

ColumnComponent.prototype.getField = function(){
	return this._column.getField();
};

ColumnComponent.prototype.getCells = function(){
	var cells = [];

	this._column.cells.forEach(function(cell){
		cells.push(cell.getComponent());
	});

	return cells;
};

ColumnComponent.prototype.getVisibility = function(){
	return this._column.visible;
};

ColumnComponent.prototype.show = function(){
	if(this._column.isGroup){
		this._column.columns.forEach(function(column){
			column.show();
		});
	}else{
		this._column.show();
	}
};

ColumnComponent.prototype.hide = function(){
	if(this._column.isGroup){
		this._column.columns.forEach(function(column){
			column.hide();
		});
	}else{
		this._column.hide();
	}
};

ColumnComponent.prototype.toggle = function(){
	if(this._column.visible){
		this.hide();
	}else{
		this.show();
	}
};

ColumnComponent.prototype.delete = function(){
	this._column.delete();
};

ColumnComponent.prototype.getSubColumns = function(){
	var output = [];

	if(this._column.columns.length){
		this._column.columns.forEach(function(column){
			output.push(column.getComponent());
		});
	}

	return output;
};

ColumnComponent.prototype.getParentColumn = function(){
	return this._column.parent instanceof Column ? this._column.parent.getComponent() : false;
};


ColumnComponent.prototype._getSelf = function(){
	return this._column;
};

ColumnComponent.prototype.scrollTo = function(){
	return this._column.table.columnManager.scrollToColumn(this._column);
};

ColumnComponent.prototype.getTable = function(){
	return this._column.table;
};

ColumnComponent.prototype.headerFilterFocus = function(){
	if(this._column.table.modExists("filter", true)){
		this._column.table.modules.filter.setHeaderFilterFocus(this._column);
	}
};

ColumnComponent.prototype.reloadHeaderFilter = function(){
	if(this._column.table.modExists("filter", true)){
		this._column.table.modules.filter.reloadHeaderFilter(this._column);
	}
};

ColumnComponent.prototype.setHeaderFilterValue = function(value){
	if(this._column.table.modExists("filter", true)){
		this._column.table.modules.filter.setHeaderFilterValue(this._column, value);
	}
};



var Column = function(def, parent){
	var self = this;

	this.table = parent.table;
	this.definition = def; //column definition
	this.parent = parent; //hold parent object
	this.type = "column"; //type of element
	this.columns = []; //child columns
	this.cells = []; //cells bound to this column
	this.element = this.createElement(); //column header element
	this.contentElement = false;
	this.groupElement = this.createGroupElement(); //column group holder element
	this.isGroup = false;
	this.tooltip = false; //hold column tooltip
	this.hozAlign = ""; //horizontal text alignment

	//multi dimentional filed handling
	this.field ="";
	this.fieldStructure = "";
	this.getFieldValue = "";
	this.setFieldValue = "";

	this.setField(this.definition.field);


	this.modules = {}; //hold module variables;

	this.cellEvents = {
		cellClick:false,
		cellDblClick:false,
		cellContext:false,
		cellTap:false,
		cellDblTap:false,
		cellTapHold:false
	};

	this.width = null; //column width
	this.minWidth = null; //column minimum width
	this.widthFixed = false; //user has specified a width for this column

	this.visible = true; //default visible state

	//initialize column
	if(def.columns){

		this.isGroup = true;

		def.columns.forEach(function(def, i){
			var newCol = new Column(def, self);
			self.attachColumn(newCol);
		});

		self.checkColumnVisibility();
	}else{
		parent.registerColumnField(this);
	}

	if(def.rowHandle && this.table.options.movableRows !== false && this.table.modExists("moveRow")){
		this.table.modules.moveRow.setHandle(true);
	}

	this._buildHeader();
};

Column.prototype.createElement = function (){
	var el = document.createElement("div");

	el.classList.add("tabulator-col");
	el.setAttribute("role", "columnheader");
	el.setAttribute("aria-sort", "none");

	return el;
};

Column.prototype.createGroupElement = function (){
	var el = document.createElement("div");

	el.classList.add("tabulator-col-group-cols");

	return el;
};

Column.prototype.setField = function(field){
	this.field = field;
	this.fieldStructure = field ? (this.table.options.nestedFieldSeparator ? field.split(this.table.options.nestedFieldSeparator) : [field]) : [];
	this.getFieldValue = this.fieldStructure.length > 1 ? this._getNestedData : this._getFlatData;
	this.setFieldValue = this.fieldStructure.length > 1 ? this._setNesteData : this._setFlatData;
};

//register column position with column manager
Column.prototype.registerColumnPosition = function(column){
	this.parent.registerColumnPosition(column);
};

//register column position with column manager
Column.prototype.registerColumnField = function(column){
	this.parent.registerColumnField(column);
};

//trigger position registration
Column.prototype.reRegisterPosition = function(){
	if(this.isGroup){
		this.columns.forEach(function(column){
			column.reRegisterPosition();
		});
	}else{
		this.registerColumnPosition(this);
	}
};

Column.prototype.setTooltip = function(){
	var self = this,
	def = self.definition;

	//set header tooltips
	var tooltip = def.headerTooltip || def.tooltip === false  ? def.headerTooltip : self.table.options.tooltipsHeader;

	if(tooltip){
		if(tooltip === true){
			if(def.field){
				self.table.modules.localize.bind("columns|" + def.field, function(value){
					self.element.setAttribute("title", value || def.title);
				});
			}else{
				self.element.setAttribute("title", def.title);
			}

		}else{
			if(typeof(tooltip) == "function"){
				tooltip = tooltip(self.getComponent());

				if(tooltip === false){
					tooltip = "";
				}
			}

			self.element.setAttribute("title", tooltip);
		}

	}else{
		self.element.setAttribute("title", "");
	}
};

//build header element
Column.prototype._buildHeader = function(){
	var self = this,
	def = self.definition;

	while(self.element.firstChild) self.element.removeChild(self.element.firstChild);

	if(def.headerVertical){
		self.element.classList.add("tabulator-col-vertical");

		if(def.headerVertical === "flip"){
			self.element.classList.add("tabulator-col-vertical-flip");
		}
	}

	self.contentElement = self._bindEvents();

	self.contentElement = self._buildColumnHeaderContent();

	self.element.appendChild(self.contentElement);

	if(self.isGroup){
		self._buildGroupHeader();
	}else{
		self._buildColumnHeader();
	}

	self.setTooltip();

	//set resizable handles
	if(self.table.options.resizableColumns && self.table.modExists("resizeColumns")){
		self.table.modules.resizeColumns.initializeColumn("header", self, self.element);
	}

	//set resizable handles
	if(def.headerFilter && self.table.modExists("filter") && self.table.modExists("edit")){
		if(typeof def.headerFilterPlaceholder !== "undefined" && def.field){
			self.table.modules.localize.setHeaderFilterColumnPlaceholder(def.field, def.headerFilterPlaceholder);
		}

		self.table.modules.filter.initializeColumn(self);
	}


	//set resizable handles
	if(self.table.modExists("frozenColumns")){
		self.table.modules.frozenColumns.initializeColumn(self);
	}

	//set movable column
	if(self.table.options.movableColumns && !self.isGroup && self.table.modExists("moveColumn")){
		self.table.modules.moveColumn.initializeColumn(self);
	}

	//set calcs column
	if((def.topCalc || def.bottomCalc) && self.table.modExists("columnCalcs")){
		self.table.modules.columnCalcs.initializeColumn(self);
	}


	//update header tooltip on mouse enter
	self.element.addEventListener("mouseenter", function(e){
		self.setTooltip();
	});
};

Column.prototype._bindEvents = function(){

	var self = this,
	def = self.definition,
	dblTap,	tapHold, tap;

	//setup header click event bindings
	if(typeof(def.headerClick) == "function"){
		self.element.addEventListener("click", function(e){def.headerClick(e, self.getComponent());});
	}

	if(typeof(def.headerDblClick) == "function"){
		self.element.addEventListener("dblclick", function(e){def.headerDblClick(e, self.getComponent());});
	}

	if(typeof(def.headerContext) == "function"){
		self.element.addEventListener("contextmenu", function(e){def.headerContext(e, self.getComponent());});
	}

	//setup header tap event bindings
	if(typeof(def.headerTap) == "function"){
		tap = false;

		self.element.addEventListener("touchstart", function(e){
			tap = true;
		});

		self.element.addEventListener("touchend", function(e){
			if(tap){
				def.headerTap(e, self.getComponent());
			}

			tap = false;
		});
	}

	if(typeof(def.headerDblTap) == "function"){
		dblTap = null;

		self.element.addEventListener("touchend", function(e){

			if(dblTap){
				clearTimeout(dblTap);
				dblTap = null;

				def.headerDblTap(e, self.getComponent());
			}else{

				dblTap = setTimeout(function(){
					clearTimeout(dblTap);
					dblTap = null;
				}, 300);
			}

		});
	}

	if(typeof(def.headerTapHold) == "function"){
		tapHold = null;

		self.element.addEventListener("touchstart", function(e){
			clearTimeout(tapHold);

			tapHold = setTimeout(function(){
				clearTimeout(tapHold);
				tapHold = null;
				tap = false;
				def.headerTapHold(e, self.getComponent());
			}, 1000);

		});

		self.element.addEventListener("touchend", function(e){
			clearTimeout(tapHold);
			tapHold = null;
		});
	}

	//store column cell click event bindings
	if(typeof(def.cellClick) == "function"){
		self.cellEvents.cellClick = def.cellClick;
	}

	if(typeof(def.cellDblClick) == "function"){
		self.cellEvents.cellDblClick = def.cellDblClick;
	}

	if(typeof(def.cellContext) == "function"){
		self.cellEvents.cellContext = def.cellContext;
	}

	//setup column cell tap event bindings
	if(typeof(def.cellTap) == "function"){
		self.cellEvents.cellTap = def.cellTap;
	}

	if(typeof(def.cellDblTap) == "function"){
		self.cellEvents.cellDblTap = def.cellDblTap;
	}

	if(typeof(def.cellTapHold) == "function"){
		self.cellEvents.cellTapHold = def.cellTapHold;
	}

	//setup column cell edit callbacks
	if(typeof(def.cellEdited) == "function"){
		self.cellEvents.cellEdited = def.cellEdited;
	}

	if(typeof(def.cellEditing) == "function"){
		self.cellEvents.cellEditing = def.cellEditing;
	}

	if(typeof(def.cellEditCancelled) == "function"){
		self.cellEvents.cellEditCancelled = def.cellEditCancelled;
	}
};

//build header element for header
Column.prototype._buildColumnHeader = function(){
	var self = this,
	def = self.definition,
	table = self.table,
	sortable;

	//set column sorter
	if(table.modExists("sort")){
		table.modules.sort.initializeColumn(self, self.contentElement);
	}

	//set column formatter
	if(table.modExists("format")){
		table.modules.format.initializeColumn(self);
	}

	//set column editor
	if(typeof def.editor != "undefined" && table.modExists("edit")){
		table.modules.edit.initializeColumn(self);
	}

	//set colum validator
	if(typeof def.validator != "undefined" && table.modExists("validate")){
		table.modules.validate.initializeColumn(self);
	}


	//set column mutator
	if(table.modExists("mutator")){
		table.modules.mutator.initializeColumn(self);
	}

	//set column accessor
	if(table.modExists("accessor")){
		table.modules.accessor.initializeColumn(self);
	}

	//set respoviveLayout
	if(typeof table.options.responsiveLayout && table.modExists("responsiveLayout")){
		table.modules.responsiveLayout.initializeColumn(self);
	}

	//set column visibility
	if(typeof def.visible != "undefined"){
		if(def.visible){
			self.show(true);
		}else{
			self.hide(true);
		}
	}

	//asign additional css classes to column header
	if(def.cssClass){
		self.element.classList.add(def.cssClass);
	}

	if(def.field){
		this.element.setAttribute("tabulator-field", def.field);
	}

	//set min width if present
	self.setMinWidth(typeof def.minWidth == "undefined" ? self.table.options.columnMinWidth : def.minWidth);

	self.reinitializeWidth();

	//set tooltip if present
	self.tooltip = self.definition.tooltip || self.definition.tooltip === false ? self.definition.tooltip : self.table.options.tooltips;

	//set orizontal text alignment
	self.hozAlign = typeof(self.definition.align) == "undefined" ? "" : self.definition.align;
};

Column.prototype._buildColumnHeaderContent = function(){
	var self = this,
	def = self.definition,
	table = self.table;

	var contentElement = document.createElement("div");
	contentElement.classList.add("tabulator-col-content");

	contentElement.appendChild(self._buildColumnHeaderTitle());

	return contentElement;
};

//build title element of column
Column.prototype._buildColumnHeaderTitle = function(){
	var self = this,
	def = self.definition,
	table = self.table,
	title;

	var titleHolderElement = document.createElement("div");
	titleHolderElement.classList.add("tabulator-col-title");

	if(def.editableTitle){
		var titleElement = document.createElement("input");
		titleElement.classList.add("tabulator-title-editor");

		titleElement.addEventListener("click", function(e){
			e.stopPropagation();
			titleElement.focus();
		});

		titleElement.addEventListener("change", function(){
			def.title = titleElement.value;
			table.options.columnTitleChanged.call(self.table, self.getComponent());
		});

		titleHolderElement.appendChild(titleElement);

		if(def.field){
			table.modules.localize.bind("columns|" + def.field, function(text){
				titleElement.value = text || (def.title || "&nbsp");
			});
		}else{
			titleElement.value  = def.title || "&nbsp";
		}

	}else{
		if(def.field){
			table.modules.localize.bind("columns|" + def.field, function(text){
				self._formatColumnHeaderTitle(titleHolderElement, text || (def.title || "&nbsp"));
			});
		}else{
			self._formatColumnHeaderTitle(titleHolderElement, def.title || "&nbsp");
		}
	}

	return titleHolderElement;
};

Column.prototype._formatColumnHeaderTitle = function(el, title){
	var formatter, contents, params, mockCell;

	if(this.definition.titleFormatter && this.table.modExists("format")){

		formatter = this.table.modules.format.getFormatter(this.definition.titleFormatter);

		mockCell = {
			getValue:function(){
				return title;
			},
			getElement:function(){
				return el;
			}
		};

		params = this.definition.titleFormatterParams || {};

		params = typeof params === "function" ? params() : params;

		contents = formatter.call(this.table.modules.format, mockCell, params);

		switch(typeof contents){
			case "object":
			if(contents instanceof Node){
				this.element.appendChild(contents);
			}else{
				this.element.innerHTML = "";
				console.warn("Format Error - Title formatter has returned a type of object, the only valid formatter object return is an instance of Node, the formatter returned:", contents);
			}
			break;
			case "undefined":
			case "null":
			this.element.innerHTML = "";
			break;
			default:
			this.element.innerHTML = contents;
		}
	}else{
		el.innerHTML = title;
	}
};


//build header element for column group
Column.prototype._buildGroupHeader = function(){
	this.element.classList.add("tabulator-col-group");
	this.element.setAttribute("role", "columngroup");
	this.element.setAttribute("aria-title", this.definition.title);

	this.element.appendChild(this.groupElement);
};

//flat field lookup
Column.prototype._getFlatData = function(data){
	return data[this.field];
};

//nested field lookup
Column.prototype._getNestedData = function(data){
	var dataObj = data,
	structure = this.fieldStructure,
	length = structure.length,
	output;

	for(let i = 0; i < length; i++){

		dataObj = dataObj[structure[i]];

		output = dataObj;

		if(!dataObj){
			break;
		}
	}

	return output;
};

//flat field set
Column.prototype._setFlatData = function(data, value){
	data[this.field] = value;
};

//nested field set
Column.prototype._setNesteData = function(data, value){
	var dataObj = data,
	structure = this.fieldStructure,
	length = structure.length;

	for(let i = 0; i < length; i++){

		if(i == length -1){
			dataObj[structure[i]] = value;
		}else{
			if(!dataObj[structure[i]]){
				dataObj[structure[i]] = {};
			}

			dataObj = dataObj[structure[i]];
		}
	}
};


//attach column to this group
Column.prototype.attachColumn = function(column){
	var self = this;

	if(self.groupElement){
		self.columns.push(column);
		self.groupElement.appendChild(column.getElement());
	}else{
		console.warn("Column Warning - Column being attached to another column instead of column group");
	}
};

//vertically align header in column
Column.prototype.verticalAlign = function(alignment, height){

	//calculate height of column header and group holder element
	var parentHeight = this.parent.isGroup ? this.parent.getGroupElement().clientHeight : (height || this.parent.getHeadersElement().clientHeight);
	// var parentHeight = this.parent.isGroup ? this.parent.getGroupElement().clientHeight : this.parent.getHeadersElement().clientHeight;

	this.element.style.height = parentHeight + "px";

	if(this.isGroup){
		this.groupElement.style.minHeight = (parentHeight - this.contentElement.offsetHeight) + "px";
	}

	//vertically align cell contents
	if(!this.isGroup && alignment !== "top"){
		if(alignment === "bottom"){
			this.element.style.paddingTop = (this.element.clientHeight - this.contentElement.offsetHeight) + "px";
		}else{
			this.element.style.paddingTop = ((this.element.clientHeight - this.contentElement.offsetHeight) / 2) + "px";
		}
	}

	this.columns.forEach(function(column){
		column.verticalAlign(alignment);
	});
};

//clear vertical alignmenet
Column.prototype.clearVerticalAlign = function(){
	this.element.style.paddingTop = "";
	this.element.style.height = "";
	this.element.style.minHeight = "";

	this.columns.forEach(function(column){
		column.clearVerticalAlign();
	});
};

//// Retreive Column Information ////

//return column header element
Column.prototype.getElement = function(){
	return this.element;
};

//return colunm group element
Column.prototype.getGroupElement = function(){
	return this.groupElement;
};

//return field name
Column.prototype.getField = function(){
	return this.field;
};

//return the first column in a group
Column.prototype.getFirstColumn = function(){
	if(!this.isGroup){
		return this;
	}else{
		if(this.columns.length){
			return this.columns[0].getFirstColumn();
		}else{
			return false;
		}
	}
};

//return the last column in a group
Column.prototype.getLastColumn = function(){
	if(!this.isGroup){
		return this;
	}else{
		if(this.columns.length){
			return this.columns[this.columns.length -1].getLastColumn();
		}else{
			return false;
		}
	}
};

//return all columns in a group
Column.prototype.getColumns = function(){
	return this.columns;
};

//return all columns in a group
Column.prototype.getCells = function(){
	return this.cells;
};

//retreive the top column in a group of columns
Column.prototype.getTopColumn = function(){
	if(this.parent.isGroup){
		return this.parent.getTopColumn();
	}else{
		return this;
	}
};

//return column definition object
Column.prototype.getDefinition = function(updateBranches){
	var colDefs = [];

	if(this.isGroup && updateBranches){
		this.columns.forEach(function(column){
			colDefs.push(column.getDefinition(true));
		});

		this.definition.columns = colDefs;
	}

	return this.definition;
};

//////////////////// Actions ////////////////////

Column.prototype.checkColumnVisibility = function(){
	var visible = false;

	this.columns.forEach(function(column){
		if(column.visible){
			visible = true;
		}
	});

	if(visible){
		this.show();
		this.parent.table.options.columnVisibilityChanged.call(this.table, this.getComponent(), false);
	}else{
		this.hide();
	}

};

//show column
Column.prototype.show = function(silent, responsiveToggle){
	if(!this.visible){
		this.visible = true;

		this.element.style.display = "";

		this.table.columnManager._verticalAlignHeaders();

		if(this.parent.isGroup){
			this.parent.checkColumnVisibility();
		}

		this.cells.forEach(function(cell){
			cell.show();
		});

		if(this.table.options.persistentLayout && this.table.modExists("responsiveLayout", true)){
			this.table.modules.persistence.save("columns");
		}

		if(!responsiveToggle && this.table.options.responsiveLayout && this.table.modExists("responsiveLayout", true)){
			this.table.modules.responsiveLayout.updateColumnVisibility(this, this.visible);
		}

		if(!silent){
			this.table.options.columnVisibilityChanged.call(this.table, this.getComponent(), true);
		}
	}
};

//hide column
Column.prototype.hide = function(silent, responsiveToggle){
	if(this.visible){
		this.visible = false;

		this.element.style.display = "none";

		this.table.columnManager._verticalAlignHeaders();

		if(this.parent.isGroup){
			this.parent.checkColumnVisibility();
		}

		this.cells.forEach(function(cell){
			cell.hide();
		});

		if(this.table.options.persistentLayout && this.table.modExists("persistence", true)){
			this.table.modules.persistence.save("columns");
		}

		if(!responsiveToggle && this.table.options.responsiveLayout && this.table.modExists("responsiveLayout", true)){
			this.table.modules.responsiveLayout.updateColumnVisibility(this, this.visible);
		}

		if(!silent){
			this.table.options.columnVisibilityChanged.call(this.table, this.getComponent(), false);
		}
	}
};

Column.prototype.matchChildWidths = function(){
	var childWidth = 0;

	if(this.contentElement && this.columns.length){
		this.columns.forEach(function(column){
			childWidth += column.getWidth();
		});

		this.contentElement.style.maxWidth = (childWidth - 1) + "px";
	}
};

Column.prototype.setWidth = function(width){
	this.widthFixed = true;
	this.setWidthActual(width);
};

Column.prototype.setWidthActual = function(width){

	if(isNaN(width)){
		width = Math.floor((this.table.element.clientWidth/100) * parseInt(width));
	}

	width = Math.max(this.minWidth, width);

	this.width = width;

	this.element.style.width = width ? width + "px" : "";

	if(!this.isGroup){
		this.cells.forEach(function(cell){
			cell.setWidth(width);
		});
	}

	if(this.parent.isGroup){
		this.parent.matchChildWidths();
	}

	//set resizable handles
	if(this.table.modExists("frozenColumns")){
		this.table.modules.frozenColumns.layout();
	}
};


Column.prototype.checkCellHeights = function(){
	var rows = [];

	this.cells.forEach(function(cell){
		if(cell.row.heightInitialized){
			if(cell.row.getElement().offsetParent !== null){
				rows.push(cell.row);
				cell.row.clearCellHeight();
			}else{
				cell.row.heightInitialized = false;
			}
		}
	});

	rows.forEach(function(row){
		row.calcHeight();
	});

	rows.forEach(function(row){
		row.setCellHeight();
	});
};

Column.prototype.getWidth = function(){
	// return this.element.offsetWidth;
	return this.width;
};

Column.prototype.getHeight = function(){
	return this.element.offsetHeight;
};

Column.prototype.setMinWidth = function(minWidth){
	this.minWidth = minWidth;

	this.element.style.minWidth = minWidth ? minWidth + "px" : "";

	this.cells.forEach(function(cell){
		cell.setMinWidth(minWidth);
	});
};

Column.prototype.delete = function(){
	if(this.isGroup){
		this.columns.forEach(function(column){
			column.delete();
		});
	}

	var cellCount = this.cells.length;

	for(let i = 0; i < cellCount; i++){
		this.cells[0].delete();
	}

	this.element.parentNode.removeChild(this.element);

	this.table.columnManager.deregisterColumn(this);
};

//////////////// Cell Management /////////////////

//generate cell for this column
Column.prototype.generateCell = function(row){
	var self = this;

	var cell = new Cell(self, row);

	this.cells.push(cell);

	return cell;
};

Column.prototype.reinitializeWidth = function(force){

	this.widthFixed = false;

	//set width if present
	if(typeof this.definition.width !== "undefined" && !force){
		this.setWidth(this.definition.width);
	}

	//hide header filters to prevent them altering column width
	if(this.table.modExists("filter")){
		this.table.modules.filter.hideHeaderFilterElements();
	}

	this.fitToData();

	//show header filters again after layout is complete
	if(this.table.modExists("filter")){
		this.table.modules.filter.showHeaderFilterElements();
	}
};

//set column width to maximum cell width
Column.prototype.fitToData = function(){
	var self = this;

	if(!this.widthFixed){
		this.element.width = "";

		self.cells.forEach(function(cell){
			cell.setWidth("");
		});
	}

	var maxWidth = this.element.offsetWidth;

	if(!self.width || !this.widthFixed){
		self.cells.forEach(function(cell){
			var width = cell.getWidth();

			if(width > maxWidth){
				maxWidth = width;
			}
		});

		if(maxWidth){
			self.setWidthActual(maxWidth + 1);
		}

	}
};

Column.prototype.deleteCell = function(cell){
	var index = this.cells.indexOf(cell);

	if(index > -1){
		this.cells.splice(index, 1);
	}
};

//////////////// Event Bindings /////////////////

//////////////// Object Generation /////////////////
Column.prototype.getComponent = function(){
	return new ColumnComponent(this);
};