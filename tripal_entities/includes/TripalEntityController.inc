<?php
/**
 * TripalEntityController extends DrupalDefaultEntityController.
 *
 * Our subclass of DrupalDefaultEntityController lets us add a few
 * important create, update, and delete methods.
 */
class TripalEntityController extends EntityAPIController {

  public function __construct($entityType) {
    parent::__construct($entityType);
  }

  /**
   * Create a Tripal data entity
   *
   * We first set up the values that are specific
   * to our data schema but then also go through the EntityAPIController
   * function.
   *
   * @param $type
   *   The machine-readable type of the entity.
   *
   * @return
   *   An object with all default fields initialized.
   */
  public function create(array $values = array()) {
    // Add some items to the values array passed to the constructor
    global $user;
    $values['uid'] = $user->uid;
    $values['created'] = time();
    $values['changed'] = time();
    $values['title'] = '';

    // The incoming values should have at a minimum the bundle_id;
    $bundle = $values['bundle'];
    $matches = array();
    if (preg_match('/dbxref_(.*)/', $bundle, $matches)) {
      $dbxref_id = $matches[1];
      $values['type'] = 'BioData';

      // Get the CVterm.
      $match = array('dbxref_id' => $dbxref_id);
      $cvterm = chado_generate_var('cvterm', $match);
      if ($cvterm) {
        $values['cvterm_id'] = $cvterm->cvterm_id;
      }
    }

    return parent::create($values);
  }

  /**
   * Delete a single entity.
   *
   * Really a convenience function for deleteMultiple().
   */
  public function delete($entity) {
    $transaction = db_transaction();
    try {
      // Invoke hook_entity_delete().
      module_invoke_all('entity_delete', $entity, $entity->type);
      field_attach_delete('BioData', $entity);

      db_delete('tripal_entity')
        ->condition('id', $entity->id)
        ->execute();
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('tripal_entities', $e);
      throw $e;
      return FALSE;
    }
    return TRUE;
  }

  /**
   * Sets the title for an entity.
   *
   * @param $entity
   * @param $title
   */
  public function setTitle($entity, $title) {
    db_update('tripal_entity')
      ->fields(array(
        'title' => $title
      ))
      ->condition('id', $entity->id)
      ->execute();
  }

  /**
   * Saves the custom fields using drupal_write_record().
   */
  public function save($entity) {
    global $user;
    $pkeys = array();

    $transaction  = db_transaction();
    try {
      // If our entity has no id, then we need to give it a
      // time of creation.
      if (empty($entity->id)) {
        $entity->created = time();
        $invocation = 'entity_insert';
      }
      else {
        $invocation = 'entity_update';
        $pkeys = array('id');
      }

      // Invoke hook_entity_presave().
      module_invoke_all('entity_presave', $entity, $entity->type);

      // Write out the entity record.
      $record = array(
        'cvterm_id' => $entity->cvterm_id,
        'type'      => $entity->type,
        'bundle'    => $entity->bundle,
        'title'     => $entity->title,
        'uid'       => $user->uid,
        'created'   => $entity->created,
        'changed'   => time(),
      );
      if ($invocation == 'entity_update') {
        $record['id'] = $entity->id;
      }
      $success = drupal_write_record('tripal_entity', $record, $pkeys);
      if ($success == SAVED_NEW) {
        $entity->id = $record['id'];
      }

      // Now we need to either insert or update the fields which are
      // attached to this entity. We use the same primary_keys logic
      // to determine whether to update or insert, and which hook we
      // need to invoke.
      if ($invocation == 'entity_insert') {
        field_attach_insert('BioData', $entity);
      }
      else {
        field_attach_update('BioData', $entity);
      }

      // Invoke either hook_entity_update() or hook_entity_insert().
      module_invoke_all('entity_postsave', $entity, $entity->type);
      module_invoke_all($invocation, $entity, $entity->type);

      return $entity;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('tripal_core', $e);
      drupal_set_message("Could not save the entity:" . $e->getMessage(), "error");
      return FALSE;
    }
  }
}