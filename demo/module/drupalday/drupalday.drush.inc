<?php

/**
 * @file
 */

/**
 * Implements hook_drush_command().
 */
function drupalday_drush_command() {

  $items['dd-import'] = array(
    'callback' => '_drupalday_import',
    'description' => 'Importar contenidos en ejemplo Drupalday.',
    'aliases' => array('ddi'),
    'arguments' => array(
      'input' => 'Fichero JSON con datos a cargar.',
    ),
    'options' => array(
      'assets' => 'Ruta al directorio de assets (files e images)',
    ),
  );

  return $items;
}

/**
 * Importar as páxinas de calquera sección
 *
 *   Scrapy:
 *     scrapy crawl sitepoint -o drupal.json --logfile=drupal.log
 *   Drush:
 *     drush ddi drupal.json --assets=./assets
 *
 * @param $input
 * @param $assets
 * @return bool
 */
function _drupalday_import($input) {

  if (!file_exists($input)) {
    return drush_set_error('FILE_NOT_FOUND', sprintf('El fichero %s no existe', $input));
  }

  $assets = drush_get_option('assets', './assets');
  if(!is_dir($assets)) {
    return drush_set_error('ASSETS_NOT_FOUND', sprintf('El directorio de assets %s no existe', $assets));
  }

  $data = file_get_contents($input);
  $data = json_decode($data, True);

  if (!is_array($data)) {
    return drush_set_error('IMPORT_DATA_CORRUPTED', 'Los datos importados están corruptos');
  }

  // Definir por si existen limitaciones en el servidor
  ini_set('memory_limit', '256M');

  // Crear los nodos
  foreach($data as $item) {
    // <editor-fold desc="values">
    $values = [
      'type'     => 'page',
      'title'    => $item['title'],
      'langcode' => 'und',
      'status'   => 1,
      'promote'  => 0,
      'sticky'   => 0,
      'uid'      => 1,

      'body'     => [
        'value'   => $item['body'],
        'summary' => '',
        'format'  => 'full_html', //
      ],

      'created' => \time(),
      'changed' => \time(),
    ];
    // </editor-fold>

    // <editor-fold desc="field_fe_image">
    if(isset($item['images']) && count($item['images']) > 0) {
      $dir = 'public://full';
      if(!file_prepare_directory($dir)) {
        drupal_mkdir($dir, NULL, True);
      }

      foreach($item['images'] as $img) {
        /**
         * assets
         * ├── files
         * │   └── full
         * └── images
         *     └── full
         */

        $path = 'public://' . $img['path'];

        $imagename = realpath($assets . '/images/' . $img['path']);
        if(!file_exists($imagename)) {
          drush_log('Imagen ' . $imagename . ' no existe.', 'error');
          continue;
        }

        $image_stream = file_get_contents($imagename);
        $file = file_save_data($image_stream, $path, FILE_EXISTS_REPLACE);

        $wrapper = \Drupal::service('stream_wrapper_manager')->getViaUri($path);
        $externalUrl = $wrapper->getExternalUrl();

        // Si se exjecuta sin alias no puede determinar la baseUrl
        if(preg_match('@^http://default@', $externalUrl)) {
          $externalUrl = preg_replace('@^http://default@', '', $externalUrl);
        }

        // Remprazar nos contidos
        $values['body']['value'] = preg_replace_callback(
          '@<img(?P<pre>[^>]*?)src=["\']' . preg_quote($img['url']) . '["\'](?P<post>[^>]*?)>@',
          function($matches) use($externalUrl) {
            $attrs = preg_replace('/ alt=[\"\'].*?[\"\']/', '', $matches['pre'] . ' ' . $matches['post']);
            $img = '<img src="' . $externalUrl . '" alt="" '. $attrs . ' />';
            return preg_replace('/[ ]{2,}/', ' ', $img);
          },
          $values['body']['value']
        );
      }
    }
    // </editor-fold>

    $node = \Drupal\node\Entity\Node::create($values);
    $node->save();
  }

  return true;
}
