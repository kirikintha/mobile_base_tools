<?php
/**
 * @name mobileBaseTools
 * All functions go in here, to keep them out of the module.
 * @file class.module.inc
 */
class mobileBaseTools {

  /**
   * Variables.
   */
  public $settings;
  public $modulePath;
  public $ignoreAgents;
  public $os;
  public $crawlers;

  /**
   * Constructor.
   */
  function __construct() {
    //Module settings.
    $this->settings = variable_get('mobile_base_tools_settings', array());
    //Theme path settings.
    $this->modulePath  = MOBILE_BASE_TOOLS_MODULE_PATH;
    //User Agenents to ignore.
    $this->ignoreAgents = array(
      'ipad',
    );
    //Operating System User Agents to match.
    $this->os = array(
      'Windows',
      'Macintosh',
      'Linux',
      'FreeBSD',
      'Solaris',
    );
    //Crawlers.
    $this->crawlers = array(
      'bot',
      'slurp',
      'spider',
      'crawl'
    );
  }

  /**
   * Desctructor.
   */
  function __destruct() {
    unset( $this );
  }

  /**
   * @name _detectMobileDevice($ignoreAgents = array('ipad'))
   * Detect if a user agent is a mobile device or not. This uses the theory that we are not looking for devices, rather than looking for devices
   * based on: http://smartmobtoolkit.wordpress.com/2008/10/16/not-device-detection
   * @param array $ignoreUserAgents = an array of values to ignore in a useer agent to ignore, such as the ipad
   * @return bool true/false if a device is a mibile device or not
   */
  public static function _detectMobileDevice($vars = NULL, $ignoreAgents = NULL) {
    //Reset mobile vars.
    $vars['mobile'] = array();
    $manager = new mobileBaseTools();
    //If we do not supply an array of ignore agents, then use the default.
    $ignoreAgents = (is_array($ignoreAgents)) ? $ignoreAgents : $manager->ignoreAgents;
    //Operating Systems we want to ignore.
    $ignore   = array_merge($manager->os, $manager->crawlers);
    //Turn the ignore array into a piped preg_match query.
    $ignore = implode('|', $ignore);
    //If we match anything on the ignore list, and do not have mobile in the user agent string, return false.
    if (preg_match('/('.$ignore.')/i', $_SERVER['HTTP_USER_AGENT'], $matches) && !strstr(strtoupper($_SERVER['HTTP_USER_AGENT']), 'MOBILE')) {
      //Return Not a mobile device.
      $vars['mobile']['is_mobile_device'] = FALSE;
    } else {
      //If we do not match on the ignore list then we are presuming we are a mobile device, unless that mobile device is to be ignored.
      $ignoreAgents = implode('|', $ignoreAgents);
      if (preg_match('/('.$ignoreAgents.')/i', $_SERVER['HTTP_USER_AGENT'], $matches)) {
        //Returned Mobile device, ignored.
        $vars['mobile']['is_mobile_device'] = 'IGNORED';
      } else {
        //Return is a mobile device.
        $vars['mobile']['is_mobile_device'] = TRUE;
      }
      //Use WURFL if the module is enabled and exists.
      if (module_exists('wurfl')) {
        //Get the device capabilities from wurfl.
        global $wurfl_device;
        //Device Capabilities.
        $vars['mobile']['device_capabilities'] = $wurfl_device->capabilities;
        //Findthe resolution.
        $vars['mobile']['device_resolution']   = self::_getDeviceResolution($vars, $manager);
        //Tablet information.
        $vars['mobile']['is_tablet']           = $wurfl_device->capabilities['product_info']['is_tablet'];
      }
    }
    //Unset the manager object.
    unset($manager);
    //Return our new mobile variables, if any.
    return $vars;
  }

  public static function _loadJSSettings() {
    //Get our mobile variables, so we can also assign them to Drupal.settings
    $vars = self::_detectMobileDevice();
    //Render our Drupal.settings.
    drupal_add_js($vars, 'setting');
  }

  /**
   * @name _interceptPage($theme_info = FALSE)
   */
  public static function _interceptPage($vars, $theme_info = FALSE) {
    //If we have a valid theme object, delegate out our content changes.
    if (is_object($theme_info)) {
      //Get the theme settings for this mobile theme.
      $theme_settings = theme_get_settings($theme_info->name);
      if (is_array($theme_settings)) {
        //Set Meta Tags for mobile browsers.
        $vars = self::_interceptMeta($vars, $theme_settings);
        //Set Mobile Stylesheet.
        $vars = self::_interceptStyles($vars, $theme_settings);
        //Intercept the javascript to disallow any non-essential javascript, if the developer wishes.
        $vars = self::_interceptJS($vars, $theme_settings);
        //Remove Admin Menu entirely from $closure if set in the admin menu.
        $vars = self::_interceptClosure($vars, $theme_settings);
        //Change anything with an imagecache preset with mobile_image to it's proper dimension for this device for our regions.
        $vars = self::_interceptImageCache($vars, $theme_settings, $theme_info);
      }
      //Intercept body id and body classes and add appropriate $vars
      $vars = self::_interceptBody($vars, $theme_settings);
      //After we process the page, remove the theme settings array.
      unset($theme_settings);
    }
    //Return our changed variables.
    return $vars;
  }

  /**
   * @name _interceptBody($vars, $theme_settings = NULL)
   * @param <type> $vars
   * @param <type> $theme_settings 
   */
  public static function _interceptBody($vars, $theme_settings = NULL) {
    //Add Body ID to each page to keep pages unique.
    $vars['body_id'] = 'pid-' . strtolower(preg_replace('/[^a-zA-Z0-9-]+/', '-', drupal_get_path_alias($_GET['q'])));
    //If we do not have a "mobile" class added to our body, do so.
    if (!strstr($vars['body_classes'], 'page-mobile')) {
      $vars['body_classes'] .= ' page-mobile';
    }
    //If we have a tablet, then add that as a body class.
    if ($vars['mobile']['is_tablet'] === TRUE) {
      $vars['body_classes'] .= ' tablet';
    }
    //Add a model name as a body class.
    if ($vars['mobile']['device_capabilities']['product_info']['model_name']) {
      $vars['body_classes'] .= ' ' .strtolower($vars['mobile']['device_capabilities']['product_info']['model_name']);
    }
    //Set our body-class for our max-width of the page.
    if ($vars['mobile']['device_resolution']) {
      $vars['body_classes'] .= ' resolution-'.$vars['mobile']['device_resolution'];
    }
    return $vars;
  }

  /**
   * @name _interceptJS(&$vars)
   * Intercepts $vars['scripts'] and turns it into $['mobile_scripts'], removes unwanted JS and aggregates it into it's own separate js file.
   */
  public static function _interceptJS($vars, $theme_settings = NULL) {
    //If we are introducing our own aggregated js to the theme, then use this parser and aggregator.
    if ($theme_settings['intercept_js'] == 1) {
      //Restrict javascript to the essentials only!
      $scripts = drupal_add_js(NULL, NULL, NULL);
      //Since jquery update is a little wonky on when it replaces jquery, then we need to do the replacements here, since for some reason the page_process can fail.
      //Since this is taken straight from the jquery update module, if there are changes or whatever - this has to be updated too.
      if (variable_get('jquery_update_replace', TRUE)) {
        // Replace jquery.js first.
        $new_jquery = array(jquery_update_jquery_path() => $scripts['header']['core']['misc/jquery.js']);
        $scripts['header']['core'] = array_merge($new_jquery, $scripts['header']['core']);
        unset($scripts['header']['core']['misc/jquery.js']);

         // Loop through each of the required replacements.
        foreach (jquery_update_get_replacements() as $type => $replacements) {
          foreach ($replacements as $find => $replace) {
            // If the file to replace is loaded on this page...
            if (isset($scripts[$type][$find])) {
              // Create a new entry for the replacement file, and unset the original one.
              $replace = JQUERY_UPDATE_REPLACE_PATH .'/'. $replace;
              $scripts[$type][$replace] = $scripts[$type][$find];
              unset($scripts[$type][$find]);
            }
          }
        }
      }
      //Look through our scripts to see if they are allowed.
      if (!empty($scripts['header'])) {
        //The scripts array has core, module, theme, settings etc as keys.
        $types = array('module');
        //Loop through each type we want to constrain and unset those variables.
        $files = array();
        foreach ($types as $type) {
          foreach ($scripts['header'][$type] as $key => $info) {
            if (in_array($key, $theme_settings['intercept_js_excludes'])) {
              //If our script coming through is in the exclusion list, then we need to unset the js we do not want.
              unset($scripts['header'][$type][$key]);
            } else {
              //Set up our js files for aggregation.
              if ($info['preprocess']) {
                $files[$key] = $info;
                unset($scripts['header'][$type][$key]);
              }
            }
          }
        }
        //The only thing I want this to do is aggregate.
        if (count($files) > 0) {
          $filename = 'js_mobile_'.  md5(serialize($files)).'.js';
          //@todo something needs to go here to make sure that the files are caching if the javscript changes and needs ot be re-cached exist.
          $preprocess_file = drupal_build_js_cache($files, $filename);
          $info = array(
            'cache'      => TRUE,
            'defer'      => FALSE,
            'preprocess' => FALSE,
          );
          $scripts['header']['theme'][$preprocess_file] = $info;
        }
        //Return our mobile scripts.
        $vars['mobile_scripts'] = drupal_get_js('header', $scripts['header']);
      }
    } else {
      //If we do not have any overrides, then just return the regular scripts to the page.
      $vars['mobile_scripts'] = $vars['scripts'];
    }
    return $vars;
  }

  /**
   * @name _interceptMeta(&$vars)
   */
  public static function _interceptMeta($vars, $theme_settings = NULL) {
    //Get our theme settings and use the meta tags in here.
    $meta = $theme_settings['intercept_meta_tags'];
    $tags = array();
    if (count($meta) > 0 ) {
      foreach ($meta as $tag) {
        $tags[] = '<meta name="'.trim($tag['meta_name']).'" content="'.trim($tag['meta_content']).'" />';
      }
      $vars['mobile_head'] = $vars['head'] .implode("\n", $tags) ."\n";
    }
    //Return $vars
    return $vars;
  }

  /**
   * @name _interceptClosure(&$vars)
   * Since the admin module tacks their markup onto the footer javascript, we can eliminate it in it's entirety by just outputing the footer scripts.
   */
  public static function _interceptClosure($vars, $theme_settings = NULL) {
    //If we are overriding the admin_menu, then only show the footer scripts.
    if ($theme_settings['intercept_admin_menu'] == 1) {
      $vars['mobile_closure'] = drupal_get_js('footer', $scripts['footer']);
    } else {
      //Else, use the native closure variable, plus add in the proper css and js for the admin menu.
      //kpr($vars);
      //Get the admin menu module css only as defined in the .info file. This will also add in any additional admin menu module styles as well.
      foreach ($vars['css']['all']['module'] as $file => $status) {
        //kpr($file);
        if (preg_match('/(admin-menu|admin_menu)\.css$/i', $file, $matches)) {
          $vars['mobile_styles'] .= '<link type="text/css" rel="stylesheet" media="all" href="/' .$file. '" />' . "\n" ;
        }
      }
      //Add Admin Menu JS.
      $vars['mobile_closure'] = $vars['closure'];
    }
    return $vars;
  }

  /**
   * @name _interceptStyles($vars, $theme_settings)
   * Get this theme's stylesheet.
   * @todo - Need to suss out how to get the mobile_styles to have the admin_menu css and js added back in as well.
   */
  public static function _interceptStyles($vars, $theme_settings = NULL) {
    $vars['mobile_styles'] = '';
    //Get any additional links assigned in the theme-settings and use that as a link.
    $links = $theme_settings['intercept_link_tags'];
    $tags = array();
    if (count($links) > 0 ) {
      foreach ($links as $tag) {
        $tags[] = '<link rel="'.trim($tag['link_rel']).'" href="' .trim($tag['link_href']). '" />';
      }
      $vars['mobile_styles'] .= implode("\n", $tags) ."\n";
    }
    //Get the mobile theme css only as defined in the .info file.
    foreach ($vars['css']['all']['theme'] as $file => $status) {
      if ($status === TRUE) {
        $vars['mobile_styles'] .= '<link type="text/css" rel="stylesheet" media="all" href="/' .$file. '" />' . "\n" ;
      }
    }
    return $vars;
  }

  /**
   * @name _getDeviceResolution($vars)
   * @param <type> $vars
   * @return <type>
   */
  public static function _getDeviceResolution($vars, $manager = NULL) {
    //Get the array we are trying to intersect.
    $ranges = $manager->settings['resolutions']['thresholds']['ranges'];
    //Set a range for resolution width and max_iamge width, so we can find the right number.
    $range  = range($vars['mobile']['device_capabilities']['display']['max_image_width'],$vars['mobile']['device_capabilities']['display']['resolution_width']);
    //In our range, find the array value that comes closes to the dimensions we define.
    $number = max(array_intersect($range,$ranges));
    //Get the resolution form the array.
    $resolutions = $manager->settings['resolutions']['thresholds']['lookup'];
    return (empty($number)) ? $resolutions[320] : $resolutions[$number];
  }

  /**
   * @name _interceptImageCache($vars, $theme_settings = NULL, $theme_info = NULL)
   * Looks for mobile_images and resizes them according to the current dimensions of the mobile device.
   */
  public static function _interceptImageCache($vars, $theme_settings = NULL, $theme_info = NULL) {
    //If we hav a mobile device, let it use the wurlf's image detection.
    if (self::_detectMobileDevice() !== FALSE) {
      //If we are a mobile device, and that device has any display properties.
      if (is_numeric($vars['mobile']['device_capabilities']['display']['max_image_width']) && is_numeric($vars['mobile']['device_capabilities']['display']['resolution_width'])) {
        //If we have regions, then we will auto-replace our mobile_image presets with the proper preset.
        if (!empty($theme_info->info['regions'])) {
          //Loop through our regions and report back their images.
          foreach ($theme_info->info['regions'] as $region) {
            //preg_replace our mobile_image with our new image resolution.
            $search = '/\/mobile_image_[0-9]+x[0-9]+\//';
            if (preg_match($search, $vars[$region], $matches)) {
              //If we match our search, then string replace everything with our new boundary.
              $find = array(
                'mobile_image_',
                '/',
              );
              $replace = array('','');
              //Get Dimensions.
              $dimensions = explode("x",str_replace($find,$replace,$matches[0]));
              //Set our replacement string.
              $resolutions = explode("x",str_replace($find,$replace,$vars['mobile']['device_resolution']));
              //Replace file path and width height for the image to the correct size.
              $search      = array(
                //%dimension x %dimension.
                $dimensions[0] .'x',
                'x' .$dimensions[1],
                //Width & Height.
                'width="' .$dimensions[0],
                'height="' .$dimensions[1],
              );
              $replace     = array(
                //%resolution x %resolution.
                $resolutions[0] .'x',
                'x' .$resolutions[1],
                //Width & Height.
                'width="' .$resolutions[0],
                'height="' .$resolutions[1],
              );
              //Replace our images.
              $vars[$region] = str_replace($search, $replace, $vars[$region]);
            }
          }
        }
      }
    }
    //Return our variables.
    return $vars;
  }
}


/**
 * mobileBaseToolsAdmin
 * This houses the theme-settings.php $form
 */
class mobileBaseToolsAdmin extends mobileBaseTools {

  /**
   * @name _getAdminForm($form = array());
   * @param <type> $form
   * @return <type>
   */
  public static function _getAdminForm($saved_settings, $theme_name, $info_theme_settings) {
    //UI Buttons.
    $buttons = array(
      '!add_button'    => '<div class="btn-add-me"><button type="button" class="add-me" title="Add" alt="Add A New Row.">Add</button><p>Add A New Row</p></div>',
      '!delete_button' => '<button type="button" class="delete-me" title="Delete" alt="Delete this Row.">Delete</button>',
    );
    //Form settings for mobile parameters.
    $form['mobile_settings'] = array(
      '#type'        => 'fieldset',
      '#title'       => t('Mobile Settings & Optimizations', array('@name' => $theme_name)),
      '#description' => t('Use this theme to set your mobile parameters.'),
      '#collapsible' => TRUE,
      '#collapsed'   => FALSE,
    );
    //Meta Tags.
    $form['mobile_settings']['meta_tags'] = array(
      '#type'        => 'fieldset',
      '#title'       => t('Meta tags'),
      '#description' => t('Add, modify or delete Meta tags for this theme.'),
      '#collapsible' => TRUE,
      '#collapsed'   => FALSE,
    );
    //Add Button.
    $form['mobile_settings']['meta_tags']['add'] = array(
      '#value' => t('!add_button',$buttons),
    );
    //Meta Tags can be added here. If there are no saved settings for this option, only one form field displays.
    $meta_tags = array(
      0 => array(
        'meta_name'    => 'viewport',
        'meta_content' => 'width=device-width; initial-scale=1.0; maximum-scale=1.0; user-scalable=1;',
      ),
      1 => array(
        'meta_name'    => 'MobileOptimized',
        'meta_content' => 'width',
      ),
      2 => array(
        'meta_name'    => 'HandheldFriendly',
        'meta_content' => 'true',
      ),
      3 => array(
        'meta_name'    => 'format-detection',
        'meta_content' => 'telephone=no',
      ),
    );
    $saved_settings['intercept_meta_tags'] = (empty($saved_settings['intercept_meta_tags'])) ? $meta_tags : $saved_settings['intercept_meta_tags'];
    foreach ($saved_settings['intercept_meta_tags'] as $key => $value) {
      if (!empty($value['meta_name']) && !empty($value['meta_content'])) {
        //If we have both a name and content, apply the meta tag
        $form['mobile_settings']['meta_tags']['element_meta_tags_' .$key] = array(
          '#type'          => 'meta_tags',
          '#default_value' => array(
            'meta_name'    => $value['meta_name'],
            'meta_content' => $value['meta_content'],
          ),
          '#element_validate' => array('_mobile_settings_submit'),
        );
      }
    }
    //Additional Stylesheet implementations espeically, for apple mobile <link rel="apple-touch-icon" href="apple-touch-icon.png" />.
    $form['mobile_settings']['mobile_styles'] = array(
      '#type'        => 'fieldset',
      '#title'       => t('Mobile Styles'),
      '#description' => t(''),
      '#collapsible' => TRUE,
      '#collapsed'   => FALSE,
    );
    //Add Button.
    $form['mobile_settings']['mobile_styles']['add'] = array(
      '#value' => t('!add_button',$buttons),
    );
    //Meta Tags can be added here. If there are no saved settings for this option, only one form field displays.
    $link_tags = array(
      0 => array(
        'link_rel'  => 'apple-touch-icon',
        'link_href' => '/' .$theme_path .'/apple-touch-icon.png',
      ),
    );
    $saved_settings['intercept_link_tags'] = (empty($saved_settings['intercept_link_tags'])) ? $link_tags : $saved_settings['intercept_link_tags'];
    foreach ($saved_settings['intercept_link_tags'] as $key => $value) {
      if (!empty($value['link_rel']) && !empty($value['link_href'])) {
        //If we have both a name and content, apply the link tag.
        $form['mobile_settings']['mobile_styles']['element_link_tags_' .$key] = array(
          '#type'          => 'link_tags',
          '#default_value' => array(
            'link_rel'  => $value['link_rel'],
            'link_href' => $value['link_href'],
          ),
          '#element_validate' => array('_mobile_settings_submit'),
        );
      }
    }
    //Javascript.
    $form['mobile_settings']['mobile_js'] = array(
      '#type'        => 'fieldset',
      '#title'       => t('Mobile Javascript Settings'),
      '#description' => t(''),
      '#collapsible' => TRUE,
      '#collapsed'   => FALSE,
    );
    //Javascript settings. You can turn on or turn off javascript interception, and also set what javascript you want to ignore.
    $form['mobile_settings']['mobile_js']['intercept_js'] = array(
      '#type'          => 'checkbox',
      '#title'         => t('Aggregate Javascript into $vars[\'mobile_scripts\'] and remove unwanted scripts from the mobile theme. All javascript is enabled by default. Un-Check a specific checkbox to omit that file form being loaded.'),
      '#default_value' => $saved_settings['intercept_js'],
      '#description'   => t('By checking this box, you will intercept the $vars[\'scripts\'] variable, remove unwanted javascript, aggregate it and return the variable $vars[\'mobile_scripts\']. This has the effect of making one js request in the header, but also optimizes the javascript to be leaner for mobile browsers. NOTE: this onnly works for javascript set in the header.'),
      '#prefix'        => '<h2>Javascript Aggregation</h2>',
    );
    /**
     * Javascript file exclusion
     * This denies any javascript we do not want to load to the page via theme UI.
     * @todo - I know this code can get compacted, I just wanted ot make sure I could suss out how it works.
     */
    //Core js files.
    $core_files   = file_scan_directory('modules', '.js', array('.', '..', 'CVS', 'drupal.js', 'jquery.js'));
    //kpr($core_files);
    $form['mobile_settings']['mobile_js']['core'] = array(
      '#type'        => 'fieldset',
      '#title'       => t('Core Javascript Files. @count files', array('@count' => count($core_files))),
      '#description' => t(''),
      '#collapsible' => TRUE,
      '#collapsed'   => TRUE,
    );
    //Reset the files array.
    $files = array();
    foreach ($core_files as $key => $file) {
      $files[$file->filename] = t('@name - @file', array('@name' => $file->name, '@file' => $file->filename));
    }
    $form['mobile_settings']['mobile_js']['core']['element_js_excludes_core'] = array(
      '#type'          => 'checkboxes',
      '#options'       => $files,
      '#default_value' => !empty($saved_settings['element_js_excludes_core']) ? $saved_settings['element_js_excludes_core'] : array_keys($files),
    );
    //Misc Drupal Files.
    $misc_files   = file_scan_directory('misc', '.js');
    $form['mobile_settings']['mobile_js']['misc'] = array(
      '#type'        => 'fieldset',
      '#title'       => t('Misc Javascript Files. @count files', array('@count' => count($misc_files))),
      '#description' => t(''),
      '#collapsible' => TRUE,
      '#collapsed'   => TRUE,
    );
    //Reset the files array.
    $files = array();
    foreach ($misc_files as $key => $file) {
      $files[$file->filename] = t('@name - @file', array('@name' => $file->name, '@file' => $file->filename));
    }
    $form['mobile_settings']['mobile_js']['misc']['element_js_excludes_misc'] = array(
      '#type'          => 'checkboxes',
      '#options'       => $files,
      '#default_value' => !empty($saved_settings['element_js_excludes_misc']) ? $saved_settings['element_js_excludes_misc'] : array_keys($files),
    );
    //Sacn Library js files if the directory exists.
    $dir = 'sites/all/libraries';
    $success = file_check_directory($dir, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS);
    if ($success) {
      $libraries_files   = file_scan_directory('sites/all/libraries', '.js');
      //kpr($libraries_files);
      $form['mobile_settings']['mobile_js']['libraries'] = array(
        '#type'        => 'fieldset',
        '#title'       => t('Library Files. @count files', array('@count' => count($libraries_files))),
        '#description' => t(''),
        '#collapsible' => TRUE,
        '#collapsed'   => TRUE,
      );
      //Reset the files array.
      $files = array();
      foreach ($libraries_files as $key => $file) {
        $files[$file->filename] = t('@name - @file', array('@name' => $file->name, '@file' => $file->filename));
      }
      $form['mobile_settings']['mobile_js']['libraries']['element_js_excludes_libraries'] = array(
        '#type'          => 'checkboxes',
        '#options'       => $files,
        '#default_value' => !empty($saved_settings['element_js_excludes_libraries']) ? $saved_settings['element_js_excludes_libraries'] : array_keys($files),
      );
    }
    //Module js files.
    $form['mobile_settings']['mobile_js']['modules'] = array(
      '#type'        => 'fieldset',
      '#title'       => t('Module Javascript Files.'),
      '#description' => t(''),
      '#collapsible' => TRUE,
      '#collapsed'   => TRUE,
    );
    // Get current list of modules from the current cache.
    $modules = module_rebuild_cache();
    uasort($modules, 'system_sort_modules_by_info_name');
    //Toggle UI. This does not necessarily need to be here, but is kinda hard to place inthe code.
    $toggle_options['attributes'] = array('class' => 'settings-toggle all-on');
    $toggle_link = l(t('Toggle All On'), $_GET['q'], $toggle_options);
    //Loop Through Modules.
    foreach ($modules as $name => $module) {
      //If a module is active, scan the directory for each javscript file and make that into a disable list.
      if ($module->status == 1 && $module->type == 'module') {
        //Scan for js files.
        $path = drupal_get_path('module', $module->name);
        $module_files = file_scan_directory($path, '.js');
        if (count($module_files) > 0) {
          //Make a module fieldset
          $form['mobile_settings']['mobile_js']['modules'][$name] = array(
            '#type'        => 'fieldset',
            '#title'       => t('@name @count files', array('@name' => $module->info['name'], '@count' => count($module_files))),
            '#description' => t('@description', array('@description' => $module->info['description'])),
            '#collapsible' => TRUE,
            '#collapsed'   => TRUE,
          );
          //Toggle All.
          $form['mobile_settings']['mobile_js']['modules'][$name][$name .'_toggle'] = array(
            '#type'       => 'markup',
            '#value'      => $toggle_link,
            '#weight'     => -100,
          );
          //Reset the files array.
          $files = array();
          //Populate module files.
          foreach ($module_files as $key => $file) {
            $files[$file->filename] = t('@name - @file', array('@name' => $file->name, '@file' => $file->filename));
          }
          $form['mobile_settings']['mobile_js']['modules'][$name]['element_js_excludes_modules_'.$name] = array(
            '#type'          => 'checkboxes',
            '#options'       => $files,
            '#default_value' => !empty($saved_settings['element_js_excludes_modules_'.$name]) ? $saved_settings['element_js_excludes_modules_'.$name] : array_keys($files),
          );
          //Unset our files.
          unset($files);
        }
        //Unset our module files
        unset($module_files);
      }
    }
    //Toggle Link for all remaining directories.
    $form['mobile_settings']['mobile_js']['libraries']['libraries_toggle'] =
    $form['mobile_settings']['mobile_js']['misc']['misc_toggle'] =
    $form['mobile_settings']['mobile_js']['core']['core_toggle'] = array(
      '#type'       => 'markup',
      '#value'      => $toggle_link,
      '#weight'     => -100,
    );
    //Disable, enable the admin_menu if is there. This will remove the admin menu from the $closure variable.
    if (module_exists('admin_menu')) {
      $form['mobile_settings']['mobile_admin'] = array(
        '#type'        => 'fieldset',
        '#title'       => t('Mobile Javascript Settings'),
        '#description' => t(''),
        '#collapsible' => TRUE,
        '#collapsed'   => FALSE,
      );
      $form['mobile_settings']['mobile_admin']['intercept_admin_menu'] = array(
        '#type'          => 'checkbox',
        '#title'         => t('Disable the admin menu from displaying in this theme.'),
        '#default_value' => $saved_settings['intercept_admin_menu'],
        '#description'   => t('By checking this box, you will intercept the $vars[\'closure\'] variable, remove the admin menu and return a variable called $vars[\'mobile_closure\']'),
        '#prefix'        => '<h2>Admin Menu</h2>',
      );
    }
    //Add in our own submit handler.
    $form['#submit'][] = '_mobile_settings_submit';
    //Return our form.
    return $form;
  }
}