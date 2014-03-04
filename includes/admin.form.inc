<?php
/**
 * @file
 * Houses the administration form for the islandora_restricted module.
 */

/**
 * Builds and renders the administration form for islandora_restricted.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 *
 * @return array
 *   An array representing the administration form.
 */
function islandora_restricted_admin_form($form, $form_state) {
  $form['islandora_restricted_state_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr security state field'),
    '#default_value' => variable_get('islandora_restricted_state_field', 'RELS_EXT_restrictedState_literal_ms'),
  );
  $form['islandora_restricted_restricted_exclude_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Entities excluded from restriction state Solr field'),
    '#default_value' => variable_get('islandora_restricted_restricted_exclude_field', 'RELS_EXT_canSeeRestricted_uri_ms'),
  );
  $form['islandora_restricted_hidden_exclude_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Entities excluded from hidden state Solr field'),
    '#default_value' => variable_get('islandora_restricted_hidden_exclude_field', 'RELS_EXT_canSeeHidden_uri_ms'),
  );
  $form['islandora_restricted_acces_denied_redirect'] = array(
    '#type' => 'textfield',
    '#title' => t('Access denied redirect'),
    '#description' => t('The URL to go to when access is denied.'),
    '#default_value' => variable_get(
      'islandora_restricted_acces_denied_redirect',
      ''
    ),
  );
  $form['islandora_restricted_redirect_login'] = array(
    '#type' => 'checkbox',
    '#title' => t('Redirect to a login form?'),
    '#description' => t('If you are redirecting to a login form or page with a login box, we will pass the object path to allow you to redirect directly back to the object once you have logged in.'),
    '#return_value' => 1,
    '#default_value' => variable_get('islandora_restricted_redirect_login',0),
  );
  return system_settings_form($form);
}
