/**
 * general-purpose scripts that don't require a separate file
 */

var AWSDocs = AWSDocs || {
};

// adjusts position of the :target, so that the heading is not obscured by the fixed top nav
AWSDocs.adjustFragmentUrl = function () {
    
    var hashUrl = window.location.hash.replace("#", "");
    // remove # since not all browsers return it
    var sanitizedHash = AWSDocs.sanitizeHash(hashUrl);
    
    if (window.location.hash) {
        var matchingFragment = $("body").find("#" + sanitizedHash + ":first");
        
        if (matchingFragment.length) {
            // this should always be true, but we'll check for safety
            var scrollTo = $("#" + sanitizedHash).offset().top - 70;
            $("html, body").animate({
                scrollTop: scrollTo
            },
            "fast");
        }
    }
};

AWSDocs.sanitizeHash = function (hash) {
    
    return encodeURIComponent(hash).replace(/(:|\.|\[|\]|,)/g, "\\$1");
    // see http://learn.jquery.com/using-jquery-core/faq/how-do-i-select-an-element-by-an-id-that-has-characters-used-in-css-notation/
}


$(document).ready(function () {
    //determine if footer cookie set
    var footerShort = getCookie("aws-doc-footer-short");
    if (footerShort === '1') {
        if ($('#footer_short_fb').length) {
            $('#footer').addClass('shortFooter_ht');
        } else {
            $('#footer').addClass('shortFooter');
        }
        $('#footer_toggle_img').toggleClass("hide");
        $('#footer_toggle_img_collapse').toggleClass("hide");
        $('#footer_short_fb').toggleClass("hide");
    }
    
    //watch for click events to add the footer state for site catalyst tracking
    $("a[href]").click(function (event) {        
        var paramValue = "shortFooter=true";
        // if short footer is visible and href is in the doc domain =
        if ($('#footer_toggle_img.hide').length && this.href.indexOf(window.location.hostname.toString()) !== -1) {
            //if href doesn't already contain the shortfooter param, add it
            if (this.href.indexOf(paramValue) < 0) {
                // Add footer param to url
                var paramSeparator = this.href.indexOf("?") !== -1 ? "&": "?";
                var hash = this.hash;
                var url = this.href;
                url = url.replace(hash, '');
                
                this.href = url + paramSeparator + paramValue + hash;
            }
        } else {
            //shortfooter param should be removed if it's in the url 
            if (this.href.indexOf(paramValue) > 0 && this.href.indexOf("?") > 0) {
                //footer widened, remove shortFooter 
                var url = this.href;
                var hash = this.hash;
                url = url.replace(hash, '');
                var urlprefix = url.split("?")[0];
                var allParams = url.split("?")[1];
                                
                paramList = allParams.split("&");
                for (var i = paramList.length -1; i>=0; i--) {
                    if(paramList[i] === paramValue) {
                        paramList.splice(i, 1); //remove the short footer param 
                    }
                }
                if (paramList.length > 0){
                    this.href = urlprefix + "?" + paramList.join("&") + hash;
                }
                else {
                    this.href = urlprefix + hash;
                }                
            }
        }
    });
    
    // on click of the ellipsis (#toggle-contents), toggle the left ToC display from 'none' to 'block'
    $('#toggle-contents').click(function () {
        $('#toc').toggleClass("open");
    });
    
    $('#footer_toggle').click(function () {
        $('#footer_toggle_img').toggleClass("hide");
        $('#footer_toggle_img_collapse').toggleClass("hide");
        $('#footer_short_fb').toggleClass("hide");
        
        if ($('#footer_toggle_img.hide').length) {
            
            if ($('#footer_short_fb').length) {
                $('#footer').addClass('shortFooter_ht');
            } else {
                $('#footer').addClass('shortFooter');
            }
            SetCookie("aws-doc-footer-short", "1", 1);
        }
        if ($('#footer_toggle_img_collapse.hide').length) {
            
            if ($('#footer_short_fb').length) {
                $('#footer').removeClass('shortFooter_ht');
            } else {
                $('#footer').removeClass('shortFooter');
            }
            SetCookie("aws-doc-footer-short", "0", 1);
        }
    });
});