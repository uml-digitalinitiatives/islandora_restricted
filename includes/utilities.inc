<?php
/**
 * @file
 * Contains utility functions for the islandora_restricted module.
 */

/**
 * Get the islandora_restricted state of an object.
 *
 * Assumes a single state on an object.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing a Fedora object.
 *
 * @return string
 *   The state of the object.
 */
function islandora_restricted_get_state(AbstractObject $object) {
  $restricted_states_relationships = $object->relationships->get(
    ISLANDORA_RELS_EXT_URI,
    ISLANDORA_RESTRICTED_OBJECT_STATE_RELATIONSHIP
  );
  $state_rels = reset($restricted_states_relationships);
  return $state_rels['object']['value'];
}

/**
 * Retrieves any current restrictions present on an object.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing a Fedora object.
 * @param bool $current_site
 *   Whether we are restricting the results to the current site or not.
 *
 * @return array
 *   An array containing...
 *   -state: A string equivalent to the current restricted state.
 *   -users: An array of user IDs that are being allowed for the type.
 *   -roles: An array of role IDs that are being allowed for the type.
 */
function islandora_restricted_retrieve_restrictions(AbstractObject $object, $current_site = FALSE) {
  $restrictions = array();
  $state = islandora_restricted_get_state($object);
  // There will only be one state so let's handle it as such.
  if ($state) {
    $restrictions['state'] = $state;
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
    if ($current_site) {
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
 * @param array $restriction
 *   See islandora_restricted_parse_users_and_roles().
 */
function islandora_restricted_add_relationships(AbstractObject $object, $restriction) {
  if (islandora_restricted_populated_restriction($restriction)) {
    if (isset($restriction['state'])) {
      islandora_restricted_add_state_relationship($object, $restriction['state']);
    }
    // Future datastructure additions possible.
    foreach ($restriction as $key => $value) {
      if (is_array($value) && !empty($value)) {
        $restriction_type = $key;
        foreach ($value as $entity_array) {
          foreach ($entity_array as $entity_uri) {
            islandora_restricted_add_entity_relationship_uri($object, $restriction_type, $entity_uri);
          }
        }
      }
    }
  }
}

/**
 * Add a specified entity URI restriction.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing a Fedora object.
 * @param string $restricted_state
 *   A string representing the restricted state of the object.
 * @param string $link_uri
 *   The subject.
 */
function islandora_restricted_add_entity_relationship_uri($object, $restricted_state, $link_uri) {
  if ($restricted_state == 'hidden') {
    $relationship_type = ISLANDORA_RESTRICTED_CAN_SEE_HIDDEN_RELATIONSHIP;
  }
  else {
    $relationship_type = ISLANDORA_RESTRICTED_CAN_SEE_RESTRICTED_RELATIONSHIP;
  }
  $object->relationships->add(
    ISLANDORA_RELS_EXT_URI,
    $relationship_type,
    $link_uri,
    RELS_TYPE_FULL_URI
  );
}

/**
 * Delete a specified entity URI restriction.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing a Fedora object.
 * @param string $restricted_state
 *   A string representing the restricted state of the object.
 * @param string $link_uri
 *   The subject.
 */
function islandora_restricted_delete_entity_relationship_uri($object, $restricted_state, $link_uri) {
  if ($restricted_state == 'hidden') {
    $relationship_type = ISLANDORA_RESTRICTED_CAN_SEE_HIDDEN_RELATIONSHIP;
  }
  else {
    $relationship_type = ISLANDORA_RESTRICTED_CAN_SEE_RESTRICTED_RELATIONSHIP;
  }
  $object->relationships->remove(
    ISLANDORA_RELS_EXT_URI,
    $relationship_type,
    $link_uri,
    RELS_TYPE_FULL_URI
  );
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
  $link_uri = islandora_restricted_create_uri_link($uid, 'user');
  islandora_restricted_add_entity_relationship_uri($object, $restricted_state, $link_uri);
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
  $link_uri = islandora_restricted_create_uri_link($rid, 'role');
  islandora_restricted_add_entity_relationship_uri($object, $restricted_state, $link_uri);
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

/**
 * Determines whether the current object has a restriction set on it.
 *
 * @param array $restriction
 *   An array containing the following information about the restriction:
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
 *
 * @return bool
 *   TRUE if the restriction is populated, FALSE otherwise.
 */
function islandora_restricted_populated_restriction($restriction) {
  if (empty($restriction['hidden']['users']) &&
    empty($restriction['hidden']['roles']) &&
    empty($restriction['restricted']['users']) &&
    empty($restriction['restricted']['roles']) &&
    !isset($restriction['state'])) {
    return FALSE;
  }
  return TRUE;
}

/**
 * Checks if the user has access to an object by our rules.
 *
 * @param AbstractObject $object
 *   Object to check for permission.
 * @param object $user_to_check
 *   User to check for permission.
 *
 * @return bool
 *   TRUE if the user has access, FALSE if not.
 */
function islandora_restricted_user_has_access(AbstractObject $object, $user_to_check) {
  // This means that all people that can add a restriction can see ALL objects.
  // if (user_access(ISLANDORA_RESTRICTED_APPLY_RESTRICTIONS, $user_to_check)) {
    
  // Instead we'll leave the fixing of broken restrictions to a higher
  // level of rights
  if (user_access('administer site configuration', $user_to_check)) {
    return TRUE;
  }
  $restrictions = islandora_restricted_retrieve_restrictions($object, TRUE);
  if (isset($restrictions['state'])) {
    $state = $restrictions['state'];
    foreach ($restrictions[$state] as $entity_type => $entity_values) {
      if ($entity_type == 'users') {
        if (in_array(islandora_restricted_create_uri_link($user_to_check->uid, 'user'), $entity_values)) {
          return TRUE;
        }
      }
      else {
        foreach ($user_to_check->roles as $rid => $role_name) {
          if (in_array(islandora_restricted_create_uri_link($rid, 'role'), $entity_values)) {
            return TRUE;
          }
        }
      }
    }
    return FALSE;
  }
  return TRUE;
}
