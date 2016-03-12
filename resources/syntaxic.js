// This file gets loaded at program start-up and when plugins are reloaded.
//
// If you add/change/remove plugins, make sure to reload (F10).
//
// Add new plugins to the bottom of this file.
//
// For the available API, see: https://github.com/msipos/syntaxic/blob/master/api.js

Syn.addPluginMenuItem("To &Lowercase", "Ctrl+Shift+U", function() {
  var handle = Syn.getCurrentDoc();
  if (handle >= 0) {
    // Get selected text, lowercase it and paste it back in.
    var text = Syn.getDocSelectedText(handle);
    Syn.docPaste(handle, text.toLowerCase());
  }
});

Syn.addPluginMenuItem("To &Uppercase", "Ctrl+U", function() {
  var handle = Syn.getCurrentDoc();
  if (handle >= 0) {
    // Get selected text, uppercase it and paste it back in.
    var text = Syn.getDocSelectedText(handle);
    Syn.docPaste(handle, text.toUpperCase());
  }
});

Syn.addPluginMenuItem("&Go to line", "Ctrl+.", function() {
  var handle = Syn.getCurrentDoc();
  if (handle < 0) return;
  var line = parseInt(Syn.promptText("Go to line", "Enter line number:", ""));
  Syn.goTo(handle, line, 0);
});
