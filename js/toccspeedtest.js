(function ($) {  
    Drupal.behaviors.toccspeedtest = {
        attach: function (context, settings) {
            $('#toccspeedtest_poll_result', context).once('toccspeedtest', function () {
                toccspeedtest_poll(
                    $('input[name=testid]', context).val(),
                    context
                );
            });
        }
    };
    
    function toccspeedtest_poll(id, context) {
        $.getJSON(Drupal.settings.basePath + 'tocc/speedtest/gtmetrix/poll/' + id, function(res) {
            if (res.status === 'queued' || res.status === 'started') {
                $('#toccspeedtest_poll_result', context).html(
                    $('#toccspeedtest_poll_result', context).html() + '.'
                );
        
                setTimeout(function() {
                    toccspeedtest_poll(id, context);
                }, 2000);
            } else if (res.status === 'completed') {
                window.location = 'results';
            } else {
                window.location.reload();
            }
        });
    }
})(jQuery);