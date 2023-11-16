import requests
import socket
import os
from websocket import create_connection

# get the directory of the current Python script
script_dir = os.path.dirname(os.path.abspath(__file__)) + "/"

URL = 'http://localhost:8080/'
WS_URL = "ws://localhost:8080/"

def compare_string_to_file(string_to_compare, file_path):
	try:
		with open(file_path, 'r') as file:
			file_content = file.read()
		return file_content == string_to_compare
	except FileNotFoundError:
		print("File not found.")
		return False
	except Exception as e:
		print(f"An error occurred: {str(e)}")
		return False


def sendback(client, msg : bytes) :
	client.send(msg)
	response = client.recv()
	assert(len(response) == len(msg))

def test_websocket():
	client = create_connection(WS_URL)
	try:
		medium_msg = "So, while both approaches can be used to modify a pointer, the choice between them depends on whether you want to change the value of the pointer itself (reference to a pointer) or change what the pointer points to (double pointer).In the reference to a pointer approach, the value of the original pointer is modified, so it now points to a different memory location. In the double pointer approach, you modify the target of the original pointer by indirectly referencing it through the double pointer."
		sendback(client, medium_msg)
		small_msg = "hello world!"
		sendback(client, small_msg)
		file_path = script_dir + 'files/large_file.txt'
		with open(file_path, 'rb') as file:
			large_file_content = file.read()
		sendback(client, large_file_content)

	except Exception as e:
		print(e)
		assert(True == False)
	finally:
		client.close()



# Send a GET request
def test_get():
	response = requests.get(URL)
	assert(response.status_code == 200)
	assert(response.headers['content-type'] == 'text/html')
	response = requests.get(URL + '/random')
	assert(response.status_code == 404)
	assert(compare_string_to_file(response.text, script_dir + "../www/404.html"))

# Send a POST request with data
def test_post():
	data = {'username': 'john', 'password': 'doe'}
	response = requests.post(URL + 'login', data=data)
	assert(response.status_code == 200)
	assert(response.text == "Response to application/x-www-form-urlencoded")
	# large file
	large_file = {'file': open(script_dir + 'files/large_file.txt', 'rb')}
	response = requests.post(URL + 'form/upload', files=large_file)
	assert(response.status_code == 413)

# Send a DELETE request
def test_delete():
	response = requests.delete(URL + 'upload/123')
	assert(response.status_code == 404)
	response = requests.delete(URL + '/cgi/add.py')
	assert(response.status_code == 405)

# Upload file -> GET file -> DELETE file
def test_upload_file():
	small_file = {'file': open(script_dir + 'files/upload.txt', 'rb')}
	response = requests.post(URL + 'form/upload', files=small_file)
	assert(response.status_code == 201)
	response = requests.get(URL + 'upload/upload.txt')
	assert(response.status_code == 200)
	response = requests.delete(URL + 'upload/upload.txt')
	assert(response.status_code == 204)

# Testing redirection in a socket
def test_redir():
	ADDRESS = "0.0.0.0"
	PORT = 8080
	REQUEST = """\
GET /redir HTTP/1.1\r
Host: 0.0.0.0\r
\r
"""
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((ADDRESS, PORT))
	s.send(REQUEST.encode())
	data = s.recv(1024)
	assert data.decode().startswith("HTTP/1.1 301")
	s.close()

def chunker(data, size):
	for i in range(0, len(data), size):
		chunk = data[i:i+size]
		yield chunk.encode()

def test_chunked():
	headers = {
		"Transfer-Encoding": "chunked",
		"Content-Type": "text/plain",
	}
	data = "This is the data to be sent in chunks"
	response = requests.post(URL + 'sendback', headers=headers, data=chunker(data, 5))
	assert response.text == data
	assert response.status_code == 200
	test_get()

# Testing cgi with addition and substraction
def test_cgi():
	num1 = 5
	num2 = 6
	url = "form/add/add.py?num1={0}&num2={1}".format(num1, num2)
	calc = "{0} + {1} = {2}".format(num1, num2, num1 + num2)
	response = requests.get(URL + url)
	assert(response.status_code == 200)
	assert(response.text.startswith("<h1>Addition Results</h1>\n<output>" + calc))

	num3 = 8
	num4 = 9
	url = "form/sub/sub.php?num1={0}&num2={1}".format(num3, num4)
	calc = "{0} - {1} = {2}".format(num3, num4, num3 - num4)
	response = requests.get(URL + url)
	assert(response.status_code == 200)
	assert(response.text.startswith("<h1>Substraction Results</h1><output>" + calc))

