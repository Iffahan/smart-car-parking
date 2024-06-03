#!/usr/bin/env python3
from flask import Flask, request, abort
from linebot import LineBotApi, WebhookHandler
from linebot.exceptions import InvalidSignatureError
from linebot.models import MessageEvent, TextMessage, TextSendMessage
import random
import time
import serial
import os
import requests
import urllib

app = Flask(__name__)

line_bot_api = LineBotApi('byKPge0R4LOX1aY2pIC2pwqlHvw3CgW/1ZsN1LNrognU/PJ375hR+2yqKHzmIx6drbFgoB5kzC1VmwI+lNlivFExsvlOgFMrdIhARHZEshCGk82JlXJVaKyT+yxpAFNqjd68mssTr4tEAPKUrvx1EQdB04t89/1O/w1cDnyilFU=')
handler = WebhookHandler('39f9bd339fd948b95e829e6b36700614')
ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)

def generate_random_password():
    return str(random.randint(0, 999999)).zfill(6)

state = None
password_given = False
password_regeneration_time = 900
last_regeneration_time = time.time()
password = generate_random_password()

def regenerate_password():
    global password, last_regeneration_time
    while True:
        time.sleep(password_regeneration_time)
        password = generate_random_password()
        last_regeneration_time = time.time()
        print("Password Regenerated")

def send_to_thingspeak(state):
    try:
        baseURL = 'https://api.thingspeak.com/update?api_key=Z8DKVJVU0D607NYK&field1='
        f=urllib.request.urlopen(baseURL+str(state))
        f.read()
        f.close()
        print(f"Sent state {state} to ThingSpeaks")
    except Exception as e:
        print(f"Error sending data to ThingSpeaks: {e}")

def receive_data():
    global password, state, password_given
    while True:
        try:
            data = ser.read_until(b'\n').decode().strip()
            if data:
                try:
                    state = int(data)
                    if state == 3:
                        print(f"state is {state}")
                        password_given = False
                        password = generate_random_password()
                        print("Password Changed")
                    if state == 0:
                        print(f"Received state {state} from Arduino. Parking is Free {password}")
                    elif state == 1:
                        print(f"Received state {state} from Arduino. Parking is Full")
                except ValueError:
                    print("Error, Invalid data from Arduino")
        except ValueError:
            print("Error, Invalid data from Arduino")

        ser.write(password.encode('utf-8'))
        time.sleep(1)

@app.route("/callback", methods=['POST'])
def callback():
    signature = request.headers['X-Line-Signature']
    body = request.get_data(as_text=True)
    try:
        handler.handle(body, signature)
    except InvalidSignatureError:
        abort(400)

    return 'OK'

@handler.add(MessageEvent, message=TextMessage)
def handle_message(event):
    global state, password, password_given
    if event.message.text == 'ขอจองการจอดรถ':
        if state == 0:
            if not password_given:
                last_regeneration_time = time.time()
                expiration_time = time.strftime("%H.%M", time.localtime(last_regeneration_time + password_regeneration_time))
                response = f'รหัสผ่านที่จอดรถ คือ {password} โปรดเข้าจอดรถภายใน 15 นาที หากเกินว่านี้จะต้องทำการจองใหม่\nรหัสนี้จะหมดอายุเวลา {expiration_time}'
                password_given = True
            else:
                remaining_time = int(password_regeneration_time - (time.time() - last_regeneration_time))
                response = f'ที่จอดรถเต็ม กรุณารอและจองใหม่ โปรดตรวจสอบใหม่อีกครั้ง\nรหัสใหม่จะสามารถขอได้ในอีก {remaining_time} วินาที'
        else:
            response = f'Not available now.'

    elif event.message.text == 'ตรวจสอบสถานะที่จอด':
        if state == 0:
            if not password_given:
                response = f'ที่จอดรถว่างอยู่ คุณสามารถจองรถได้'
            else:
                response = f'ที่จอดรถว่างอยู่ แต่มีผู้จองแล้ว กรุณาลองใหม่อีกครั้ง'
        else:
            response = f'ขออภัย ที่จอดรถว่างไม่ว่าง คุณสามารถจองที่จอดรถได้'
    else:
        response = '`ไม่มคำสั่งนี้ กรุณาพิมพ์คำสั่งใหม่ หรือกดได้ที่รูปในเมนู'

    line_bot_api.reply_message(event.reply_token, TextSendMessage(text=response))


if __name__ == "__main__":
    # Run receive_data in a separate thread to continuously receive data from Arduino
    import threading
    receive_data_thread = threading.Thread(target=receive_data)
    regenerate_password_thread = threading.Thread(target=regenerate_password)
    send_to_thingspeak_thread = threading.Thread(target=send_to_thingspeak, args=(state,))
    receive_data_thread.start()
    regenerate_password_thread.start()
    send_to_thingspeak_thread.start()

    # Use Ngrok for local testing
    if "NGROK_URL" in os.environ:
        ngrok_url = os.environ["NGROK_URL"]
        print(f"Using Ngrok URL: {ngrok_url}")
        # Update the Line webhook URL dynamically with Ngrok URL
        line_bot_api.set_webhook_endpoint(f"{ngrok_url}/callback")
    # Run the Flask app
    send_to_thingspeak(state)
    app.run(port=3000)
