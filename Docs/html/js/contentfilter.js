/**
 * functionality for language filtering
 */

var AWSDocs = AWSDocs || {};

AWSDocs.hasSectionMatch = function (selected) {  // check to see if the cookie matches a filterable section on the page

    var filterableEls = $(".langfilter");

    for (var l = 0; l < filterableEls.length; l++) {

        var filterableSection = filterableEls[l];
        var filterableSectionName = $(filterableSection).attr("name");
        if (filterableSectionName == selected) {
            return true;
        }
    }
    return false;
}

AWSDocs.filterContent = function (selected) {

    var filterableEls = $(".langfilter");
    var rightTocLinks = $("li.pagetoc");

    var hasSectionMatch = (function () {
        for (var l = 0; l < filterableEls.length; l++) {

            var filterableSection = filterableEls[l];
            var filterableSectionName = $(filterableSection).attr("name");
            if (filterableSectionName == selected) {
                return true;
            }
        }
        return false;
    })();

    if (selected == "All") {  // if the selected filter is "All", show all sections
        filterableEls.show();
        rightTocLinks.show();
    }

    else if ((selected !== "All") && (hasSectionMatch === false)) { // if the selected filter is not "All" and does NOT match a filterable section, show all sections

        filterableEls.show();
        rightTocLinks.show();
    }

    else { // if the selected filter is not "All" and DOES match a filterable section, show the section

        for (var i = 0; i < filterableEls.length; i++) {

            var filterableSection = filterableEls[i];
            var filterableSectionName = $(filterableSection).attr("name");

            if (filterableSectionName == selected) {              // if a section is selected ...
                $(filterableSection).show();                    // show the section ...
                if (rightTocLinks.length > 0) {
                    for (var j = 0; j < rightTocLinks.length; j++) {

                        if ($(rightTocLinks[j]).attr("name") == filterableSectionName) {
                            $(rightTocLinks[j]).show();         // and show the corresponding right ToC item
                        }
                    }
                }
            }

            else {                                                // if a section is not selected ...
                $(filterableSection).hide();                    // hide the section ...

                if (rightTocLinks.length > 0) {

                    for (var j = 0; j < rightTocLinks.length; j++) {

                        if ($(rightTocLinks[j]).attr("name") == filterableSectionName) {
                            $(rightTocLinks[j]).hide();        // and hide the corresponding right ToC item
                        }
                    }
                }
            }

        }
    }
}

AWSDocs.setFilterCookie = function (cookieVal) {

    document.cookie = "awsdocs_content_filter=" + encodeURIComponent(cookieVal);

}

AWSDocs.getFilterCookie = function () {

    var awsDocsContentFilterCookieKey = "awsdocs_content_filter=";
    var cookieArray = document.cookie.split(';');
    for (var i = 0; i < cookieArray.length; i++) {
        var cookie = cookieArray[i];
        while (cookie.charAt(0) == ' ') {                       // get rid of spaces before the cookie value
            cookie = cookie.substring(1, cookie.length);
        }
        if (cookie.indexOf(awsDocsContentFilterCookieKey) == 0) {
            var cookieVal = decodeURIComponent(cookie.substring(awsDocsContentFilterCookieKey.length, cookie.length));

            return cookieVal;                            // return the cookie value, or null if no cookie is set
        }
    }
    return null;

}

AWSDocs.setupFilter = function () {

    var cookieVal = AWSDocs.getFilterCookie();
    if (cookieVal) {

        if (AWSDocs.hasSectionMatch(cookieVal)) {
            $("#filter-select").val(cookieVal);
        } else {
            $("#filter-select").val("All");
        }

        AWSDocs.filterContent(cookieVal);               // if a cookie is available, filter with that value on page load

    } else {

        $("#filter-select").val("All");
    }

    $("#filter-select").change(function () {

        var selected = $(this).val();
        AWSDocs.setFilterCookie(selected);
        AWSDocs.filterContent(selected);                // filter again whenever an item is selected from the filter menu

    });
}

$(document).ready(function () {

    var filterableEls = $(".langfilter");
    if (filterableEls.length > 0) {                     // check if there are filterable sections on the page ...

        $("#language-filter").show();                 // if so, show the filter menu and set up the filter
        AWSDocs.setupFilter();

    }
});