<?php

/**
 * @file
 */

/**
 * Implements hook_drush_command().
 */
function gdgourense_drush_command() {

  $items['gdgourense-import'] = array(
    'callback' => '_gdgourense_import',
    'description' => 'Importar contidos de exemplo.',
    'arguments' => array(
      'input' => 'Ficheiro JSON cos datos a cargar.',
    ),
    'options' => array(
      'assets' => 'Ruta ao directorio de assets (files e images)',
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
 *     drush gdgourense-import drupal.json --assets=./assets
 *
 * @param $input
 * @param $assets
 * @return bool
 */
function _gdgourense_import($input) {

  if (!file_exists($input)) {
    return drush_set_error('FILE_NOT_FOUND', sprintf('O ficheiro %s non existe', $input));
  }

  $assets = drush_get_option('assets', './assets');
  if(!is_dir($assets)) {
    return drush_set_error('ASSETS_NOT_FOUND', sprintf('O directorio de assets %s non existe', $assets));
  }

  $data = file_get_contents($input);
  $data = json_decode($data, True);

  if (!is_array($data)) {
    return drush_set_error('IMPORT_DATA_CORRUPTED', 'Os datos importados están corruptos (sé fuerte!)');
  }

  // Definir por se existen limitacións no servidor
  ini_set('memory_limit', '256M');

  // Crear os nodos
  foreach($data as $item) {
    // <editor-fold desc="comúns">
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

    // <editor-fold desc="imaxe">
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
          drush_log('Imaxe ' . $imagename . ' non existe.', 'error');
          continue;
        }

        $image_stream = file_get_contents($imagename);
        $file = file_save_data($image_stream, $path, FILE_EXISTS_REPLACE);

        $wrapper = \Drupal::service('stream_wrapper_manager')->getViaUri($path);
        $externalUrl = $wrapper->getExternalUrl();

        // Si se executa sen alias non pode determinar a baseUrl
        if(preg_match('@^http://default@', $externalUrl)) {
          $externalUrl = preg_replace('@^http://default@', '', $externalUrl);
        }

        // Remprazar nos contidos
        $values['body']['value'] = preg_replace_callback(
          '@<img(?P<pre>[^>]*?)src=["\']' . preg_quote($img['url'], '@') . '["\'](?P<post>[^>]*?)>@',
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
