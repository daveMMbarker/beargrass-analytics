import os
import time
import sqlite3
import datetime
import json
import requests
from Adafruit_IO import MQTTClient, Client, RequestError

ADAFRUIT_IO_USERNAME = os.getenv("ADAFRUIT_IO_USERNAME")
ADAFRUIT_IO_KEY = os.getenv("ADAFRUIT_IO_KEY")
ADAFRUIT_IO_FEED_ID = "beargrass"
DATABASE_NAME = "beargrass-analytics.db"

API_KEY = os.getenv("AMBIENT_KEY")
APP_KEY = os.getenv("AMBIENT_APP_KEY")
MAC_ADDRESS = os.getenv("AMBIENT_MAC_ADDRESS")

# Ambient Weather API endpoint for fetching device data
API_URL = "https://api.ambientweather.net/v1/devices"


def insert_message(feed_id, payload_str):
    """
    This function now creates a new, thread-local database connection for each call.
    """
    db_connection = None
    try:
        message_data = json.loads(payload_str)
        # Extract data from the JSON object
        message_id = message_data.get("id")
        timestamp_str = message_data.get("timestamp")
        mA = message_data.get("mA")
        RSSI = message_data.get("RSSI")
        SNR = message_data.get("SNR")

        # Open a new connection for this thread
        db_connection = sqlite3.connect(DATABASE_NAME)
        cursor = db_connection.cursor()
        cursor.execute("""
            INSERT INTO mqtt_messages (feed_id, message_id, timestamp, mA, RSSI, SNR)
            VALUES (?, ?, ?, ?, ?, ?)
        """, (feed_id, message_id, timestamp_str, mA, RSSI, SNR))
        db_connection.commit()
        print(f"Message from '{feed_id}' with ID '{message_id}' saved to database.")
    except json.JSONDecodeError as e:
        print(f"Error decoding JSON payload: {e}")
    except sqlite3.Error as e:
        print(f"Error inserting message into database: {e}")
    finally:
        # Always close the connection
        if db_connection:
            db_connection.close()

def insert_weather_data(payload_str,latest_weather_data):
    """
    This function now creates a new, thread-local database connection for each call.
    """
    if not latest_weather_data:
        print("No weather data to display.")
        return

    try:
        message_data = json.loads(payload_str)
        # Extract message_id from the mqtt payload
        message_id = message_data.get("id")
        pressure = latest_weather_data.get('baromrelin')
        temperature = latest_weather_data.get('tempf')
        hourlyrain = latest_weather_data.get('hourlyrainin')
        eventrain = latest_weather_data.get('eventrainin')
        dailyrain = latest_weather_data.get('dailyrainin')
        totalrain = latest_weather_data.get('totalrainin')
        monthlyrain = latest_weather_data.get('monthlyrainin')
        weeklyrain = latest_weather_data.get('weeklyrainin')
        humidity = latest_weather_data.get('humidity')
        maxdailygust = latest_weather_data.get('maxdailygust')
        date = latest_weather_data.get('date')
        datelastrain = latest_weather_data.get('lastRain')

        # Open a new connection for this thread
        db_connection = sqlite3.connect(DATABASE_NAME)
        cursor = db_connection.cursor()
        cursor.execute("""
            INSERT INTO weather_data (message_id,pressure,temperature,hourlyrain,eventrain,dailyrain,totalrain,monthlyrain,weeklyrain,humidity,maxdailygust,date,datelastrain)
            VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)
        """, (message_id,pressure,temperature,hourlyrain,eventrain,dailyrain,totalrain,monthlyrain,weeklyrain,humidity,maxdailygust,date,datelastrain))
        db_connection.commit()
        print(f"Message with id '{message_id}' saved to database.")
    except json.JSONDecodeError as e:
        print(f"Error decoding JSON payload: {e}")
    except sqlite3.Error as e:
        print(f"Error inserting message into database: {e}")
    finally:
        # Always close the connection
        if db_connection:
            db_connection.close()

# --- Callback Functions ---

def connected(client):
    print("Connected to Adafruit IO! Listening for changes...")
    print(f"Subscribing to feed: {ADAFRUIT_IO_FEED_ID}")
    client.subscribe(ADAFRUIT_IO_FEED_ID)

def disconnected(client):
    print("Disconnected from Adafruit IO.")

def message(client, feed_id, payload):
    # The payload from the adafruit-io library is already a string
    print(f"Feed '{feed_id}' received new value: {payload}")
    insert_message(feed_id, payload)
    latest_weather_data = get_weather_data();
    insert_weather_data(payload,latest_weather_data);


def subscribe(client, userdata, mid, granted_qos):
    print(f"Subscribed to feed with QoS: {granted_qos}")

def main():
    if not ADAFRUIT_IO_USERNAME or not ADAFRUIT_IO_KEY:
        print("Error: ADAFRUIT_IO_USERNAME and ADAFRUIT_IO_KEY environment variables must be set.")
        print("Please set them before running the script.")
        print("Example: export ADAFRUIT_IO_USERNAME='your_username'")
        print("         export ADAFRUIT_IO_KEY='your_key'")
        return

    try:
        # Create an MQTT client instance for Adafruit IO
        client = MQTTClient(ADAFRUIT_IO_USERNAME, ADAFRUIT_IO_KEY)

        # Assign callback functions
        client.on_connect = connected
        client.on_disconnect = disconnected
        client.on_message = message
        client.on_subscribe = subscribe

        # Connect to Adafruit IO
        print("Attempting to connect to Adafruit IO MQTT broker...")
        client.connect()

        # Start the MQTT loop in a non-blocking way.
        client.loop_background()

        print("Press Ctrl+C to exit.")
        while True:
            time.sleep(1)

    except RequestError as e:
        print(f"Adafruit IO Request Error: {e}")
        print("Please check your ADAFRUIT_IO_USERNAME and ADAFRUIT_IO_KEY.")
    except KeyboardInterrupt:
        print("\nExiting program.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
    finally:
        # The loop_background() will handle disconnection.
        print("Program finished.")


def get_weather_data():
    if not API_KEY or not APP_KEY or not MAC_ADDRESS:
        print("Error: Please set the AMBIENT_WEATHER_API_KEY, AMBIENT_WEATHER_APP_KEY, and AMBIENT_WEATHER_MAC_ADDRESS environment variables.")
        return None

    # Construct the full API URL with the MAC address and keys
    full_url = f"{API_URL}/{MAC_ADDRESS}"
    params = {
        "applicationKey": APP_KEY,
        "apiKey": API_KEY,
        "limit":1
    }

    try:
        print(f"Fetching data for device {MAC_ADDRESS}...")
        # Make the GET request to the Ambient Weather API
        response = requests.get(full_url, params=params)

        # Raise an exception for bad status codes (4xx or 5xx)
        response.raise_for_status()

        weather_data = response.json()
       # The API returns a list of data points, usually with the latest at the beginning.
        if weather_data and isinstance(weather_data, list):
            latest_data = weather_data[0]
            print(f"LATEST WEATHER:{latest_data}")
            return latest_data
        else:
            print("No data found or unexpected response format.")
            return None

    except requests.exceptions.HTTPError as err:
        print(f"HTTP Error: {err}")
        print("Please check your API keys and MAC address.")
    except requests.exceptions.RequestException as err:
        print(f"An error occurred: {err}")
    except json.JSONDecodeError:
        print("Failed to decode JSON from the response.")

    return None

if __name__ == "__main__":
    main()
