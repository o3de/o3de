function doFilterOnLoad(){
	var filterValue = getFilterCookie();
	if(filterValue!=null){
		filterValue = getFilterCookie();		
		doFilter(filterValue);
		document.getElementById('filter-select').value = filterValue;
	}
}

function doFilter(value){
	var elements;
	setFilterCookie(value);
	var filterN = 0;
	var f=0;
	for (filterN = 0; filterN<filterNames.length; filterN++){
		var elements = document.getElementsByName(filterNames[filterN]);
		if(value == filterNames[filterN] || value == 'All'){
			for (f=0; f != elements.length; f++) {
				elements[f].style.display='inline';
			}
		}
		else{
			for (f=0; f != elements.length; f++) {
				elements[f].style.display='none';
			}
		}
	}
}

function setFilterCookie(value){
	document.cookie = "aws_lang_filter" + "=" + escape(value);
}

function getFilterCookie(){
	var dc = document.cookie;
	var prefix = "aws_lang_filter" + "=";
	var begin = dc.indexOf("; " + prefix);
	if (begin == -1) {
	   begin = dc.indexOf(prefix);
	if (begin != 0) 
	   return null;
	} else
	begin += 2;
	var end = document.cookie.indexOf(";", begin);
	if (end == -1)
	  end = dc.length;
	return unescape(dc.substring(begin + prefix.length, end));
}
