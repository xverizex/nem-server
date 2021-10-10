#!/bin/bash
openssl req -x509 -sha256 -nodes -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365
