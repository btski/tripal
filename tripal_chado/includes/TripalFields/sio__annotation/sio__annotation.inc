<?php

class sio__annotation extends ChadoField {


  // --------------------------------------------------------------------------
  //                     EDITABLE STATIC CONSTANTS
  //
  // The following constants SHOULD be set for each descendent class.  They are
  // used by the static functions to provide information to Drupal about
  // the field and it's default widget and formatter.
  // --------------------------------------------------------------------------

  // The default lable for this field.
  public static $default_label = 'Annotations';

  // The default description for this field.
  public static $description = 'This record is annotated with controlled vocabulary terms.';

  // Provide a list of instance specific settings. These can be access within
  // the instanceSettingsForm.  When the instanceSettingsForm is submitted
  // then Drupal with automatically change these settings for the instnace.
  // It is recommended to put settings at the instance level whenever possible.
  // If you override this variable in a child class be sure to replicate the
  // term_name, term_vocab, term_accession and term_fixed keys as these are
  // required for all TripalFields.
  public static $default_instance_settings  = array(
    // The short name for the vocabulary (e.g. shcema, SO, GO, PATO, etc.).
    'term_vocabulary' => 'SIO',
    // The name of the term.
    'term_name' => 'annotation',
    // The unique ID (i.e. accession) of the term.
    'term_accession' => '001166',
    // Set to TRUE if the site admin is allowed to change the term
    // type. This will create form elements when editing the field instance
    // to allow the site admin to change the term settings above.
    'term_fixed' => FALSE,
  );

  // The default widget for this field.
  public static $default_widget = 'chado_linker__cvterm_widget';

  // The default formatter for this field.
  public static $default_formatter = 'chado_linker__cvterm_formatter';

  // A boolean specifying that users should not be allowed to create
  // fields and instances of this field type through the UI. Such
  // fields can only be created programmatically with field_create_field()
  // and field_create_instance().
  public static $no_ui = FALSE;


  public function validate($entity_type, $entity, $langcode, $items, &$errors) {

    $field_name = $this->field['field_name'];

    foreach ($items as $delta => $item) {
      // Get the term that matches.
      $cvterm_name = $item['cvterm_name'];
      $cv_id = $item['cv_id'];
      if($cvterm_name and $cv_id) {
        $cvterm = chado_generate_var('cvterm', array(
          'cv_id' => $cv_id,
          'name' => $cvterm_name,
        ));
        if (!$cvterm) {
          $errors[$field_name][$langcode][$delta][] = array(
            'message' =>  t("Cannot find a term that matches the term name and vocabulary."),
            'error' => 'cvterm_name'
          );
        }
      }
    }
  }

  /**
   * @see TripalField::elementInfo()
   */
  public function elementInfo() {
    $field_table = $this->instance['settings']['chado_table'];
    $schema = chado_get_schema($field_table);

    $vocabulary_term = tripal_get_chado_semweb_term('cvterm', 'cv_id');
    $accession_term = tripal_get_chado_semweb_term('dbxref', 'accession');
    $definition_term = tripal_get_chado_semweb_term('cvterm', 'definition');

    $field_term = $this->getFieldTermID();
    $info = array(
      $field_term => array(
        'operations' => array(),
        'sortable' => FALSE,
        'searchable' => FALSE,
        'type' => 'array',
        'elements' => array(
          $vocabulary_term => array(
            'sortable' => TRUE,
            'searchable' => TRUE,
            'name' => 'vocabulary',
            'label' => 'Annotation Term Vocabulary',
            'operations' => array('eq', 'ne', 'contains', 'starts'),
          ),
          $accession_term => array(
            'sortable' => TRUE,
            'searchable' => TRUE,
            'name' => 'accession',
            'label' => 'Annotation Term Accession',
            'operations' => array('eq', 'ne', 'contains', 'starts'),
          ),
          $definition_term = array(
            'sortable' => TRUE,
            'searchable' => TRUE,
            'name' => 'description',
            'label' => 'Annotation Term Description',
            'operations' => array('eq', 'ne', 'contains', 'starts'),
          ),
        ),
      ),
    );

    if (array_key_exists('is_not', $schema['fields'])) {
      $negation_term = tripal_get_chado_semweb_term($field_table, 'is_not');
      $info[$field_term]['elements'][$negation_term] = array(
        'sortable' => TRUE,
        'searchable' => TRUE,
        'name' => 'negates',
        'label' => 'Annotation Term Negates',
        'operations' => array('eq', 'ne'),
      );
    }
    if (array_key_exists('rank', $schema['fields'])) {
      $rank_term = tripal_get_chado_semweb_term($field_table, 'rank');
      $info[$field_term]['elements'][$rank_term] = array(
        'sortable' => TRUE,
        'searchable' => TRUE,
        'name' => 'rank',
        'label' => 'Annotation Term Rank',
        'operations' => array('eq', 'ne', 'lte', 'gte'),
        'type' => numeric
      );
    }
    if (array_key_exists('pub_id', $schema['fields'])) {
    }
    return $info;
  }

  /**
   * @see ChadoField::query()
   */
  public function query($query, $condition) {
    $alias = $this->field['field_name'];
    $operator = $condition['operator'];

    $field_table = $this->instance['settings']['chado_table'];
    $base_table = $this->instance['settings']['base_table'];

    $schema = chado_get_schema($field_table);
    $pkey = $schema['primary key'][0];
    $fkey_lcolumn = key($schema['foreign keys'][$base_table]['columns']);
    $fkey_rcolumn = $schema['foreign keys'][$base_table]['columns'][$fkey_lcolumn];

    $field_term_id = $this->getFieldTermID();

    $vocabulary_term = $field_term_id . ',' . tripal_get_chado_semweb_term('cvterm', 'cv_id');
    $accession_term = $field_term_id . ',' . tripal_get_chado_semweb_term('dbxref', 'accession');
    $definition_term = $field_term_id . ',' . tripal_get_chado_semweb_term('cvterm', 'definition');


    // Join to the xxx_cvterm table for this field.
    $this->queryJoinOnce($query, $field_table, $alias, "base.$fkey_rcolumn = $alias.$fkey_lcolumn");

    if ($condition['column'] == $vocabulary_term) {
      $this->queryJoinOnce($query, 'cvterm', $alias . '_cvterm', $alias . ".cvterm_id = " . $alias . "_cvterm.cvterm_id");
      $this->queryJoinOnce($query, 'dbxref', $alias . '_dbxref', $alias . "_cvterm.dbxref_id = " . $alias . "_dbxref.dbxref_id");
      $this->queryJoinOnce($query, 'db', $alias . '_db', $alias . "_db.db_id = " . $alias . "_dbxref.db_id");
      $query->condition($alias . '_db.name', $condition['value'], $operator);
    }
    if ($condition['column'] == $accession_term) {
      $this->queryJoinOnce($query, 'cvterm', $alias . '_cvterm', $alias . ".cvterm_id = " . $alias . "_cvterm.cvterm_id");
      $this->queryJoinOnce($query, 'dbxref', $alias . '_dbxref', $alias . "_cvterm.dbxref_id = " . $alias . "_dbxref.dbxref_id");
      $query->condition($alias . '_dbxref.accession', $condition['value'], $operator);
    }
    if ($condition['column'] == $definition_term) {
      $this->queryJoinOnce($query, 'cvterm', $alias . '_cvterm', $alias . ".cvterm_id = " . $alias . "_cvterm.cvterm_id");
      $query->condition($alias . '_cvterm.definition', $condition['value'], $operator);
    }

    if (array_key_exists('is_not', $schema['fields'])) {
      $negation_term = $field_term_id . ',' . tripal_get_chado_semweb_term($field_table, 'is_not');
      if ($condition['column'] == $negation_term) {
        $query->condition($alias . '.is_not', $condition['value'], $operator);
      }
    }
    if (array_key_exists('rank', $schema['fields'])) {
      $rank_term = $field_term_id . ',' . tripal_get_chado_semweb_term($field_table, 'rank');
      if ($condition['column'] == $rank_term) {
        $query->condition($alias . '.rank', $condition['value'], $operator);
      }
    }
    if (array_key_exists('pub_id', $schema['fields'])) {
    }
  }

  /**
   * @see ChadoField::queryOrder()
   */
  public function queryOrder($query, $order) {
    $alias = $this->field['field_name'];

    $field_table = $this->instance['settings']['chado_table'];
    $base_table = $this->instance['settings']['base_table'];

    $schema = chado_get_schema($field_table);
    $pkey = $schema['primary key'][0];
    $fkey_lcolumn = key($schema['foreign keys'][$base_table]['columns']);
    $fkey_rcolumn = $schema['foreign keys'][$base_table]['columns'][$fkey_lcolumn];

    $field_term_id = $this->getFieldTermID();

    $vocabulary_term = $field_term_id . ',' . tripal_get_chado_semweb_term('cvterm', 'cv_id');
    $accession_term = $field_term_id . ',' . tripal_get_chado_semweb_term('dbxref', 'accession');
    $definition_term = $field_term_id . ',' . tripal_get_chado_semweb_term('cvterm', 'definition');

    // Join to the xxx_cvterm table for this field.
    $this->queryJoinOnce($query, $field_table, $alias, "base.$fkey_rcolumn = $alias.$fkey_lcolumn");

    if ($order['column'] == $vocabulary_term) {
      $this->queryJoinOnce($query, 'cvterm', $alias . '_cvterm', $alias . ".cvterm_id = " . $alias . "_cvterm.cvterm_id");
      $this->queryJoinOnce($query, 'dbxref', $alias . '_dbxref', $alias . "_cvterm.dbxref_id = " . $alias . "_dbxref.dbxref_id");
      $this->queryJoinOnce($query, 'db', $alias . '_db', $alias . "_db.db_id = " . $alias . "_dbxref.db_id");
      $query->orderBy($alias . "_dbxref.name", $order['direction']);
    }
    if ($order['column'] == $accession_term) {
      $this->queryJoinOnce($query, 'cvterm', $alias . '_cvterm', $alias . ".cvterm_id = " . $alias . "_cvterm.cvterm_id");
      $this->queryJoinOnce($query, 'dbxref', $alias . '_dbxref', $alias . "_cvterm.dbxref_id = " . $alias . "_dbxref.dbxref_id");
      $query->orderBy($alias . "_dbxref.accession", $order['direction']);
    }
    if ($order['column'] == $definition_term) {
      $this->queryJoinOnce($query, 'cvterm', $alias . '_cvterm', $alias . ".cvterm_id = " . $alias . "_cvterm.cvterm_id");
      $query->orderBy($alias . "_cvterm.definition", $order['direction']);
    }
    if (array_key_exists('is_not', $schema['fields'])) {
      $negation_term = $field_term_id . ',' . tripal_get_chado_semweb_term($field_table, 'is_not');
      if ($order['column'] == $negation_term) {
        $query->orderBy("$alias.is_not", $order['direction']);
      }
    }
    if (array_key_exists('rank', $schema['fields'])) {
      $rank_term = $field_term_id . ',' . tripal_get_chado_semweb_term($field_table, 'rank');
      if ($order['column'] == $rank_term) {
        $query->orderBy("$alias.rank", $order['direction']);
      }
    }
    if (array_key_exists('pub_id', $schema['fields'])) {
    }

  }
  /**
   *
   * @see TripalField::load()
   */
  public function load($entity) {
    $field_name = $this->field['field_name'];
    $field_type = $this->field['type'];
    $field_table = $this->instance['settings']['chado_table'];
    $field_column = $this->instance['settings']['chado_column'];
    $base_table = $this->instance['settings']['base_table'];

    // Get the FK that links to the base record.
    $schema = chado_get_schema($field_table);
    $pkey = $schema['primary key'][0];
    $fkey_lcolumn = key($schema['foreign keys'][$base_table]['columns']);
    $fkey_rcolumn = $schema['foreign keys'][$base_table]['columns'][$fkey_lcolumn];

    $vocabulary = tripal_get_chado_semweb_term('cvterm', 'cv_id');
    $accession = tripal_get_chado_semweb_term('dbxref', 'accession');
    $definition = tripal_get_chado_semweb_term('cvterm', 'definition');
    if (array_key_exists('is_not', $schema['fields'])) {
      $negation = tripal_get_chado_semweb_term($field_table, 'is_not');
    }
    if (array_key_exists('rank', $schema['fields'])) {
      $rank_term = tripal_get_chado_semweb_term($field_table, 'rank');
    }

    // Set some defaults for the empty record.
    $chado_record = $entity->chado_record;
    $entity->{$field_name}['und'][0] = array(
      'value' => '',
      'chado-' . $field_table . '__' . $pkey => '',
      'chado-' . $field_table . '__' . $fkey_lcolumn => $chado_record->$fkey_rcolumn,
      'chado-' . $field_table . '__cvterm_id' => '',
    );
    if (array_key_exists('is_not', $schema['fields'])) {
      $entity->{$field_name}['und'][0]['chado-' . $field_table . '__is_not'] = '';
    }
    if (array_key_exists('rank', $schema['fields'])) {
      $entity->{$field_name}['und'][0]['chado-' . $field_table . '__rank'] = '';
    }
    if (array_key_exists('pub_id', $schema['fields'])) {
      $entity->{$field_name}['und'][0]['chado-' . $field_table . '__pub_id'] = '';
    }

    // Get the annotations associated with this base record for this fields type.
    $columns = array('*');
    $match = array(
      $fkey_lcolumn => $chado_record->$fkey_rcolumn,
    );
    $order_by = array($pkey => 'ASC');
    if (array_key_exists('rank', $schema['fields'])) {
      $order_by = array('rank' => 'ASC');
    }
    $options = array(
      'return_array' => TRUE,
      'order_by' => $order_by
    );
    $fcvterms = chado_select_record($field_table, $columns, $match, $options);
    for ($i = 0; $i < count($fcvterms); $i++) {
      $linker = $fcvterms[$i];
      $cvterm = chado_generate_var('cvterm', array('cvterm_id' => $linker->cvterm_id));
      $entity->{$field_name}['und'][$i] = array(
        'value' => array(
          $vocabulary => $cvterm->dbxref_id->db_id->name,
          $accession => $cvterm->dbxref_id->accession,
          $definition => $cvterm->name
        ),
        'chado-' . $field_table . '__' . $pkey => $linker->$pkey,
        'chado-' . $field_table . '__' . $fkey_lcolumn => $linker->$fkey_lcolumn,
        'chado-' . $field_table . '__' . 'cvterm_id' => $linker->cvterm_id,
      );
      if (array_key_exists('is_not', $schema['fields'])) {
        $entity->{$field_name}['und'][$i]['value'][$negation] = $linker->is_not;
        $entity->{$field_name}['und'][$i]['chado-' . $field_table . '__is_not'] = $linker->is_not;
      }
      if (array_key_exists('rank', $schema['fields'])) {
        $entity->{$field_name}['und'][$i]['value'][$rank_term] = $linker->rank;
        $entity->{$field_name}['und'][$i]['chado-' . $field_table . '__rank'] = $linker->rank;
      }
      if (array_key_exists('pub_id', $schema['fields'])) {
        $entity->{$field_name}['und'][$i]['chado-' . $field_table . '__pub_id'] = $linker->pub_id;
      }
    }
  }
}

/**
 * Theme function for the dbxref_id_widget.
 *
 * @param $variables
 */
function theme_chado_linker__cvterm_widget($variables) {
  $element = $variables['element'];

  // These two fields were added to the widget to help identify the fields
  // for layout.
  $table_name = $element['#table_name'];
  $fkey = $element['#fkey_field'];

  $layout = "
    <div class=\"annotation-cvterm-widget\">
      <div class=\"annotation-cvterm-widget-item\">" .
      drupal_render($element['cv__cv_id']) . "
      </div>
      <div class=\"annotation-cvterm-widget-item\">" .
      drupal_render($element['cvterm__name']) . "
      </div>
      <div class=\"annotation-cvterm-widget-item\">" .
      drupal_render($element['pub']) . "
      </div>
      <div class=\"annotation-cvterm-widget-item\">" .
      drupal_render($element['chado-' . $table_name . '__is_not']) . "
      </div>
    </div>
  ";
  return $layout;
}

/**
 * An Ajax callback for the dbxref widget.
 */
function chado_linker__cvterm_widget_form_ajax_callback($form, $form_state) {

  $field_name = $form_state['triggering_element']['#parents'][0];
  $delta = $form_state['triggering_element']['#parents'][2];


  return $form[$field_name]['und'][$delta];
}