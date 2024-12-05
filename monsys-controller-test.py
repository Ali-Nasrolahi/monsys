#!/bin/python3

import requests
# Tests controller handler
print(requests.post('https://192.168.8.247/api/v1/', data='0x55{somekey: somevalue}', verify=False).text)
