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

  // Add plugin callback.
  //
  // Arguments:
  //   type (string) - Type of an event to register for. Possibilities for now are:
  //     * 'after_save'
  //     * 'before_save'
  //     * 'newline'
  //   callback (function) - Callback to register.  Signature:
  //     function callback(docId) {}
  //
  // Return value: none
  //
  // Availability: 0.9.3
  addPluginCallback: function(type, callback) {},

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
  //   0.9.3: (deprecated) called automatically by the editor before reloading.
  //
  // Arguments: none
  //
  // Return value: none
  //
  // Availability: 0.9.2, deprecated 0.9.3.
  clearPluginMenu: function() {},

  // Enter a character into a document (as if the user did).  Note: use ch=13 (carriage return) for newline.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //   ch (int) - Character to enter (ASCII/unicode)
  //
  // Return value: none
  //
  // Availability: 0.9.2
  docChar: function(docId, ch) {},

  // Copy text (as if user did).  The text is returned as a string (does not go into the clipboard).  If there
  // is no selection, "" is returned.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //
  // Return value: (string) copy of selected text.
  //
  // Availability: 0.9.3
  docCopy: function(docId) {},

  // Cut text (as if user did).  The text is returned as a string (does not go into the clipboard).  If there
  // is no selection, "" is returned.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //
  // Return value: (string) previously selected text.
  //
  // Availability: 0.9.3
  docCut: function(docId) {},

  // Move cursor as if the user clicked this row/col with a mouse.  If row/col are outside of the document,
  // they will be fixed to nearest valid location.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //   row (int) - Row to go to.
  //   col (int) - Col to go to.
  //   ctrl (bool) - True if ctrl held together with the click (jump to location)
  //   shift (bool) - True if shift held together with the click (change selection)
  //
  // Return value: none
  //
  // Availability: 0.9.3
  docMove: function(docId, row, col, ctrl, shift) {},

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

  // Get document cursor location.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //
  // Return value: (object) Cursor location, as object of the form { 'row': integer, 'col': integer }
  //
  // Availability: 0.9.3
  getDocCursorLocation: function(docId) {},

  // Get a line of text from the document/
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //   row (integer) - line to get (1 through numLines).
  //
  // Return value: (string) Text of the line.
  //
  // Availability: 0.9.3
  getDocLine: function(docId, row) {},

  // Get number of lines in the document
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //
  // Return value: (integer) Number of lines in the document.
  //
  // Availability: 0.9.3
  getDocNumLines: function(docId) {},

  // Get document selected text.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //
  // Return value: (string) currently selected text in this doc, or "" if no text is selected.
  //
  // Availability: 0.9.2
  getDocSelectedText: function(docId) {},

  // Get document selection info.  Note start_row/start_col are always before end_row/end_col.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //
  // Return value: (object or null).  Null in case there is no selection.  Object of the form
  // {'start_row': integer, 'start_col': integer, 'end_row': integer, 'end_col': integer,
  //  'lines_start': integer, 'lines_end': integer} otherwise.
  //
  // Availability: 0.9.3
  getDocSelection: function(docId) {},

  // Get document entire text.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //
  // Return value: (string) entire document as a string.
  //
  // Availability: 0.9.2
  getDocText: function(docId) {},

  // Get document type.
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //
  // Return value: (string) document type, the same as the text in Document->Document type, for instance
  // 'Javascript' or 'C/C++'.
  //
  // Availability: 0.9.3
  getDocType: function(docId) {},

  // Go to this location in the file
  //
  // Arguments:
  //   docId (integer) - Document handle.
  //   row (integer) - Row.
  //   col (integer) - Column.
  //
  // Return value: none
  //
  // Availability: 0.9.2, deprecated 0.9.3, use docMove instead.
  goTo: function(docId, row, col) {},

  // Read a file and return as text.  This uses Syntaxic IO abstractions and will work over SSH as well.
  //
  // Arguments:
  //   path (string) - Path
  //
  // Return value: text
  //
  // Availability: 0.9.3
  ioReadFile: function(path) {},

  // Remove a file.  This uses Syntaxic IO abstractions and will work over SSH as well.
  //
  // Arguments:
  //   path (string) - Path
  //
  // Return value: None
  //
  // Availability: 0.9.3
  ioRemoveFile: function(path) {},

  // Get a temp file location.
  //
  // Arguments: none
  //
  // Return value: (string) Path of a temp file that can be written to.
  //
  // Availability: 0.9.3
  ioTempFile: function() {},

  // Write a file (overwrite if exists).  This uses Syntaxic IO abstractions and will work over SSH as well.
  //
  // Arguments:
  //   path (string) - Path
  //   text (string) - Text to write.
  //
  // Return value: text
  //
  // Availability: 0.9.3
  ioWriteFile: function(path, text) {},

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