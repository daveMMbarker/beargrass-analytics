import os
import requests
import json
import datetime

# --- Configuration ---
# It is highly recommended to set these as environment variables for security.
# You can get these keys from your Ambient Weather account page.
# export AMBIENT_WEATHER_API_KEY="your_api_key"
# export AMBIENT_WEATHER_APP_KEY="your_application_key"
# export AMBIENT_WEATHER_MAC_ADDRESS="your_device_mac_address"

API_KEY = "b866de80bec244329adafa0c2a6ec570a457fe4f52bb48aaaa445d54e56437f9"
APP_KEY = "c7d6bae954ba46d1a809dadec1379bcef47806116db74bf1a4fc04835125f110"
MAC_ADDRESS = "84:F3:EB:9C:95:8D"

# Ambient Weather API endpoint for fetching device data
API_URL = "https://api.ambientweather.net/v1/devices"

def get_weather_data():
    """
    Fetches the latest weather data from the Ambient Weather API for a specific device.
    """
    if not API_KEY or not APP_KEY or not MAC_ADDRESS:
        print("Error: Please set the AMBIENT_WEATHER_API_KEY, AMBIENT_WEATHER_APP_KEY, and AMBIENT_WEATHER_MAC_ADDRESS environment variables.")
        return None

    # Construct the full API URL with the MAC address and keys
    full_url = f"{API_URL}/{MAC_ADDRESS}"
    params = {
        "applicationKey": APP_KEY,
        "apiKey": API_KEY,
    }

    try:
        print(f"Fetching data for device {MAC_ADDRESS}...")
        # Make the GET request to the Ambient Weather API
        response = requests.get(full_url, params=params)
        print(f"URL:{full_url}{params}")

        # Raise an exception for bad status codes (4xx or 5xx)
        response.raise_for_status()

        # Parse the JSON response
        weather_data = response.json()

        # The API returns a list of data points, usually with the latest at the beginning.
        if weather_data and isinstance(weather_data, list):
            latest_data = weather_data[0]
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

def display_weather_data(data):
    """
    Prints the fetched weather data in a readable format.
    """
    if not data:
        print("No weather data to display.")
        return

    print("\n--- Ambient Weather Data ---")
    # Convert the UTC timestamp to a readable format
    utc_timestamp = data.get("dateutc")
    if utc_timestamp:
        local_time = datetime.datetime.fromtimestamp(utc_timestamp / 1000)
        print(f"Last updated: {local_time.strftime('%Y-%m-%d %H:%M:%S')}")
    
    # Common weather metrics
    print(f"Temperature (Outdoor): {data.get('tempf')} °F")
    print(f"Humidity (Outdoor): {data.get('humidity')} %")
    print(f"Wind Speed: {data.get('windspeedmph')} mph")
    print(f"Wind Direction: {data.get('winddir')}°")
    print(f"Barometric Pressure: {data.get('baromrelin')} inHg")
    print(f"Rainfall (Today): {data.get('dailyrainin')} in")
    print("----------------------------")
    print(f"WEATHER:{data}")

def main():
    """
    Main function to run the program.
    """
    latest_weather_data = get_weather_data()
    display_weather_data(latest_weather_data)

if __name__ == "__main__":
    main()
