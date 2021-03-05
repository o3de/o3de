var AWSDocs = AWSDocs || {};

AWSDocs.feedbackOnload = function () {
    var myPage = top.document.location.search.substr(1);
    if (myPage.length > 0) {
        var docfile = myPage.match(/topic_url=([^=\;\"#\?\s]+)/);
        if (docfile.length == 2) {
            var fblink = document.getElementById("fblink");
            fblink.href = fblink.href + "&topic_url=" + docfile[1];
        }
    }
};
