<?php
/**
 * @file
 * Contains batch functionality for the islandora_restricted module.
 */

/**
 * Holds a mapping of queries based upon content model.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 *
 * @return array
 *   An array of valid child batch queries for the current object.
 *
 * This needs to be re-addressed in the future to be more generic.
 */
function islandora_restricted_batch_options(AbstractObject $object) {
  // @TODO: This should be updated in the future to use hooks similar to
  // or combining what exists in the XACML module or alternatively
  // using a generic form of traversal.
  $mappings = array();
  if (in_array('islandora:bookCModel', $object->models)) {
    $pages_query = <<<EOQ
  SELECT ?object from <#ri> WHERE
  {
    ?object <fedora-rels-ext:isMemberOf> <info:fedora/{$object->id}>
  }
EOQ;
    $mappings['islandora:bookCModel'] = array(
      'type' => 'sparql',
      'query' => $pages_query,
      'description' => t('All pages of this book (existing and new).'),
    );
  }

  if (in_array('islandora:newspaperIssueCModel', $object->models)) {
    $pages_query = <<<EOQ
    SELECT ?object from <#ri> WHERE
    {
      ?object <fedora-rels-ext:isMemberOf> <info:fedora/{$object->id}>
    }
EOQ;
    $mappings['islandora:newspaperIssueCModel'] = array(
      'type' => 'sparql',
      'query' => $pages_query,
      'description' => t('All pages of this issue (existing and new).'),
    );
  }

  if (in_array('islandora:newspaperCModel', $object->models)) {
    $issues_query = <<<EOQ
  SELECT ?object from <#ri> WHERE
  {
    ?object <fedora-rels-ext:isMemberOf> <info:fedora/{$object->id}>
  }
EOQ;
    $mappings['islandora:newspaperCModel'] = array(
      'type' => 'sparql',
      'query' => $issues_query,
      'description' => t('All newspaper issues and their pages (existing and new).'),
    );
  }

  if (in_array('islandora:collectionCModel', $object->models)) {
    $collection_query = <<<EOQ
  select ?object from <#ri> WHERE
  {
    {
      ?object <fedora-rels-ext:isMemberOfCollection> <info:fedora/{$object->id}>
    }
    UNION
    {
      ?object <fedora-rels-ext:isMemberOf> <info:fedora/{$object->id}>
    }
    ?object <fedora-model:state> <info:fedora/fedora-system:def/model#Active>
  }
EOQ;
    $mappings['islandora:collectionCModel'] = array(
      'type' => 'sparql',
      'query' => $collection_query,
      'description' => t('All children of this collection and collections within this collection (existing and new).'),
    );
  }
  return $mappings;
}

/**
 * Performs the batch updation for the islandora_restricted module.
 *
 * @param string $source_pid
 *   The target pid where we are starting from.
 * @param array $batch_query
 *   An associative array where the key is the unique ID and contains:
 *   -type: The type of query, either sparql or itql.
 *   -query: The defined query string.
 *   -description: The human-readable description of the query.
 * @param string $type
 *   The type of action that is happening either delete or apply.
 * @param array $context
 *   The context of the current batch,
 */
function islandora_restricted_batch_function($source_pid, $batch_query, $type, &$context) {
  module_load_include('inc', 'islandora_restricted', 'includes/utilities');
  if (empty($context['sandbox'])) {
    $context['sandbox'] = array();
    $context['sandbox']['progress'] = 0;
    $batch_pids = array();
    islandora_restricted_gather_pids($source_pid, $batch_query, $batch_pids);
    $context['sandbox']['pids'] = array_keys($batch_pids);
    $context['sandbox']['items'] = count($context['sandbox']['pids']);
    $context['results']['success'] = array();
  }
  if (!empty($context['sandbox']['pids'])) {
    $target_pid = array_pop($context['sandbox']['pids']);
    $context['message'] = t('Updating restrictions for @pid (@current of @total)...', array(
      '@pid' => $target_pid,
      '@current' => $context['sandbox']['progress'],
      '@total' => $context['sandbox']['items'],
    ));
    if ($type == 'apply') {
      islandora_restricted_batch_update($target_pid, $source_pid);
    }
    else {
      islandora_restricted_remove_relationships(islandora_object_load($target_pid));
    }
    $context['results']['success'][] = $target_pid;
    $context['sandbox']['progress']++;
  }
  // Make sure we don't divide by zero.
  $context['finished'] = ($context['sandbox']['items'] == 0) ? 1 : ($context['sandbox']['progress'] / $context['sandbox']['items']);
}

/**
 * Utility function that goes out and gathers all the pids to be batched over.
 *
 * @param string $pid
 *   The root pid we are querying from.
 * @param array $batch_query
 *   An associative array where the key is the unique ID and contains:
 *   -type: The type of query, either sparql or itql.
 *   -query: The defined query string.
 *   -description: The human-readable description of the query.
 * @param array $batch_pids
 *   An array containing the persistent list of pids to eventually batch.
 */
function islandora_restricted_gather_pids($pid, $batch_query, &$batch_pids) {
  $object = islandora_object_load($pid);
  if ($batch_query['type'] == 'itql') {
    $content = $object->repository->ri->itqlQuery($batch_query['query']);
  }
  else {
    $content = $object->repository->ri->sparqlQuery($batch_query['query']);
  }
  foreach ($content as $result) {
    // We need to potentially recurse down even more to cover differing data
    // structure for example books and newspapers.
    $result_pid = $result['object']['value'];
    if (!isset($batch_pids[$result_pid])) {
      $batch_pids[$result_pid] = TRUE;
      $result_object = islandora_object_load($result_pid);
      $recurse_mappings = islandora_restricted_batch_options($result_object);
      if (!empty($recurse_mappings)) {
        foreach ($recurse_mappings as $query_choice) {
          islandora_restricted_gather_pids($result_object->id, $query_choice, $batch_pids);
        }
      }
    }
  }
}

/**
 * Update the target object to reflect what has been passed through.
 *
 * @param string $target_pid
 *   The PID that we are updating.
 * @param string $source_pid
 *   The PID that we are updating against.
 */
function islandora_restricted_batch_update($target_pid, $source_pid) {
  module_load_include('inc', 'islandora_restricted', 'includes/utilities');
  $target_object = islandora_object_load($target_pid);
  $source_object = islandora_object_load($source_pid);
  $current_restrictions = islandora_restricted_retrieve_restrictions($target_object, FALSE);
  $applied_restrictions = islandora_restricted_retrieve_restrictions($source_object, FALSE);
  if (!islandora_restricted_populated_restriction($current_restrictions)) {
    // Not one currently set, use the add function.
    islandora_restricted_add_relationships($target_object, $applied_restrictions);
  }
  // Need to determine what's different and add on accordingly.
  else {
    islandora_restricted_batch_update_existing_restrictions($target_object, $current_restrictions, $applied_restrictions);
  }
}

/**
 * Utility function used to updating an object that has existing restrictions.
 *
 * @param AbstractObject $target_object
 *   An AbstractObject representing an object within Fedora.
 *
 * @param array $current_restrictions
 *   An array containing the following about the current restriction:
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
 * @param array $applied_restrictions
 *   An array containing the following about the applied restriction:
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
function islandora_restricted_batch_update_existing_restrictions(AbstractObject $target_object, $current_restrictions, $applied_restrictions) {
  // See if we need to change the state. For purposes of this function we will
  // set the value to public if it's not set for easier comparisons.
  if (!isset($current_restrictions['state'])) {
    $current_restrictions['state'] = 'public';
  }
  if (!isset($current_restrictions['state'])) {
    $applied_restrictions['state'] = 'public';
  }
  if ($applied_restrictions['state'] != 'public') {
    if ($current_restrictions['state'] != $applied_restrictions['state']) {
      islandora_restricted_remove_state_relationship($target_object);
      islandora_restricted_add_state_relationship($target_object, $applied_restrictions['state']);
    }
  }
  else {
    // Moving to public so we need to remove the state relationship.
    islandora_restricted_remove_state_relationship($target_object);
  }
  // Now check the users and roles.
  $restrictions_types = array(
    'hidden',
    'restricted',
  );
  foreach ($restrictions_types as $type) {
    $users_insertion = array_diff($applied_restrictions[$type]['users'], $current_restrictions[$type]['users']);
    $users_deletion = array_diff($current_restrictions[$type]['users'], $applied_restrictions[$type]['users']);
    foreach ($users_deletion as $delete_user_uri) {
      islandora_restricted_delete_entity_relationship_uri($target_object, $type, $delete_user_uri);
    }
    foreach ($users_insertion as $insert_user_uri) {
      islandora_restricted_add_entity_relationship_uri($target_object, $type, $insert_user_uri);
    }

    $roles_insertion = array_diff($applied_restrictions[$type]['roles'], $current_restrictions[$type]['roles']);
    $roles_deletion = array_diff($current_restrictions[$type]['roles'], $applied_restrictions[$type]['roles']);
    foreach ($roles_deletion as $delete_role_uri) {
      islandora_restricted_delete_entity_relationship_uri($target_object, $type, $delete_role_uri);
    }
    foreach ($roles_insertion as $insert_role_uri) {
      islandora_restricted_add_entity_relationship_uri($target_object, $type, $insert_role_uri);
    }
  }
}

/**
 * Finished function for the update restrictions batch.
 *
 * @param bool $success
 *   Whether the batch was successful or not.
 * @param array $results
 *   An array containing the results of the batch operations.
 * @param array $operations
 *   The operations array that was used in the batch.
 */
function islandora_restricted_batch_finished($success, $results, $operations) {
  if ($success) {
    $message = format_plural(count($results['success']), 'One restriction updated.', '@count restrictions updated.');
  }
  else {
    $message = t('Finished with an error.');
  }
  drupal_set_message($message);
}
