var fs = require('fs')
var util = require('util')

/////////////////////////////////////////////////// This is purely based on HLJS code
// NOTE: Remove PHRASAL_WORDS_MODE. It is not relevant.

var hljs = {}
hljs.inherit = function(parent, obj) {
  var result = {};
  for (var key in parent)
    result[key] = parent[key];
  if (obj)
    for (var key in obj)
      result[key] = obj[key];
  return result;
};

// Common regexps
hljs.IDENT_RE = '[a-zA-Z]\\w*';
hljs.UNDERSCORE_IDENT_RE = '[a-zA-Z_]\\w*';
hljs.NUMBER_RE = '\\b\\d+(\\.\\d+)?';
hljs.C_NUMBER_RE = '\\b(0[xX][a-fA-F0-9]+|(\\d+(\\.\\d*)?|\\.\\d+)([eE][-+]?\\d+)?)'; // 0x..., 0..., decimal, float
hljs.BINARY_NUMBER_RE = '\\b(0b[01]+)'; // 0b...
hljs.RE_STARTERS_RE = '!|!=|!==|%|%=|&|&&|&=|\\*|\\*=|\\+|\\+=|,|-|-=|/=|/|:|;|<<|<<=|<=|<|===|==|=|>>>=|>>=|>=|>>>|>>|>|\\?|\\[|\\{|\\(|\\^|\\^=|\\||\\|=|\\|\\||~';

// Common modes
hljs.BACKSLASH_ESCAPE = {
  begin: '\\\\[\\s\\S]', relevance: 0
};
hljs.APOS_STRING_MODE = {
  className: 'string',
  begin: '\'', end: '\'',
  illegal: '\\n',
  contains: [hljs.BACKSLASH_ESCAPE]
};
hljs.QUOTE_STRING_MODE = {
  className: 'string',
  begin: '"', end: '"',
  illegal: '\\n',
  contains: [hljs.BACKSLASH_ESCAPE]
};
hljs.PHRASAL_WORDS_MODE = {
  begin: ''
};
hljs.C_LINE_COMMENT_MODE = {
  className: 'comment',
  begin: '//', end: '$',
};
hljs.C_BLOCK_COMMENT_MODE = {
  className: 'comment',
  begin: '/\\*', end: '\\*/',
};
hljs.HASH_COMMENT_MODE = {
  className: 'comment',
  begin: '#', end: '$',
};
hljs.NUMBER_MODE = {
  className: 'number',
  begin: hljs.NUMBER_RE,
  relevance: 0
};
hljs.C_NUMBER_MODE = {
  className: 'number',
  begin: hljs.C_NUMBER_RE,
  relevance: 0
};
hljs.BINARY_NUMBER_MODE = {
  className: 'number',
  begin: hljs.BINARY_NUMBER_RE,
  relevance: 0
};
hljs.CSS_NUMBER_MODE = {
  className: 'number',
  begin: hljs.NUMBER_RE + '(' +
    '%|em|ex|ch|rem'  +
    '|vw|vh|vmin|vmax' +
    '|cm|mm|in|pt|pc|px' +
    '|deg|grad|rad|turn' +
    '|s|ms' +
    '|Hz|kHz' +
    '|dpi|dpcm|dppx' +
    ')?',
  relevance: 0
};
hljs.REGEXP_MODE = {
  className: 'regexp',
  begin: /\//, end: /\/[gimuy]*/,
  illegal: /\n/,
  contains: [
    hljs.BACKSLASH_ESCAPE,
    {
      begin: /\[/, end: /\]/,
      relevance: 0,
      contains: [hljs.BACKSLASH_ESCAPE]
    }
  ]
};
hljs.TITLE_MODE = {
  className: 'title',
  begin: hljs.IDENT_RE,
  relevance: 0
};
hljs.UNDERSCORE_TITLE_MODE = {
  className: 'title',
  begin: hljs.UNDERSCORE_IDENT_RE,
  relevance: 0
};

///////////////////////////////////////////////////////////////////////////////
// From here on we hack on HLJS to make it work for us (JSON definitions)

var VALID_CLASSES = [
  "string", "preprocessor", "comment", "number", "operator", "regexp", "keyword"
];

var CLASS_MAP = {
  "variable": "ident"
};

var validObj = function(obj, depth) {
  if (obj == null) return false;
  if (!obj.hasOwnProperty("className") && obj.hasOwnProperty("begin") && obj.begin !== "" && depth == 2) {
    return true;
  }
  if (depth > 2) return false;
  if (!obj.hasOwnProperty("className")) return false;
  className = obj.className;
  if (VALID_CLASSES.indexOf(className) == -1){
    console.log("dropping " + className);
    return false;
  }
  return true;
}

var process = function(obj, depth) {
  if (obj == null) return;
  if (depth == 2) {
    delete obj.contains;
  }
  if (depth > 0) {
    delete obj.keywords;
  }
  delete obj.beginKeywords;
  delete obj.endsWithParent;
  delete obj.aliases;
  delete obj.relevance;
  delete obj.returnBegin;

  // Expand variants
  if (obj.hasOwnProperty("contains")) {
    var array = obj.contains;
    for (var i = array.length - 1; i >= 0; i--) {
      var child = array[i];
      if (child.hasOwnProperty("variants")) {

        // Go through variants
        var array2 = child.variants;
        for (var j = 0; j < array2.length; j++) {
          // Inherit
          var variant = array2[j];
          var new_obj = {};
          for (var field in child) {
            if (field != "variants") new_obj[field] = child[field];
          }
          for (var field in variant) {
            new_obj[field] = variant[field];
          }
          array.push(new_obj);
        }
        array.splice(i, 1);
      }
    }
  }

  // Deal with keywords
  if (obj.hasOwnProperty("keywords")) {
    var keywords = obj.keywords;
    if (typeof(keywords) === "string") {
      keywords = {keywords: keywords};
    }
    obj.meta = keywords;
    delete obj.keywords;
  }

  // Go through contains
  if (obj.hasOwnProperty("contains")) {
    var array = obj.contains;
    for (var i = array.length - 1; i >= 0; i--) {
      var child = array[i];
      process(child, depth+1);
      if (!validObj(child, depth+1)) {
        array.splice(i, 1);
      }
    }
    //if (array.length == 0) {
    //  delete obj.contains;
    //}
  }

  // Lexemes (identifier):
  if (depth == 0) {
    var lexemes = hljs.UNDERSCORE_IDENT_RE;
    if (obj.hasOwnProperty("lexemes")) {
      lexemes = obj.lexemes;
    }
    obj.contains.unshift({"className": "ident", "begin": lexemes})
  }
  delete obj.lexemes;

  // Deal with root.
  if (depth == 0) {
    var root = {};
    var to_delete = [];
    for (var field in obj) {
      if (field !== "meta") {
        root[field] = obj[field];
        to_delete.push(field);
      }
    }
    for (var i = 0; i < to_delete.length; i++) {
      delete obj[to_delete[i]];
    }
    obj.root = root;
  }

  return obj;
};

var return_func = function(filename) {
  var text = fs.readFileSync(filename)+'';
  var x = null;
  return eval('x = ' + text);
};

extras = ['bash', 'clojure', 'coffeescript', 'd', 'dart', 'delphi', 'django', 'dockerfile',
    'elixir', 'erlang', 'fsharp', 'glsl', 'groovy', 'haskell', 'haxe', 'julia', 'less', 'lisp',
    'lua', 'makefile', 'markdown', 'mathematica', 'matlab', 'nginx', 'nimrod', 'ocaml',
    'powershell', 'processing', 'protobuf', 'puppet', 'r', 'rust', 'scala', 'scheme',
    'smalltalk', 'tcl', 'tex', 'vbnet', 'verilog', 'vhdl', 'x86asm', 'xml']
languages = ['cmake', 'cpp', 'cs', 'css', 'go', 'java', 'javascript', 'json', 'objectivec',
    'perl', 'php', 'python', 'ruby', 'sql', 'swift']

metas = {}

for (var i = 0; i < extras.length; i++) {
  var lang = extras[i];
  try {
    var name = lang;
    console.log(' * Doing language ' + name)
    var func = return_func('input/' + name + '.js');
    var data = func(hljs);
    var extensions = ["." + name];
    var aliases = data.aliases;
    if (aliases instanceof Array) {
      for (var j = 0; j < aliases.length; j++) {
        extensions.push("." + aliases[j]);
      }
    }

    var langName = name.charAt(0).toUpperCase() + name.slice(1);

    metas[langName] = {
      "meta_file": name,
      "extensions": extensions
    }

    data = process(data, 0);

    // Handle regex objects into JSON
    var replacer = function(key, value) {
      if (value instanceof RegExp) {
          var str = value.toString();
          return str.substring(1, str.length - 1);
      }
      return value;
    }
    var json = JSON.stringify(data, replacer, '  ');
    fs.writeFileSync('output/' + name + '.json', json);
  } catch(e) {
    console.log("ERROR: For language " + lang + ": " + e.stack);
    //console.log("ERROR: data was: " + util.inspect(data, {depth:4}));
  }
}

console.log(JSON.stringify(metas, null, '  '))