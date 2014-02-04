<?php
/**
 * @file
 * Houses the Simple Restrictions MENU_LOCAL_TASK form for islandora_restricted.
 */

/**
 * Creates the restriction management form for display in a MENU_LOCAL_TASK.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 *
 * @return array
 *   An array representing the manage form.
 */
function islandora_restricted_manage_form($form, $form_state, AbstractObject $object) {
  module_load_include('inc', 'islandora_restricted', 'includes/utilities');
  module_load_include('inc', 'islandora_restricted', 'includes/batch');
  // This currently retrieves all restrictions placed on the object which
  // can handle future more in depth usage. For now we will handle this like
  // there's only one.
  $restricted_options = islandora_restricted_retrieve_restrictions($object);
  // Let's parse the hidden and restricted arrays to be used in the UI.
  $user_objects = entity_load('user');
  $user_list = array_map(function ($user) {
    return empty($user->name) ? variable_get('anonymous', 'Anonymous') : $user->name;
  }, $user_objects);
  $role_list = user_roles();
  $form['restriction_choices'] = array(
    '#type' => 'radios',
    '#title' => t('Restriction Type'),
    '#default_value' => isset($restricted_options['state']) ? $restricted_options['state'] : 'public',
    '#options' => array(
      'public' => t('Public'),
      'restricted' => t('Restricted'),
      'hidden' => t('Hidden'),
    ),
  );
  $form['public_description'] = array(
    '#type' => 'item',
    '#markup' => t('Normal access rules apply and there is no change to the display of the object.'),
    '#states' => array(
      'visible' => array(
        ':input[name="restriction_choices"]' => array('value' => 'public'),
      ),
    ),
  );
  $form['restricted_description'] = array(
    '#type' => 'item',
    '#markup' => t("Only users with explicit permission can see the object. All other users only see the object's label and a general access denied thumbnail."),
    '#states' => array(
      'visible' => array(
        ':input[name="restriction_choices"]' => array('value' => 'restricted'),
      ),
    ),
  );
  $form['hidden_description'] = array(
    '#type' => 'item',
    '#markup' => t('Only users with explicit permission can see the object. All others are unaware that the object exists.'),
    '#states' => array(
      'visible' => array(
        ':input[name="restriction_choices"]' => array('value' => 'hidden'),
      ),
    ),
  );
  $form['restricted_allowed_users'] = array(
    '#type' => 'select',
    '#title' => t('Allowed Users When Restricted'),
    '#default_value' => (!empty($restricted_options) && !empty($restricted_options['restricted']['users'])) ? $restricted_options['restricted']['users'] : NULL,
    '#options' => $user_list,
    '#multiple' => TRUE,
  );
  $form['restricted_allowed_roles'] = array(
    '#type' => 'select',
    '#title' => t('Allowed Roles When Restricted'),
    '#default_value' => (!empty($restricted_options) && !empty($restricted_options['restricted']['roles'])) ? $restricted_options['restricted']['roles'] : NULL,
    '#options' => $role_list,
    '#multiple' => TRUE,
  );

  $form['hidden_allowed_users'] = array(
    '#type' => 'select',
    '#title' => t('Allowed Users When Hidden'),
    '#default_value' => (!empty($restricted_options) && !empty($restricted_options['hidden']['users'])) ? $restricted_options['hidden']['users'] : NULL,
    '#options' => $user_list,
    '#multiple' => TRUE,
  );
  $form['hidden_allowed_roles'] = array(
    '#type' => 'select',
    '#title' => t('Allowed Roles When Hidden'),
    '#default_value' => (!empty($restricted_options) && !empty($restricted_options['hidden']['roles'])) ? $restricted_options['hidden']['roles'] : NULL,
    '#options' => $role_list,
    '#multiple' => TRUE,
  );

  // Get all valid batch options for the current object.
  $batch_options = islandora_restricted_batch_options($object);
  $form['batch_update'] = array(
    '#type' => 'checkbox',
    '#title' => t('Cascade restrictions?'),
    '#access' => empty($batch_options) ? FALSE : TRUE,
  );

  if (!empty($batch_options)) {
    $ui_options = array();
    $form_state['islandora_restricted_batch_options'] = $batch_options;
    foreach ($batch_options as $key => $values) {
      $ui_options[$key] = $values['description'];
    }
    $form['batch_options'] = array(
      '#type' => 'select',
      '#title' => t('What items would you like to apply the restrictions to?'),
      '#options' => $ui_options,
      '#states' => array(
        'visible' => array(
          ':input[name="batch_update"]' => array('checked' => TRUE),
        ),
      ),
    );
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#name' => 'islandora-restricted-submit',
    '#value' => t('Apply'),
  );
  $form['delete'] = array(
    '#type' => 'submit',
    '#access' => empty($restricted_options) ? FALSE : TRUE,
    '#name' => 'islandora-restricted-delete',
    '#value' => t('Delete'),
  );
  $form['restrictions'] = array(
    '#type' => 'value',
    '#value' => $restricted_options,
  );
  return $form;
}

/**
 * Submit handler for the manage form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function islandora_restricted_manage_form_submit($form, $form_state) {
  module_load_include('inc', 'islandora_restricted', 'includes/utilities');
  $object = menu_get_object('islandora_object', '2');
  // Do submit things.
  if ($form_state['triggering_element']['#name'] == 'islandora-restricted-submit') {
    $restricted_state = $form_state['values']['restriction_choices'];
    $hidden_allowed_users = $form_state['values']['hidden_allowed_users'];
    $hidden_allowed_roles = $form_state['values']['hidden_allowed_roles'];
    $restricted_allowed_users = $form_state['values']['restricted_allowed_users'];
    $restricted_allowed_roles = $form_state['values']['restricted_allowed_roles'];
    $params = array(
      'state' => $restricted_state,
      'users' => array(
        'hidden' => $hidden_allowed_users,
        'restricted' => $restricted_allowed_users,
      ),
      'roles' => array(
        'hidden' => $hidden_allowed_roles,
        'restricted' => $restricted_allowed_roles,
      ),
    );
    $applied_restrictions = $form_state['values']['restrictions'];
    // See if we need to change the state.
    if ($restricted_state != 'public') {
      if (isset($applied_restrictions['state'])) {
        if ($restricted_state != $applied_restrictions['state']) {
          islandora_restricted_remove_state_relationship($object);
          islandora_restricted_add_state_relationship($object, $restricted_state);
        }
      }
      // Previously had no state associated add it.
      else {
        islandora_restricted_add_state_relationship($object, $restricted_state);
      }
    }
    else {
      // Moving to public so we need to remove the state relationship.
      islandora_restricted_remove_state_relationship($object);
    }
    // Now let's see if we need to update the users and roles.
    foreach ($params['users'] as $user_type => $users) {
      $existing_users = $applied_restrictions[$user_type]['users'];
      $users_deletion = array_diff_key($existing_users, $users);
      $users_insertion = array_diff_key($users, $existing_users);
      foreach ($users_deletion as $delete_uid) {
        islandora_restricted_remove_user_relationship($object, $user_type, $delete_uid);
      }
      foreach ($users_insertion as $insert_uid) {
        islandora_restricted_add_user_relationship($object, $user_type, $insert_uid);
      }
    }
    foreach ($params['roles'] as $role_type => $roles) {
      $existing_roles = $applied_restrictions[$role_type]['roles'];
      $roles_deletion = array_diff_key($existing_roles, $roles);
      $roles_insertion = array_diff_key($roles, $existing_roles);
      foreach ($roles_deletion as $delete_rid) {
        islandora_restricted_remove_role_relationship($object, $role_type, $delete_rid);
      }
      foreach ($roles_insertion as $insert_rid) {
        islandora_restricted_add_role_relationship($object, $role_type, $insert_rid);
      }
    }
    // Handle batch things.
    if ($form_state['values']['batch_update']) {
      $batch_query = $form_state['islandora_restricted_batch_options'][$form_state['values']['batch_options']];
      $batch = array(
        'title' => t('Updating Restrictions'),
        'progress_message' => t('Please wait while the objects are being updated.'),
        'operations' => array(
          /*array(
            'islandora_restricted_batch_function',
            array($object->id, $batch_query, $restricted_state, $allowed_roles, $allowed_users),
          ),*/
        ),
        'file' => drupal_get_path('module', 'islandora_restricted') . '/includes/batch.inc',
      );
      batch_set($batch);
    }
    drupal_set_message(t('The applied restriction has been saved!'));
  }
  // Do delete things.
  else {
    // See if the object actually had a value set before this point.
    islandora_restricted_remove_relationships($object);
    drupal_set_message(t('The applied restriction has been lifted!'));
  }
}
