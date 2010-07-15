<?php
/**
 * @file
 * Hybrid Authentication (OpenId + OAuth) for Google Data API
 * 
 * This file implements the authentication callback
 */

/**
 * Authentication callback. Parse OpenID + OAuth parameters
 * 
 * Remae of openid_authentication_page()
 */
function google_hybrid_authentication_page() {
  $result = openid_complete();
  switch ($result['status']) {
    case 'success':
      // This will store the OAuth parameters in session
      $params = google_hybrid_response_params($result, GOOGLE_HYBRID_OPENID_OAUTH);
      if ($params && ($consumer = google_hybrid_oauth_consumer())) {
         // We store the consumer key too, to be used later
        $params['consumer'] = $consumer->key;
        google_hybrid_store_params($params);
      }
      // Resume usual OpenID authentication
      return google_hybrid_openid_authentication($result);
    case 'failed':
      drupal_set_message(t('OpenID login failed.'), 'error');
      break;
    case 'cancel':
      drupal_set_message(t('OpenID login cancelled.'));
      break;
  }
  drupal_goto();
}

/**
 * Get some namespace parameters from the response, like the OAuth parameters
 */
function google_hybrid_response_params($result, $namespace) {
  // Get the key for the OAuth namespace. This can change depending on OpenID parameters
  // It should be 'ext1' or 'ext2' but it's possible also that we get no OAuth response
  if ($key = array_search($namespace, $result)) {   
    $parts = explode('.', $key);
    $ns = 'openid.' . $parts[2];
    $position = strlen($ns) + 1;
    $params = array();
    foreach ($result as $key => $value) {
      if (strpos($key, $ns) === 0) {
        $name = substr($key, $position);
        $params[$name] = $value;
      } 
    }
    return $params;
  }  
}

/**
 * Authenticate a user or attempt registration.
 * 
 * Rewritten openid_authentication() to implement some custom behavior
 * 
 * @param $response Response values from the OpenID Provider.
 */
function google_hybrid_openid_authentication($response) {
  // Before anything else we do some parameter mapping
  // I don't know why but Google doesn't return sreg parameters (?)
  $axdata = google_hybrid_response_params($response, GOOGLE_HYBRID_OPENID_AX);
  $response['openid.sreg.nickname'] = $axdata['value.first'] . ' ' . $axdata['value.last'];
  $response['openid.sreg.email'] = $axdata['value.email'];

  // If not enabled quick registration, use openid module instead
  if (!variable_get('google_hybrid_user_register', 0)) {
    return openid_authentication($response);
  }

  // Quick registration is enabled, this means:
  // - We trust all data returned from Google
  // - Don't require email verification
  module_load_include('inc', 'openid');

  $identity = $response['openid.claimed_id'];

  $account = user_external_load($identity);

  if (isset($account->uid)) {
    // Custom 1: We trust google e-mails without checking, unless hidden variable overridden
    if (!variable_get('google_hybrid_email_verification', FALSE) || $account->login) {
      user_external_login($account, $_SESSION['openid']['user_login_values']);
    }
    else {
      drupal_set_message(t('You must validate your email address for this account before logging in via OpenID'));
    }
  }
  else {
    // Register new user
    $form_state['redirect'] = NULL;
    $form_state['values']['name'] = (empty($response['openid.sreg.nickname'])) ? '' : $response['openid.sreg.nickname'];
    $form_state['values']['mail'] = (empty($response['openid.sreg.email'])) ? '' : $response['openid.sreg.email'];
    $form_state['values']['pass']  = user_password();
    $form_state['values']['status'] = variable_get('user_register', 1) == 1;
    $form_state['values']['response'] = $response;
    $form_state['values']['auth_openid'] = $identity;

    if (empty($response['openid.sreg.email']) && empty($response['openid.sreg.nickname'])) {
      drupal_set_message(t('Please complete the registration by filling out the form below. If you already have an account, you can <a href="@login">log in</a> now and add your OpenID under "My account".', array('@login' => url('user/login'))), 'warning');
      $success = FALSE;
    }
    else {
      $form = drupal_retrieve_form('user_register', $form_state);
      drupal_prepare_form('user_register', $form, $form_state);
      drupal_validate_form('user_register', $form, $form_state);
      $success = !form_get_errors();
      if (!$success) {
        drupal_set_message(t('Account registration using the information provided by your OpenID provider failed due to the reasons listed below. Please complete the registration by filling out the form below. If you already have an account, you can <a href="@login">log in</a> now and add your OpenID under "My account".', array('@login' => url('user/login'))), 'warning');
        // Append form validation errors below the above warning.
        $messages = drupal_get_messages('error');
        foreach ($messages['error'] as $message) {
          drupal_set_message( $message, 'error');
        }
      }
    }
    if (!$success) {
      // We were unable to register a valid new user, redirect to standard
      // user/register and prefill with the values we received.
      $_SESSION['openid']['values'] = $form_state['values'];
      // We'll want to redirect back to the same place.
      $destination = drupal_get_destination();
      unset($_REQUEST['destination']);
      drupal_goto('user/register', $destination);
    }
    else {
      unset($form_state['values']['response']);
      $account = user_save('', $form_state['values']);
      // Terminate if an error occured during user_save().
      if (!$account) {
        drupal_set_message(t("Error saving user account."), 'error');
        drupal_goto();
      }
      user_external_login($account);
    }
    drupal_redirect_form($form, $form_state['redirect']);
  }

  drupal_goto();
}
