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
function islandora_restricted_admin_form($form, &$form_state) {
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
    '#type' => 'textfield',
    '#title' => t('Restricted access login page'),
    '#description' => t('URL of a page with a login block to send anonymous users, this can provide different information from the "Access denied redirect" page.'),
    '#default_value' => variable_get('islandora_restricted_redirect_login', ''),
  );
  return system_settings_form($form);
}
