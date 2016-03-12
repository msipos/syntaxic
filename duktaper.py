import re
import sys

init_code = [
  'void init_wrappers(duk_context* ctx) {',
  '  duk_push_global_object(ctx);'
]

def create_wrapper(rv_type, func_name, args):
    print 'static duk_ret_t wrapper_%s(duk_context* ctx) {' % (func_name)

    bound_name = func_name
    if bound_name.startswith('duk_'):
        bound_name = bound_name[4:]

    for i, pair in enumerate(args):
        arg_type = pair[0]
        arg_name = pair[1]
        print '  %s arg%d = ' % (arg_type, i),
        if arg_type == 'int':
            print 'duk_require_int(ctx, %d);' % i
        elif arg_type == 'const char*':
            print 'duk_require_string(ctx, %d);' % i
        elif arg_type == 'bool':
            print 'duk_require_boolean(ctx, %d);' % i
        else:
            raise RuntimeError('Unknown type "%s"' % arg_type)

    # Invoke and push
    if rv_type == 'int':
        print '  duk_push_int(ctx, ',
    elif rv_type == 'void':
        print '  ',
    elif rv_type == 'std::string':
        print '  duk_push_string(ctx, ',
    else:
        raise RuntimeError('Unknown return type "' + rv_type + '"')

    print '%s(' % func_name,
    for i in xrange(len(args)):
        if i == 0:
            print 'arg%d' % i,
        else:
            print ', arg%d' % i,
    print ')',
    if rv_type == 'std::string':
        print '.c_str()',

    if rv_type != 'void':
        print ')',
    print ';'

    # Return
    if rv_type == 'void':
        print '  return 0;'
    else:
        print '  return 1;'
    print '}'
    print

    # Init code
    init_code.append('  duk_push_c_function(ctx, wrapper_%s, %d);' % (func_name, len(args)))
    init_code.append('  duk_put_prop_string(ctx, -2, "%s");' % bound_name);

regex = re.compile('[ (),]+')
for filename in sys.argv[1:]:
    f = open(filename)

    for line in f:
        line = line.strip()
        if line.startswith('// DUKTAPE_NAMESPACE'):
            arr = line.split()
            namespace = arr[2]
            init_code.append('  duk_pop(ctx);')
            init_code.append('  duk_push_global_object(ctx);')
            init_code.append('  duk_push_string(ctx, "%s");' % namespace)
            init_code.append('  duk_push_object(ctx);')
            init_code.append('  duk_put_prop(ctx, -3);')
            init_code.append('  duk_pop(ctx);')
            init_code.append('  duk_get_global_string(ctx, "%s");' % namespace)

        elif line.startswith('DFUNC'):
            arr = regex.split(line)
            rv_type = arr[1]
            func_name = arr[2]

            args = []

            pair = ['', '']

            in_type = True

            first_arg = 3
            if rv_type == 'const':
                rv_type = rv_type + ' ' + func_name
                func_name = arr[3]
                first_arg = 4

            for arg in arr[first_arg:]:
                if in_type:
                    if arg == 'const':
                        pair[0] = 'const '
                    else:
                        pair[0] = pair[0] + arg
                        in_type = False
                else:
                    pair[1] = arg
                    in_type = True
                    args.append(pair)
                    pair = ['', '']
            create_wrapper(rv_type, func_name, args)

    f.close()


print "\n".join(init_code)
print '  duk_pop(ctx);'
print '}'