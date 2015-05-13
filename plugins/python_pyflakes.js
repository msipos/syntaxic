// Run pyflakes to mark errors in Python code.  You may have to change PYFLAKES_BIN depending on your OS.
//
// INSTALL INSTRUCTIONS
//
//  1) Click Plugins->Manage plugins...
//  2) Copy following text to the bottom of the file.
//  3) Press F10 to reload.
//
// NOTE: You can customize keyboard shortcut by changing the text below.

var PYFLAKES_BIN = "pyflakes";

Syn.addPluginMenuItem("Pyflakes", "F1", function() {
  var handle = Syn.getCurrentDoc();
  if (handle < 0) return;
  Syn.clearOverlays(handle);

  var result = Syn.system({
    command: PYFLAKES_BIN,
    stdin: Syn.getDocText(handle)
  });

  // Process output of pyflakes
  var output = result.stdout + result.stderr;
  var lines = output.split('\n');

  var num_errors = 0;
  for (var i = 0; i < lines.length; i++) {
    var line = lines[i];
    var arr = line.split(':');
    if (arr.length >= 3) {
      if (arr[0] !== '<stdin>') continue;
      var row = parseInt(arr[1]);
      var col = 0;
      var text = "";
      if (arr.length > 3) {
        col = parseInt(arr[2]);
        text = arr.slice(3).join(':');
      } else {
        text = arr.slice(2).join(':');
      }
      Syn.addOverlay(handle, row, col, 1, text);
      num_errors += 1;
    }
  }
  Syn.feedback('Pyflakes', num_errors + ' errors found. Press ESC to clear.');
});
