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
 * @param string $pid
 *   The target pid where we are starting from.
 * @param array $batch_query
 *
 * @param array $context
 */
function islandora_restricted_batch_function($source_pid, $batch_query, &$context) {
  if (empty($context['sandbox'])) {
    $context['sandbox'] = array();
    $context['sandbox']['progress'] = 0;
    $batch_pids = array();
    islandora_restricted_gather_pids($source_pid, $batch_query, $batch_pids);
    $context['sandbox']['pids'] = array_keys($batch_pids);
    $context['sandbox']['items'] = count($context['sandbox']['pids']);
    $context['results']['redirect'] = $source_pid;
    $context['results']['success'] = array();
    $context['results']['fail'] = array();
  }
  $target_pid = array_pop($context['sandbox']['pids']);
  islandora_restricted_batch_update($target_pid, $source_pid);
  $context['sandbox']['progress']++;

  // Make sure we don't divide by zero.
  $context['finished'] = ($context['sandbox']['items'] == 0) ? 1 : ($context['sandbox']['progress'] / $context['sandbox']['items']);
}

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
          islandora_restricted_gather_pids($result_object->id, $query_choice, $yum);
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
  dsm($target_pid, 'update target');
  module_load_include('inc', 'islandora_restricted', 'includes/utilities');
  $target_object = islandora_object_load($target_pid);
  $source_object = islandora_object_load($source_pid);
  $applied_restrictions = islandora_restricted_retrieve_restrictions($source_object, FALSE);
  $current_restrictions = islandora_restricted_retrieve_restrictions($target_object, FALSE);
  if (!islandora_restricted_populated_restriction($current_restrictions)) {
    // Not one currently set, use the add function.
  }
  // Need to determine what's different and add on accordingly.
  else {

  }
  dsm($current_restrictions, 'current restrictions');
}
