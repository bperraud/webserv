from websocket import create_connection

class WebSocketClient:
	def __init__(self, url):
		self.url = url
		self.ws = None

	def connect(self):
		self.ws = create_connection(self.url)
		print("Connected")

	def send_message(self, message):
		self.ws.send(message)
		print(f"Sent: {message}")

	def receive_message(self):
		message = self.ws.recv()
		print(len(message))
		print(f"Received: {message}")
		return message

	def close(self):
		if self.ws:
			self.ws.close()
			print("Connection closed")

def main():
	WS_URL = "ws://localhost:8080/"

	client = WebSocketClient(WS_URL)
	data_bytes = bytes([0x01] * 287)

	try:
		client.connect()
		client.send_message("So, while both approaches can be used to modify a pointer, the choice between them depends on whether you want to change the value of the pointer itself (reference to a pointer) or change what the pointer points to (double pointer).In the reference to a pointer approach, the value of the original pointer is modified, so it now points to a different memory location. In the double pointer approach, you modify the target of the original pointer by indirectly referencing it through the double pointer.")
		response = client.receive_message()
		print(response)
	except Exception as e:
		print(f"Error: {e}")
	finally:
		client.close()

if __name__ == "__main__":
	main()
