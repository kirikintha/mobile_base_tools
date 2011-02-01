/**
 * Module jQuery functions and Miscellaneous functions specific to this module.
 */

//If javascript is enabled
if (Drupal.jsEnabled) {
  //Variables
  var data, rows, row, target, output, copy, next;
  var items = {};
  //Set module namespace.
  Drupal.mobileBaseTools       = Drupal.mobileBaseTools || {};
  Drupal.mobileBaseTools.theme = Drupal.mobileBaseTools.theme || {};
  //Bind page events.
  Drupal.behaviors.mobileBaseToolsInit = function(context) {
    //Delete form field for meta tags.
    if($('button.delete-me').length){
      $('button.delete-me', context).each(function() {
        $(this).bind('click', Drupal.mobileBaseTools._del);
      });
    }
    //Add Form Field
    if($('button.add-me').length){
      $('button.add-me', context).each(function() {
        $(this).bind('click', Drupal.mobileBaseTools._add);
      });
    }
    //Toggle all module checkboxes on or off.
    if($('a.settings-toggle').length) {
      $('a.settings-toggle', context).each(function() {
        $(this).bind('click', Drupal.mobileBaseTools._toggle);
      });
    }
  }
  //Delete a row.
  Drupal.mobileBaseTools._del = function() {
    //Get the Rows already built.
    items  = Drupal.mobileBaseTools._find($(this));
    //Get the Rows already built.
    rows   = items.rows;
    //Find our target.
    target = items.target;
    //Find our target.
    if (rows.length > 1) {
      $(this).parents('.form-item').fadeOut(function() {
        $(this).remove();
        Drupal.theme('message', target, 'Row Deleted.', 'status');
      });
    } else {
      Drupal.theme('message', target, 'You must have at least one row, you cannot delete any more rows.', 'warning');
    }
  }
  //Add a row.
  Drupal.mobileBaseTools._add = function() {
    //Get the Rows already built.
    items  = Drupal.mobileBaseTools._find($(this));
    //Get the Rows already built.
    rows   = items.rows;
    //Get the row we are cloning.
    row    = rows[0];
    //Find our target.
    target = items.target;
    //Reset this target's messages.
    $(this).closest('div[class^=fieldset-]').find('.message').each(function() {$(this).remove();});
    //Generate the next number for a row.
    next = rows[rows.length-1].id;
    next = next.replace(/[a-zA-z\-]/ig, '');
    next = (parseInt(next) + 1);
    //Clone a unique form item.
    copy = $('#'+row.id).clone();
    copy.attr('id', copy.attr('id').replace(/[0-9]/ig, next));
    copy.find('.form-item, input').each(function() {
      //Replace out id's and name with our highest row value.
      $(this).attr('id', $(this).attr('id').replace(/[0-9]/ig, next));
    });
    copy.find('input').each(function() {
      //Replace out id's and name with our highest row value.
      $(this).attr('name', $(this).attr('name').replace(/[0-9]/ig, next));
      $(this).val('');
    });
    //Append our cloned form item.
    copy.appendTo(target);
    //Re-attach Drupal behaviors.
    Drupal.attachBehaviors(copy);
    //Set a status message.
    Drupal.theme('message', target, 'New Row Added.', 'status');
  }
  //Toggle this element and it's children.
  Drupal.mobileBaseTools._toggle = function () {
    //Check to see if we are selected or not, if we are, select all. If not deselect all but what was selected.
    var checked = $(this).closest('div[class^=fieldset-]').find('.form-checkbox:checked');
    var rows    = $(this).closest('div[class^=fieldset-]').find('.form-checkbox');
    if ($(this).hasClass('all-on')) {
      $(this).removeClass('all-on').text(Drupal.t('Toggle All Off'));
      rows.each(function() {
        $(this).attr('checked', true);
      });
    } else {
      $(this).addClass('all-on').text(Drupal.t('Toggle All On'));
      rows.each(function() {
        $(this).attr('checked', false);
      });
    }
    return false;
  }
  //Find a particular set of rows if they exist.
  Drupal.mobileBaseTools._find = function (manager) {
    //Clear out items objec,t so there is no cross contamination.
    items = {};
    //Get the rows we need from this item.
    items.rows   = manager.closest('div[class^=fieldset-]').children('.form-item');
    //Get the target for this operation.
    items.target = manager.closest('div[class^=fieldset-]');
    //Return our items.
    return items;
  }

  /**
   * Theme implenentations.
   */
  //Theme prototype message.
  Drupal.theme.prototype.message = function(target, text, status) {
    //Remove any messages set.
    target.find('.message').each(function() {$(this).remove();});
    //Set our new Message.
    target.append(Drupal.mobileBaseTools.theme.setMessage(Drupal.t(text), status));
    //Set our messages to fade out eventually.
    setTimeout(function() {
      //Set the timeout for our classes.
      $('.status, .warning').fadeOut();
    },'1500');
  }
  //Set a Drupal Message.
  Drupal.mobileBaseTools.theme.setMessage = function(text, status) {
    output = '';
    output += '<div class="message '+status+'"><p><h4>';
    output += text;
    output += '</h4></p></div>';
    return output;
  }
}