import os
import cgitb
import datetime

cgitb.enable()

cookie_name = "visit_count"
cookie_value = "1"

if "HTTP_COOKIE" in os.environ:
    cookies = os.environ["HTTP_COOKIE"]
    cookies = cookies.split("; ")
    for cookie in cookies:
        name, value = cookie.split("=")
        if name == cookie_name:
            cookie_value = int(value) + 1

print("Content-type: text/html")
print(f"Set-Cookie: {cookie_name}={cookie_value}; expires={datetime.datetime.now() + datetime.timedelta(days=365)}")

print()
print("<html>")
print("<head>")
print("<title>Visit Counter</title>")
print("</head>")
print("<body>")
print(f"<h1>You have visited this page {cookie_value} times.</h1>")
print("</body>")
print("</html>")
