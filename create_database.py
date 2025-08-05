import os
import time
import sqlite3
import datetime
import json
import requests

DATABASE_NAME = "beargrass-analytics.db"

def setup_database():
    """
    Sets up the SQLite database and creates the table for storing MQTT messages.
    This function is now a one-time setup and does not keep a persistent connection.
    """
    try:
        # Create a temporary connection just for creating the table
        db_connection = sqlite3.connect(DATABASE_NAME)
        cursor = db_connection.cursor()
        # Drop the old table to ensure the new schema is used
        cursor.execute("DROP TABLE IF EXISTS mqtt_messages;")
        cursor.execute("""
            CREATE TABLE mqtt_messages (
                db_id INTEGER PRIMARY KEY AUTOINCREMENT,
                feed_id TEXT NOT NULL,
                message_id TEXT,
                timestamp TEXT,
                mA REAL,
                RSSI REAL,
                SNR REAL
            );
        """)
        db_connection.commit()

        cursor.execute("DROP TABLE IF EXISTS weather_data;")
        cursor.execute("""
            CREATE TABLE weather_data (
                db_id INTEGER PRIMARY KEY AUTOINCREMENT,
                message_id TEXT,
                pressure REAL,
                temperature REAL,
                hourlyrain REAL,
                eventrain REAL,
                dailyrain REAL,
                totalrain REAL,
                monthlyrain REAL,
                weeklyrain REAL,
                humidity REAL,
                maxdailygust REAL,
                date TEXT,
                datelastrain TEXT
            );
        """)
        db_connection.commit()
        db_connection.close()
        print(f"Database '{DATABASE_NAME}' set up successfully with new schema.")
        return True
    except sqlite3.Error as e:
        print(f"Error setting up database: {e}")
        return False



        print(f"Database '{DATABASE_NAME}' set up successfully with new schema.")
        return True
    except sqlite3.Error as e:
        print(f"Error setting up database: {e}")
        return False

def main():
    setup_database()

if __name__ == "__main__":
    main()
