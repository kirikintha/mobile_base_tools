/**
 * Module jQuery functions and Miscellaneous functions specific to this module.
 */

//If javascript is enabled
if (Drupal.jsEnabled) {
  //Variables
  var rows, row, target, output, copy, next;
  //Set module namespace.
  Drupal.mobileThemeTools       = Drupal.mobileThemeTools || {};
  Drupal.mobileThemeTools.theme = Drupal.mobileThemeTools.theme || {};
  //Bind page events.
  Drupal.behaviors.mobileThemeToolsInit = function(context) {
    //Delete form field for meta tags.
    if($('button.delete-me').length){
      $('button.delete-me', context).each(function() {
        $(this).bind('click', Drupal.mobileThemeTools.del);
      });
    }
    //Add Form Field
    if($('button.add-me').length){
      $('button.add-me', context).each(function() {
        $(this).bind('click', Drupal.mobileThemeTools.add);
      });
    }
    //Toggle all module checkboxes on or off.
    if($('input.settings-toggle').length) {
      $('input.settings-toggle', context).each(function() {
        $(this).bind('click', Drupal.mobileThemeTools.toggle);
      });
    }
  }
  //Delete a row.
  Drupal.mobileThemeTools.del = function() {
    //Find out how many rows they have, and make sure that we have at least the defaults. This cannot be less than 4 rows.
    rows   = $(this).closest('.fieldset-content').children('.form-item');
    target = $(this).closest('.fieldset-content');
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
  Drupal.mobileThemeTools.add = function() {
    //Get the Rows already built.
    rows   = $(this).closest('.fieldset-content').children('.form-item');
    //Get the row we are cloning.
    row    = rows[0];
    //Find our target.
    target = $(this).closest('.fieldset-content');
    //Reset this target's messages.
    $(this).closest('.fieldset-content').find('.message').each(function() {$(this).remove();});
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
  Drupal.mobileThemeTools.toggle = function () {
    //Check to see fi we are selected or not, if we are, select all. If not deselect all but what was selected.
    var checked = $(this).closest('.fieldset-content').find('.form-checkbox:checked');
    var rows    = $(this).closest('.fieldset-content').find('.form-checkbox');
    if ($(this).is(':checked')) {
      rows.each(function() {
        $(this).attr('checked', true);
      });
    } else {
      rows.each(function() {
        $(this).attr('checked', false);
      });
    }
  }

  /**
   * Theme implenentations.
   */
  //Theme prototype message.
  Drupal.theme.prototype.message = function(target, text, status) {
    //Remove any messages set.
    target.find('.message').each(function() {$(this).remove();});
    //Set our new Message.
    target.append(Drupal.mobileThemeTools.theme.setMessage(Drupal.t(text), status));
    //Set our messages to fade out eventually.
    setTimeout(function() {
      //Set the timeout for our classes.
      $('.status, .warning').fadeOut();
    },'1500');
  }
  //Set a Drupal Message.
  Drupal.mobileThemeTools.theme.setMessage = function(text, status) {
    output = '';
    output += '<div class="message '+status+'"><p><h4>';
    output += text;
    output += '</h4></p></div>';
    return output;
  }
}