// Example of how to run a command in your shell.  Replace the command ('make') with whatever you'd like.
//
// INSTALL INSTRUCTIONS
//
//  1) Click Plugins->Manage plugins...
//  2) Copy following text to the bottom of the file.
//  3) Press F10 to reload.
//
// NOTE: You can customize keyboard shortcut by changing the text below.

Syn.addPluginMenuItem("Make", "F5", function() {
  var handle = Syn.getShellDoc();
  if (handle < 0) {
    Syn.feedback("No shell", "No shell");
    return;
  }
  Syn.docPaste(handle, "make");
  Syn.docChar(handle, 13); // Press enter in the shell
});
