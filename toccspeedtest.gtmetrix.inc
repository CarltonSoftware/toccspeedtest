<?php

/**
 * Form
 *
 * @param array $form       Form object
 * @param array $form_state Form state
 *
 * @return void
 */
function toccspeedtest_gtmetrix_view_form($form, &$form_state)
{
    // Bool allowing tests to commence
    $startAllowed = true;
    
    // Provide a wrapper around the entire form, since we'll replace the whole
    // thing with each submit.
    $form['#prefix'] = '<div id="toccspeedtest_gtmetrix_view_form_wrapper">';
    $form['#suffix'] = '</div>';
    
    if (isset($form_state['rebuild']) && $form_state['rebuild'] == true) {
        
        $form['message'] = array(
            '#markup' => '<div class="messages status" id="toccspeedtest_poll_result">Starting test, please wait.</div>'
        );
        
        try {
            $form['testid'] = array(
                '#type' => 'hidden',
                '#value' => toccspeedtest_start_gtmetrix_test()
            );
            $path = drupal_get_path('module', 'toccspeedtest');
            $form['#attached']['css'] = array();
            $form['#attached']['js'] = array();
            $form['#attached']['js'][] = $path . '/js/toccspeedtest.js';
        } catch (Exception $ex) {
            $form['message'] = array(
                '#markup' => '<div class="messages error">' . $ex->getMessage() . '</div>'
            );
        }
    } else {
        $account = toccspeedtest_get_connection()->status();
        if ($account['api_credits'] == '0') {
            $startAllowed = false;
            $form['message'] = array(
                '#markup' => t(
                    '<div class="messages error">Unfortunately you are out of credits with the gtmetrix api.  '
                        . 'You will get a topup of credits at !credits.</div>',
                    array(
                        '!credits' => date('d F Y H:i:s', $account['api_refill'])
                    )
                )
            );
        }
    }
    
    if ($startAllowed) {
        $form['submit'] = array(
            '#type' => 'submit',
            '#value' => t('Start test'),
            '#ajax' => array(
                'wrapper' => 'toccspeedtest_gtmetrix_view_form_wrapper',
                'callback' => 'toccspeedtest_gtmetrix_view_form_callback',
            )
        );
    }

    if (isset($form_state['rebuild']) && $form_state['rebuild'] == true) {
        $form['submit']['#value'] = 'Please wait';
        $form['submit']['#attributes']['disabled'] = 'disabled';
    }

    return $form;
}

/**
 * Start a new test on the gtmetrix api
 * 
 * @return string
 */
function toccspeedtest_start_gtmetrix_test()
{
    $test = toccspeedtest_get_connection();

    $testid = $test->test(array(
        'url' => toccspeedtest_get_website_test_url(),
        'location' => variable_get('toccspeedtest_gtmetrix_test_location', 1)
    ));
    
    if ($testid) {
        return $testid;
    } else {
        throw new Exception($test->error(), -1);
    }
}

/**
 * External hook to kick on the test callback
 * 
 * @return array
 */
function toccspeedtest_start_gtmetrix_test_callback()
{
    return array(
        'testid' => toccspeedtest_start_gtmetrix_test()
    );
}

/**
 * Form handler
 *
 * @param array $form       Form object
 * @param array $form_state Form state
 *
 * @return void
 */
function toccspeedtest_gtmetrix_view_form_submit($form, &$form_state)
{
    $form_state['rebuild'] = true;
}

/**
 * Ajax form handler
 *
 * @param array $form       Form object
 * @param array $form_state Form state
 * 
 * @return array
 */
function toccspeedtest_gtmetrix_view_form_callback($form, &$form_state)
{
    return $form;
}

/**
 * Poll the gtmetrix api
 * 
 * @param string $test_id GTMetrix testid
 * 
 * @return array
 */
function toccspeedtest_gtmetrix_callback($test_id)
{
    $status = 'completed';
    
    if (toccspeedtest_get_test($test_id) === false) {
        $test = toccspeedtest_get_connection();
        $test->load($test_id);
        $test->poll_state();

        $status = $test->state();
        if ($test->error()) {
            $status = 'error';
        }

        if ($status == 'completed') {
            global $user;
            
            // Get the report
            $image = $test->download_resource('report_pdf_full');
            if ($image) {
                // Save report
                $file = file_save_data(
                    $image,
                    'public://' . $test_id . '.pdf',
                    FILE_EXISTS_RENAME
                );
            }
            
            db_insert('toccspeedtest')
                ->fields(array(
                    'test_id' => $test_id,
                    'report_url' => $test->results()['report_url'],
                    'report_media_id' => (empty($file) ? 0 : $file->fid),
                    'web_url' => toccspeedtest_get_website_test_url(),
                    'pagespeed_score' => $test->results()['pagespeed_score'],
                    'yslow_score' => $test->results()['yslow_score'],
                    'html_load_bytes' => $test->results()['html_bytes'],
                    'html_load_time' => $test->results()['html_load_time'],
                    'page_load_bytes' => $test->results()['page_bytes'],
                    'page_load_time' => $test->results()['page_load_time'],
                    'page_elements' => $test->results()['page_elements'],
                    'created' => REQUEST_TIME,
                    'created_by' => $user->uid
                ))->execute();
            
            drupal_set_message('Test completed!');
        }
    }
    
    return array(
        'status' => $status
    );
}

/**
 * Return the wishlist select
 * 
 * @return \SelectQuery
 */
function toccspeedtest_get_query()
{
    $query = db_select('toccspeedtest', 'tst');
    $query->fields('tst');
    
    return $query;
}

/**
 * Check and return a test
 * 
 * @param string $test_id
 * 
 * @return boolean
 */
function toccspeedtest_get_test($test_id)
{
    $test = toccspeedtest_get_query()->condition('tst.test_id', $test_id, '=')
        ->execute()
        ->fetch();
    
    if ($test) {
        return $test;
    }
    
    return false;
}

/**
 * Exports the results stored in the table
 * 
 * @return void
 */
function toccspeedtest_gtmetrix_export_results()
{
    // send response headers to the browser
    drupal_add_http_header('Content-Type', 'text/csv');
    drupal_add_http_header('Content-Disposition', 'attachment;filename=results.csv');

    $fp = fopen('php://output', 'w');
    $results = toccspeedtest_get_query()->execute()->fetchAll();
    
    // Add headers
    fputcsv(
        $fp,
        array(
            'Test id',
            'Date',
            'Page speed score',
            'YSlow score',
            'HTML Size in bytes',
            'HTML load time',
            'Page Size in bytes',
            'Page load time',
        )
    );
    
    foreach($results as $result) {
        fputcsv(
            $fp,
            array(
                $result->test_id,
                date('d M Y H:i:s', $result->created),
                $result->pagespeed_score,
                $result->yslow_score,
                $result->html_load_bytes,
                $result->html_load_time,
                $result->page_load_bytes,
                $result->page_load_time
            )
        );
    }
    fclose($fp);
    drupal_exit();
}

/**
 * Return a page showing the gtmetrix retults
 * 
 * @return array
 */
function toccspeedtest_gtmetrix_get_results()
{
    $results = toccspeedtest_get_query()->execute()->fetchAll();
    
    if ($results && count($results) > 0) {
        return array(
            '#markup' => '<h2>Previous test results</h2>'
                . toccspeedtest_get_results_table($results)
                . '<p>' . l('Export results to csv', 'admin/config/tocc/speedtest/results/export') . '</p>'
        );
    } else {
        return array(
            '#markup' => '<h2>No results found</h2>'
        );
    }
}

/**
 * Return a table of data based on the metrix results
 * 
 * @param array $results
 * 
 * @return string
 */
function toccspeedtest_get_results_table($results)
{
    $header = array(
        array('data' => t('Test id')),
        array('data' => t('Test url')),
        array('data' => t('Page speed score')),
        array('data' => t('YSlow score')),
        array('data' => t('HTML Size')),
        array('data' => t('HTML load time')),
        array('data' => t('Page Size')),
        array('data' => t('Page load time')),
        array('data' => t('Request time')),
        array('data' => t('Report'))
    );
    
    // Sort by date
    uasort($results, function($a, $b) {
        return $b->created > $a->created;
    });

    $data = array();
    foreach ($results as $result) {
        
        $report_link = ($result->report_media_id > 0) ? l(
            'View report',
            'media/' . $result->report_media_id . '/open',
            array(
                'attributes' => array(
                    'target' => '_blank'
                )
            )
        ) : 'Not available';
        
        $data[] = array(
            array('data' => $result->test_id),
            array(
                'data' => $result->web_url,
                'onclick' => 'javascript:window.location = this.innerHTML; return false;'
            ),
            array('data' => $result->pagespeed_score),
            array('data' => $result->yslow_score),
            array('data' => round(($result->html_load_bytes / 1000), 2) . 'kb'),
            array('data' => round($result->html_load_time / 1000, 2) . 's'),
            array('data' => round(($result->page_load_bytes / 1000000), 2) . 'mb'),
            array('data' => round($result->page_load_time / 1000, 2) . 's'),
            array('data' => date('d M Y H:i:s', $result->created)),
            array('data' => $report_link)
        );
    }
    
    $per_page = 10;
    
    // Initialize the pager
    $current_page = pager_default_initialize(count($data), $per_page);
    $chunks = array_chunk($data, $per_page, TRUE);
    
    if (isset($chunks[$current_page])) {
        return theme(
            'table',
            array(
                'header' => $header,
                'rows' => $chunks[$current_page]
            )
        ) . theme('pager', array('quantity', count($data)));
    } else {
        return 'No results found';
    }
}