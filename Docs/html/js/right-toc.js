/**
 * interactivity for the right, page-level ToC
 */

var AWSDocs = AWSDocs || {};

AWSDocs.rightNavLinks;
AWSDocs.sections;

$(document).ready(function () {

    /*
     * When a fragment target is selected in the right nav, clear style for the previous
     * selection and apply style for the new selection. The style highlights
     * the selected link.
     */

    $("a.pagetoc").click(function () {

        $("a.pagetoc.selected").removeClass("selected");
        $(this).addClass("selected");

        var currentHref = $(this).attr( "href" );
        var hashUrl = window.location.hash.replace("#", "");

        if(currentHref == ("#" + hashUrl)) {

            AWSDocs.adjustFragmentUrl();
        }
    });

    AWSDocs.rightNavLinks = $("a.pagetoc");
    AWSDocs.sections = $("h2");

});

AWSDocs.isElementInViewport = function (element) {
    var rect = element.getBoundingClientRect();

    return (
        rect.top >= 0 &&                      // see if the top of the element is below the top of the viewport
        rect.bottom <= $(window).height()     // see if the bottom of the element is above the bottom of the viewport
    );
}

$(window).on('DOMContentLoaded load resize scroll', function () {
    if (AWSDocs.sections) {
        var numSections = AWSDocs.sections.length; // get total number of H2 sections on the page
        for (var i = 0; i < numSections; i++) {
            var sec = AWSDocs.sections[i];
            var docBottom = $(document).height() - $(window).height() - $(window).scrollTop();

            if (docBottom == 0) {                                                 // if we're at the bottom of the document
                $("a.pagetoc.selected").removeClass("selected");                  // remove "selected" from all pagetoc links
                $( AWSDocs.rightNavLinks[numSections - 1] ).addClass("selected"); // and add it to the LAST pagetoc link
            }

            else if (AWSDocs.isElementInViewport(sec)) {
                var sectionId = $(sec).attr("id");

                for (var j = 0; j < AWSDocs.rightNavLinks.length; j++) {
                    var rightNavLinkTarget = $(AWSDocs.rightNavLinks[j]).attr("href").substr(1);

                    if (rightNavLinkTarget == sectionId) {
                        $(AWSDocs.rightNavLinks).removeClass("selected");
                        $(AWSDocs.rightNavLinks[j]).addClass("selected");
                        return;
                    }
                }

            }
        }
    }
});



