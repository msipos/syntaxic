//
// All plugin API is available through the Syn object.
//
// Note, all document references are held on Javascript side through _handles_, which are Javascript integers.  This reduces the chance of the program crashing through the Javascript references outliving the documents on the C++ side.
//
var Syn = {
  // Add overlay (floating message) to a document.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //   row (integer) - Row.
  //   col (integer) - Column.
  //   type (integer) - Type (must be 1 for now).
  //   text (string) - Text of the overlay.
  //
  // Return value: none
  //
  // Availability: 0.9.2
  addOverlay: function(docId, row, col, type, text) {},

  // Add menu item under plugins/.
  //
  // Arguments:
  //   text (string) - Menu item title
  //   shortcut (string) - Menu shortcut, string such as "Ctrl+J", empty for no shortcut
  //   action (function) - Javascript function to execute
  //
  // Return value: none
  //
  // Availability: 0.9.2
  addPluginMenuItem: function(text, shortcut, action) {},

  // Clear all overlays.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //
  // Return value: none
  //
  // Availability: 0.9.2
  clearOverlays: function(docId) {},

  // Clear plugins/ menu. You probably want to call this at the top of your syntaxic.js
  //
  // Arguments: none
  //
  // Return value: none
  //
  // Availability: 0.9.2
  clearPluginMenu: function() {},

  // Enter a character into a document (as if the user did).  Note: use ch=13 (carriage return) for newline.
  //
  // Arguments:
  //   ch (int) - Character to enter (ASCII/unicode)
  //
  // Return value: none
  //
  // Availability: 0.9.2
  docChar: function(docId, ch) {},

  // Paste text into a document (as if the user did).
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //   text (string) - Text to paste
  //
  // Return value: none
  //
  // Availability: 0.9.2
  docPaste: function(docId, text) {},

  // Fade in a message in the corner of the editor window.
  //
  // Arguments:
  //   title (string) - Message title
  //   text (string) - Message text
  //
  // Return value: none
  //
  // Availability: 0.9.1
  feedback: function(title, text) {},

  // Get current document handle.
  //
  // Arguments: none
  //
  // Return value: (integer) currently selected doc handle, or -1 if no document is selected.
  //
  // Availability: 0.9.2
  getCurrentDoc: function() {},

  // Get a shell handle, if any is available.
  //
  // Arguments: none
  //
  // Return value: (integer) first shell handle, or -1 if no shells are open
  //
  // Availability: 0.9.2
  getShellDoc: function() {},

  // Get document selected text.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //
  // Return value: (string) currently selected text in this doc, or "" if no text is selected.
  //
  // Availability: 0.9.2
  getDocSelectedText: function(docId) {},

  // Get document entire text.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //
  // Return value: (string) entire document as a string.
  //
  // Availability: 0.9.2
  getDocText: function(docId) {},

  // Go to this location in the file
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //   row (integer) - Row.
  //   col (integer) - Column.
  //
  // Return value: none
  //
  // Availability: 0.9.2
  goTo: function(docId, row, col) {},

  // Prompt the user to enter text.
  //
  // Arguments:
  //   title (string) - Dialog box title
  //   text (string) - Dialog box text
  //   defaultText (string) - Default text already entered.
  //
  // Return value: none
  //
  // Availability: 0.9.2
  promptText: function(title, text, defaultText) {},

  // Synchronously execute an external command.
  //
  // Arguments:
  //   obj (object) - Object with following keys:
  //     command (string) - Required.  Command to execute.
  //     stdin (string) - Optional.  If supplied, stdin will be piped into the external process.
  //
  // Return value: object with keys:
  //     exitCode (integer)
  //     stdout (string)
  //     stderr (string)
  //
  // Availability: 0.9.2
  system: function(docId, row, col) {},
};