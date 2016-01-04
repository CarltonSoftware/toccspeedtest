<?php

function toccspeedtest_gtmetrix_config_form($form, &$form_state)
{
    $form['intro'] = array(
        '#markup' => '<p>Before being able to use the api, you need to <a href="https://gtmetrix.com/api/">sign up</a> for an api key.</p>'
    );
    $form['toccspeedtest_gtmetrix_email'] = array(
        '#type' => 'textfield',
        '#required' => true,
        '#title' => t('Api email'),
        '#default_value' => variable_get('toccspeedtest_gtmetrix_email', '')
    );
    $form['toccspeedtest_gtmetrix_apikey'] = array(
        '#type' => 'textfield',
        '#required' => true,
        '#title' => t('Api key'),
        '#default_value' => variable_get('toccspeedtest_gtmetrix_apikey', '')
    );
    $form['toccspeedtest_gtmetrix_test_url'] = array(
        '#type' => 'textfield',
        '#required' => true,
        '#title' => t('Test url'),
        '#default_value' => variable_get('toccspeedtest_gtmetrix_test_url', '<front>')
    );
    
    try {
        if ($conn = toccspeedtest_get_connection()) {
            $locations = array();
            foreach ($conn->locations() as $location) {
                $locations[$location['id']] = $location['name'];
            }
            
            $form['toccspeedtest_gtmetrix_test_location'] = array(
                '#type' => 'select',
                '#title' => t('Test location'),
                '#description' => t('Location of test server'),
                '#default_value' => variable_get('toccspeedtest_gtmetrix_test_location'),
                '#options' => $locations
            );
        }
    } catch (Exception $ex) {

    }

    return system_settings_form($form);
}