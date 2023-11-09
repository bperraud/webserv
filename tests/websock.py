from websocket import create_connection
import time

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
		#time.sleep(2)
		#response = client.receive_message()
		#print(response)
		time.sleep(1)
		client.send_message(data_bytes)
		response = client.receive_message()
		print(response)
	except Exception as e:
		print(f"Error: {e}")
	finally:
		client.close()

if __name__ == "__main__":
	main()
