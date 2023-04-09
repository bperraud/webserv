import requests

# Send a GET request
response = requests.get('http://localhost:8080/')
print(response.status_code)
print(response.text)

# Send a POST request with data
data = {'username': 'john', 'password': 'doe'}
response = requests.post('http://localhost:8080/login', data=data)
print(response.status_code)
print(response.text)

# Send a DELETE request
response = requests.delete('http://localhost:8080/user/123')
print(response.status_code)
print(response.text)
