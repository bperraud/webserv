import requests

URL = 'http://localhost:8080/'

# Send a GET request
def rtest_get():
	response = requests.get(URL)
	assert(response.status_code == 200)
	assert(response.headers['content-type'] == 'text/html')
	response = requests.get(URL + '/random')
	assert(response.status_code == 404)


# Send a POST request with data
def rtest_post():
	data = {'username': 'john', 'password': 'doe'}
	response = requests.post(URL + '/login', data=data)
	assert(response.status_code == 404)
	# large file
	files = {'file': open('files/large_file.txt', 'rb')}
	response = requests.post(URL + 'form/upload', files=files)
	assert(response.status_code == 413)


# Send a DELETE request
def rtest_delete():
	response = requests.delete(URL + '/user/123')
	assert(response.status_code == 404)
	response = requests.delete(URL + '/cgi/add.py')
	assert(response.status_code == 405)

# Upload file -> GET file -> DELETE file
def rtest_upload_file():
	files = {'file': open('files/upload.txt', 'rb')}
	response = requests.post(URL + 'form/upload', files=files)
	assert(response.status_code == 201)
	response = requests.get(URL + 'upload/upload.txt')
	assert(response.status_code == 200)
	response = requests.delete(URL + 'upload/upload.txt')
	assert(response.status_code == 204)


def chunker(data, size):
	for i in range(0, len(data), size):
		chunk = data[i:i+size]
		yield chunk.encode()


def rtest_chunked_message():

	headers = {
		"Transfer-Encoding": "chunked",
		"Content-Type": "text/plain",
	}

	data = '5\r\nhello\r\n6\r\nworld!\r\n0\r\n\r\n'
	response = requests.post(URL, headers=headers, data=data)
	assert response.status_code == 200
	assert response.text == 'OK'


def test_chunked():

	headers = {
		"Transfer-Encoding": "chunked",
		"Content-Type": "text/plain",
	}
	data = "This is the data to be sent in chunks"
	response = requests.post(URL, headers=headers, data=chunker(data, 5))
	assert response.text == data
