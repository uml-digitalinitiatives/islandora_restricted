<?php
/**
 * @file
 * Contains utility functions for the islandora_restricted module.
 */

/**
 * Retrieves any current restrictions present on an object.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing a Fedora object.
 * @param bool $current_site
 *   Whether we are restricting the results to the current site or not.
 *
 * @return array
 *   An array containing:
 *   -state: A string equivalent to the current restricted state.
 *   -users: An array of user IDs that are being allowed for the type.
 *   -roles: An array of role IDs that are being allowed for the type.
 */
function islandora_restricted_retrieve_restrictions(AbstractObject $object, $current_site = FALSE) {
  $restrictions = array();
  $restricted_states_relationships = $object->relationships->get(
    ISLANDORA_RELS_EXT_URI,
    ISLANDORA_RESTRICTED_OBJECT_STATE_RELATIONSHIP
  );
  // There is will only be one state so let's handle it as such.
  if (!empty($restricted_states_relationships)) {
    $state_rels = reset($restricted_states_relationships);
    $restrictions['state'] = $state_rels['object']['value'];
  }
  $hidden_users_and_roles = $object->relationships->get(
    ISLANDORA_RELS_EXT_URI,
    ISLANDORA_RESTRICTED_CAN_SEE_HIDDEN_RELATIONSHIP,
    NULL,
    RELS_TYPE_FULL_URI
  );
  $restricted_users_and_roles = $object->relationships->get(
    ISLANDORA_RELS_EXT_URI,
    ISLANDORA_RESTRICTED_CAN_SEE_RESTRICTED_RELATIONSHIP,
    NULL,
    RELS_TYPE_FULL_URI
  );

  $hidden_restrictions = islandora_restricted_parse_users_and_roles($hidden_users_and_roles, $current_site);
  $restricted_restrictions = islandora_restricted_parse_users_and_roles($restricted_users_and_roles, $current_site);
  $restrictions['hidden'] = $hidden_restrictions;
  $restrictions['restricted'] = $restricted_restrictions;
  return $restrictions;
}

/**
 * Create a URI link for a Drupal user or role.
 *
 * @param string $id
 *   The user/role ID of the URI link we are creating.
 * @param string $type
 *   The type of URI link we are creating.
 *
 * @return string
 *   The link URI.
 */
function islandora_restricted_create_uri_link($id, $type) {
  global $base_url;
  if ($type == 'role') {
    $link_uri = "$base_url/admin/people/permissions/role/edit/$id";
  }
  else {
    $link_uri = "$base_url/user/$id";
  }
  return $link_uri;
}

/**
 * Checks if the entities are in the current site and parses the user/roles.
 *
 * @param array $rels_results
 *   An array representing the relationships on the object.
 * @param bool $current_site
 *   Whether we are restricting to the current site or not.
 *
 * @return array
 *   An array containing the users and roles for the specified restriction type.
 */
function islandora_restricted_parse_users_and_roles($rels_results, $current_site) {
  $parsed_rels = array(
    'users' => array(),
    'roles' => array(),
  );
  foreach ($rels_results as $rels) {
    $entity_uri = $rels['object']['value'];
    if (!$current_site) {
      if (islandora_restricted_valid_user_uri($entity_uri)) {
        $parsed_rels['users'][] = $entity_uri;
      }
      if (islandora_restricted_valid_role_uri($entity_uri)) {
        $parsed_rels['roles'][] = $entity_uri;
      }
    }
    else {
      if (strpos($entity_uri, 'user') !== FALSE) {
        $parsed_rels['users'][] = $entity_uri;
      }
      elseif (strpos($entity_uri, 'role') !== FALSE) {
        $parsed_rels['roles'][] = $entity_uri;
      }
    }
  }
  return $parsed_rels;
}

/**
 * Determines whether the user is valid in the current site structure.
 *
 * @param string $entity_uri
 *   A string containing the entity URI.
 *
 * @return bool
 *   TRUE if it is valid, FALSE otherwise.
 */
function islandora_restricted_valid_user_uri($entity_uri) {
  global $base_url;
  if (strpos($entity_uri, "$base_url/user/") !== FALSE) {
    return TRUE;
  }
  return FALSE;
}

/**
 * Determines whether the role is valid in the current site structure.
 *
 * @param string $entity_uri
 *   A string containing the entity URI.
 *
 * @return bool
 *   TRUE if it is valid, FALSE otherwise.
 */
function islandora_restricted_valid_role_uri($entity_uri) {
  global $base_url;
  if (strpos($entity_uri, "$base_url/admin/people/permissions/role/edit/") !== FALSE) {
    return TRUE;
  }
  return FALSE;
}

/**
 * Retrieves the Drupal UID from a given entity URI.
 *
 * @param string $entity_uri
 *   A string containing the entity URI.
 *
 * @return string
 *   The Drupal UID parsed from the entity URI.
 */
function islandora_restricted_retrieve_user_from_uri($entity_uri) {
  global $base_url;
  return str_replace("$base_url/user/", '', $entity_uri);
}

/**
 * Retrieves the Drupal RID from a given entity URI.
 *
 * @param string $entity_uri
 *   A string containing the entity URI.
 *
 * @return string
 *   The Drupal RID parsed from the entity URI.
 */
function islandora_restricted_retrieve_role_from_uri($entity_uri) {
  global $base_url;
  return str_replace("$base_url/admin/people/permissions/role/edit/", '', $entity_uri);
}

/**
 * Adds the relationships to the object.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object in Fedora.
 * @param array $params
 *   An array containing:
 *   -state: A string representing the state that is being added.
 *   -users: An array containing associative arrays:
 *     -hidden: An array that defines the UIDs that are allowed to view hidden
 *     objects.
 *     -restricted: An array that defines the UIDs are allowed to view
 *     restricted objects.
 *   -roles: An array containing associative arrays:
 *     -hidden: An array that defines the RIDs that are allowed to view hidden
 *     objects.
 *     -restricted: An array that defines the RIDs are allowed to view
 *     restricted objects.
 */
function islandora_restricted_add_relationships(AbstractObject $object, $params) {
  // Let's first add the state.
  islandora_restricted_add_state_relationship($object, $params['state']);
  foreach ($params['users'] as $restriction_type => $users) {
    foreach ($users as $uid) {
      islandora_restricted_add_user_relationship($object, $restriction_type, $uid);
    }
  }
  foreach ($params['roles'] as $restriction_type => $roles) {
    foreach ($roles as $rid) {
      islandora_restricted_add_role_relationship($object, $restriction_type, $rid);
    }
  }
}

/**
 * Add a specified Drupal UID to the RELS-EXT for a specified restriction.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing a Fedora object.
 * @param string $restricted_state
 *   A string representing the restricted state of the object.
 * @param string $uid
 *   The Druapl UID that is being allowed to access the object.
 */
function islandora_restricted_add_user_relationship($object, $restricted_state, $uid) {
  if ($restricted_state == 'hidden') {
    $relationship_type = ISLANDORA_RESTRICTED_CAN_SEE_HIDDEN_RELATIONSHIP;
  }
  else {
    $relationship_type = ISLANDORA_RESTRICTED_CAN_SEE_RESTRICTED_RELATIONSHIP;
  }
  $link_uri = islandora_restricted_create_uri_link($uid, 'user');
  $object->relationships->add(
    ISLANDORA_RELS_EXT_URI,
    $relationship_type,
    $link_uri,
    RELS_TYPE_FULL_URI
  );
}

/**
 * Add a specified Drupal RID to the RELS-EXT for a specified restriction.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing a Fedora object.
 * @param string $restricted_state
 *   A string representing the restricted state of the object.
 * @param string $rid
 *   The Druapl RID that is being allowed to access the object.
 */
function islandora_restricted_add_role_relationship($object, $restricted_state, $rid) {
  if ($restricted_state == 'hidden') {
    $relationship_type = ISLANDORA_RESTRICTED_CAN_SEE_HIDDEN_RELATIONSHIP;
  }
  else {
    $relationship_type = ISLANDORA_RESTRICTED_CAN_SEE_RESTRICTED_RELATIONSHIP;
  }
  $link_uri = islandora_restricted_create_uri_link($rid, 'role');
  $object->relationships->add(
    ISLANDORA_RELS_EXT_URI,
    $relationship_type,
    $link_uri,
    RELS_TYPE_FULL_URI
  );
}

/**
 * Remove relationships from the object.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 */
function islandora_restricted_remove_relationships(AbstractObject $object) {
  // Remove the state first.
  $object->relationships->remove(
    ISLANDORA_RELS_EXT_URI,
    ISLANDORA_RESTRICTED_OBJECT_STATE_RELATIONSHIP
  );
  // Now all the hidden restrictions.
  $object->relationships->remove(
    ISLANDORA_RELS_EXT_URI,
    ISLANDORA_RESTRICTED_CAN_SEE_HIDDEN_RELATIONSHIP
  );
  // Now all the restricted restrictions.
  $object->relationships->remove(
    ISLANDORA_RELS_EXT_URI,
    ISLANDORA_RESTRICTED_CAN_SEE_RESTRICTED_RELATIONSHIP
  );
}

/**
 * Remove a specified Drupal UID to the RELS-EXT for a specified restriction.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing a Fedora object.
 * @param string $restricted_state
 *   A string representing the restricted state of the object.
 * @param string $uid
 *   The Druapl UID that is being allowed to access the object.
 */
function islandora_restricted_remove_user_relationship(AbstractObject $object, $restricted_state, $uid) {
  if ($restricted_state == 'restricted') {
    $viewing_type = ISLANDORA_RESTRICTED_CAN_SEE_RESTRICTED_RELATIONSHIP;
  }
  else {
    $viewing_type = ISLANDORA_RESTRICTED_CAN_SEE_HIDDEN_RELATIONSHIP;
  }
  $link_uri = islandora_restricted_create_uri_link($uid, 'user');
  $object->relationships->remove(
    ISLANDORA_RELS_EXT_URI,
    $viewing_type,
    $link_uri,
    RELS_TYPE_FULL_URI
  );
}

/**
 * Remove a specified Drupal RID to the RELS-EXT for a specified restriction.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing a Fedora object.
 * @param string $restricted_state
 *   A string representing the restricted state of the object.
 * @param string $rid
 *   The Druapl RID that is being allowed to access the object.
 */
function islandora_restricted_remove_role_relationship(AbstractObject $object, $restricted_state, $rid) {
  if ($restricted_state == 'restricted') {
    $viewing_type = ISLANDORA_RESTRICTED_CAN_SEE_RESTRICTED_RELATIONSHIP;
  }
  else {
    $viewing_type = ISLANDORA_RESTRICTED_CAN_SEE_HIDDEN_RELATIONSHIP;
  }
  $link_uri = islandora_restricted_create_uri_link($rid, 'role');
  $object->relationships->remove(
    ISLANDORA_RELS_EXT_URI,
    $viewing_type,
    $link_uri,
    RELS_TYPE_FULL_URI
  );
}

/**
 * Adds the given state to the RELS-EXT for an object.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 * @param string $restricted_state
 *   The state that the object is moving to.
 */
function islandora_restricted_add_state_relationship(AbstractObject $object, $restricted_state) {
  $object->relationships->add(
    ISLANDORA_RELS_EXT_URI,
    ISLANDORA_RESTRICTED_OBJECT_STATE_RELATIONSHIP,
    $restricted_state,
    TRUE
  );
}

/**
 * Remove the state from  the RELS-EXT for an object.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 */
function islandora_restricted_remove_state_relationship(AbstractObject $object) {
  $object->relationships->remove(
    ISLANDORA_RELS_EXT_URI,
    ISLANDORA_RESTRICTED_OBJECT_STATE_RELATIONSHIP
  );
}
