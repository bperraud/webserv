from websocket import create_connection
import websocket
import os
import logging

def sendback(client, msg : str) :
	client.send(msg, websocket.ABNF.OPCODE_TEXT)
	response = client.recv()
	assert(msg == response)

WS_URL = "ws://localhost:8080/"

script_dir = os.path.dirname(os.path.abspath(__file__)) + "/"

#file_path = 'your_file.txt'
file_path = script_dir + 'files/large_file.txt'

text_data = 'a' * 68980

# Write the string to the file
with open(file_path, 'w') as file:
    file.write(text_data)

def main():

	try:
		websocket.enableTrace(True)
		client = create_connection(WS_URL)
		#small_msg = "hello world!"
		#sendback(client, small_msg)
		#medium_msg = "So, while both approaches can be used to modify a pointer, the choice between them depends on whether you want to change the value of the pointer itself (reference to a pointer) or change what the pointer points to (double pointer).In the reference to a pointer approach, the value of the original pointer is modified, so it now points to a different memory location. In the double pointer approach, you modify the target of the original pointer by indirectly referencing it through the double pointer."
		#sendback(client, medium_msg)

		#with open(file_path, 'r') as file:
		#	large_file_content = file.read()
		#sendback(client, large_file_content)
		ping_data = b'PING_DATA'  # Replace with the actual data you want to send
		client.ping(ping_data)
		client.close()

	except Exception as e:
		# Log the error message to a file
		logging.error(f"An error occurred: {repr(e)}")
		print("error")
		print(repr(e))

if __name__ == "__main__":
	main()
