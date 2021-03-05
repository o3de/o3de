/**
 * overrides and additional functionality for highlight.js
 */

$(document).ready(function() {

    // apply highlighting to all <pre><code>..</code></pre> blocks on a page
    if (typeof hljs !== "undefined") {
        hljs.initHighlighting();
    }
    
    // remove css classes from nested code elements, to prevent funky styling
    $('em.replaceable code, em.replaceable code span, a.xref code').removeClass();
    $('code.nohighlight code, code.nohighlight span').removeClass();

});