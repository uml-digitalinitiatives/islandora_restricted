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
    '#default_value' => variable_get('islandora_restricted_state_field', 'somedefaultupdate'),
  );
  $form['islandora_restricted_exclude_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr excluded restrictions field'),
    '#default_value' => variable_get('islandora_restricted_exclude_field', 'somedefaultupdate'),
  );
  return system_settings_form($form);
}
