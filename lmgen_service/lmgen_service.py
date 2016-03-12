from bottle import route, request, run, template
from subprocess import check_output
import uuid
import logging
from urllib import urlencode

token_map = {}

FAIL_URL = "https://syntaxiceditor.com/gumroad/fail"

@route('/check')
def check():
    return '<html>alive</html>'

@route('/license_key')
def license_key():
    try:
        params = request.params.decode()

        if 'token' not in params:
            return 'Invalid token, please contact support@kpartite.com'

        token = params["token"]
        logging.info('/license_key, token = {0}'.format(token))

        if len(token) > 75:
            return 'Invalid token, please contact support@kpartite.com'

        if token not in token_map:
            return 'Invalid token, please contact support@kpartite.com'

        email, key = token_map[token]

        return template("""
          <html><body><h1>Thank you for purchasing Syntaxic!</h1>

          <p>Your license information is the following:</p>

          <p>Email: <b>{{email}}</b></p>
          <p>License key: <b>{{key}}</b></p>

          <p>Please enter these into your program and keep them for your records.</p>

          <p>If you have any problems unlocking Syntaxic, please contact <a href="mailto:support@kpartite.com">support@kpartite.com</a>.</p>

          </body></html>

        """, email=email, key=key)
    except:
        logging.exception("license_key")
        return """
          <html><body><h1>Error occured during processing of your purchase.</h1>

          <h2>Please contact us at <a href="mailto:support@kpartite.com">support@kpartite.com</a>.</h2>
          </body></html>
        """

@route('/purchase')
def purchase():
    try:
        params = request.params
        if 'email' not in params:
            return FAIL_URL

        email = params["email"]
        logging.info('/purchase, email = {0}'.format(email))
        if len(email) > 75:
            return FAIL_URL

        key = check_output(['./lmgen', email])
        logging.info('/purchase, key = {0}'.format(key))
        token = str(uuid.uuid4())
        logging.info('/purchase, token = {0}'.format(token))
        token_map[token] = (email, key)

        return "https://syntaxiceditor.com/gumroad/license_key?{0}".format(
            urlencode({'token': token}))
    except:
        logging.exception("purchase")
        return FAIL_URL

logging.basicConfig(filename='log.txt', format=logging.BASIC_FORMAT, level=logging.INFO)
run(host='localhost', port=8666)
