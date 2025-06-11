#!/usr/bin/python3
# -*- coding: utf-8 -*-
import requests
import sys

def print_request(request):
    method = request.method
    path_url = request.path_url
    headers = (''.join('{0}: {1}\r\n'.format(k, v) for k, v in request.headers.items()))
    body = (request.body) or ""
    req = ''.join(
        [
            method,
            ' ',
            path_url,
            ' HTTP/1.1\r\n',
            headers,
            '\r\n',
            body
        ]
        )
    req_size = str(len(req))
    return ''.join([req_size,'\n',req,'\r\n'])

#POST multipart form data
def post_multipart(host, port, namespace, headers, body) :
    req = requests.Request(
        'POST',
        'https://{host}:{port}{namespace}'.format(
            host = host,
            port = port,
            namespace = namespace,
        ),
        headers = headers,
        data = body
    )
    prepared = req.prepare()
    return print_request(prepared)

if __name__ == "__main__":
    #usage sample below
    #target's hostname and port
    #this will be resolved to IP for TCP connection
    host = 'cppserver'
    port = '8080'
    namespace = '/api/v1/game/join'
    #below you should specify or able to operate with
    #virtual server name on your target
    headers = {
        'Content-Type': 'application/json'
    }
    body = "{\"userName\":\"Scooby Doo\", \"mapId\":\"town\"}"

    sys.stdout.buffer.write(post_multipart(host, port, namespace, headers, body).encode())
