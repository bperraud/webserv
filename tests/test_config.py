import requests

# Send a GET request
def test_get():
	response = requests.get('http://localhost:8080/')
	assert(response.status_code == 200)
	assert(response.headers['content-type'] == 'text/html')
	response = requests.get('http://localhost:8080/random')
	assert(response.status_code == 404)


# Send a POST request with data
def test_post():
	data = {'username': 'john', 'password': 'doe'}
	response = requests.post('http://localhost:8080/login', data=data)
	assert(response.status_code == 404)
	# large file
	files = {'file': open('files/large_file.txt', 'rb')}
	response = requests.post('http://localhost:8080/form/upload', files=files)
	assert(response.status_code == 413)


# Send a DELETE request
def test_delete():
	response = requests.delete('http://localhost:8080/user/123')
	assert(response.status_code == 404)
	response = requests.delete('http://localhost:8080/cgi/add.py')
	assert(response.status_code == 405)

# Upload file -> GET file -> DELETE file
def test_upload_file():
	files = {'file': open('files/upload.txt', 'rb')}
	response = requests.post('http://localhost:8080/form/upload', files=files)
	assert(response.status_code == 201)
	response = requests.get('http://localhost:8080/upload/upload.txt')
	assert(response.status_code == 200)
	response = requests.delete('http://localhost:8080/upload/upload.txt')
	assert(response.status_code == 204)

