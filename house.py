#!/bin/python
from bottle import route, run, request, template, static_file
import os

@route('/sensors/<sensor>', method='GET')
def recipe_show(sensor=""):
    cmd = request.query.get("cmd")
    os.system('mosquitto_pub -t home/sensors/' + sensor + ' -m "' + cmd + '"')
    return "OK"

@route('/')
def index():
    output = template('house.html')
    return output

@route('/house.jpg')
def index():
    return static_file('house.jpg', '.')

run(host='0.0.0.0', port=8000, debug=True)
