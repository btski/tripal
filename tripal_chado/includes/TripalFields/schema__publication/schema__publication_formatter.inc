<?php

class schema__publication_formatter extends TripalFieldFormatter {
  // The default lable for this field.
  public static $label = 'Publication';

  // The list of field types for which this formatter is appropriate.
  public static $field_types = array('schema__publication');

  // The list of default settings for this formatter.
  public static $settings = array();
  /**
   *
   * @see TripalFieldFormatter::settingsForm()
   */
  public function settingsForm($view_mode, $form, &$form_state) {

  }

  /**
   *
   * @see TripalFieldFormatter::view()
   */
  public function view(&$element, $entity_type, $entity, $langcode, $items, $display) {

  }
}