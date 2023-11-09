from websocket import create_connection

WS_URL = "ws://localhost:8080/"

ws = create_connection(WS_URL)
print(ws.recv())
print("Sending 'Hello, World'...")
ws.send("Hello, World")
print("Sent")
print("Receiving...")
result =  ws.recv()
print("Received '%s'" % result)
ws.close()
