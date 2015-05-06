//
// All plugin API is available through the Syn object.
//
// Note, all document references are held on Javascript side through _handles_, which are Javascript integers.  This reduces the chance of the program crashing through the Javascript references outliving the documents on the C++ side.
//
var Syn = {
  // Add menu item under plugins/.
  //
  // Arguments:
  //   text (string) - Menu item title
  //   shortcut (string) - Menu shortcut, string such as "Ctrl+J"
  //   action (function) - Javascript function to execute
  //
  // Return value: none
  //
  // Availability: 0.9.2
  addPluginMenuItem: function(text, shortcut, action) {},

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

  // Replace document selected text.  If no selection in doc, no action is performed.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //   newText (string) - New text.
  //
  // Return value: none
  //
  // Availability: 0.9.2
  replaceDocSelectedText: function(docId, newText) {},

  // Write text into a document, as if a user did.  (This is useful to write a command into the shell.)
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //   text (string) - Text to write
  //
  // Return value: none
  //
  // Availability: 0.9.2
  writeIntoDoc: function(docId, text) {},
};