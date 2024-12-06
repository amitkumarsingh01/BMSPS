from flask import Flask, render_template, request, redirect, url_for, jsonify
import requests
import json
from datetime import datetime

app = Flask(__name__)

# Relay status (simulated, replace with actual GPIO control later)
relay_status = False

# Dummy data for floors (will be dynamically generated)
floors = ['Ground', 'First', 'Second']

# Sensor data fetching function (from ThingSpeak API)
def fetch_sensor_data():
    numResultsLoaded = 20
    url = f'https://api.thingspeak.com/channels/2776658/feeds.json?results={numResultsLoaded}'
    response = requests.get(url)
    data = response.json()
    feeds = data['feeds']
    
    # Parse sensor data into individual lists
    temp = [float(feed['field1']) for feed in feeds]
    humidity = [float(feed['field2']) for feed in feeds]
    gas = [float(feed['field3']) for feed in feeds]
    current = [float(feed['field4']) for feed in feeds]
    voltage = [float(feed['field5']) for feed in feeds]
    power = [v * i for v, i in zip(voltage, current)]
    
    # Convert ISO time format to a more readable format
    latest_time = datetime.strptime(feeds[-1]['created_at'], '%Y-%m-%dT%H:%M:%SZ').strftime('%Y-%m-%d %H:%M:%S')
    latest_data = {
        'temp': temp[-1],
        'humidity': humidity[-1],
        'gas': gas[-1],
        'current': current[-1],
        'voltage': voltage[-1],
        'power': power[-1],
        'time': latest_time
    }
    
    return {
        'temp': temp,
        'humidity': humidity,
        'gas': gas,
        'current': current,
        'voltage': voltage,
        'power': power,
        'latest': latest_data
    }

@app.route('/')
def index():
    return render_template('index.html', floors=floors)

@app.route('/add_floor', methods=['POST'])
def add_floor():
    floor_name = request.form['floor_name']
    floors.append(floor_name)
    return redirect(url_for('index'))

@app.route('/floor/<floor_name>')
def floor_page(floor_name):
    sensor_data = fetch_sensor_data()
    return render_template('floor.html', floor_name=floor_name, sensor_data=sensor_data, relay_status=relay_status)

@app.route('/toggle_relay', methods=['POST'])
def toggle_relay():
    global relay_status
    relay_status = not relay_status
    return jsonify({'relay_status': relay_status})

if __name__ == '__main__':
    app.run(debug=True)
