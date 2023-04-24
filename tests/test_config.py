import requests
import socket

URL = 'http://localhost:8080/'

# Send a GET request
def test_get():
	response = requests.get(URL)
	assert(response.status_code == 200)
	assert(response.headers['content-type'] == 'text/html')
	response = requests.get(URL + '/random')
	assert(response.status_code == 404)

# Send a POST request with data
def test_post():
	data = {'username': 'john', 'password': 'doe'}
	response = requests.post(URL + 'login', data=data)
	assert(response.status_code == 200)
	assert(response.text == "Response to application/x-www-form-urlencoded")
	# large file
	files = {'file': open('tests/files/large_file.txt', 'rb')}
	response = requests.post(URL + 'form/upload', files=files)
	assert(response.status_code == 413)

# Send a DELETE request
def test_delete():
	response = requests.delete(URL + 'upload/123')
	assert(response.status_code == 404)
	response = requests.delete(URL + '/cgi/add.py')
	assert(response.status_code == 405)

# Upload file -> GET file -> DELETE file
def test_upload_file():
	files = {'file': open('tests/files/upload.txt', 'rb')}
	response = requests.post(URL + 'form/upload', files=files)
	assert(response.status_code == 201)
	response = requests.get(URL + 'upload/upload.txt')
	assert(response.status_code == 200)
	response = requests.delete(URL + 'upload/upload.txt')
	assert(response.status_code == 204)

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

def cgi():
    response = requests.get(URL + 'form/add/add.py')
    assert(response.status_code == 301)
