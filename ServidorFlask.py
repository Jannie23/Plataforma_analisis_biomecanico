# -*- coding: utf-8 -*-
from flask import Flask, request, jsonify
from threading import Lock

app = Flask(__name__)

# Datos Globales
latest_imu = None
latest_emg = None

# Locks
imu_lock = Lock()
emg_lock = Lock()

# --- ENDPOINTS ESCRITURA ---

@app.route('/update_imu', methods=['POST'])
def update_imu():
    global latest_imu
    data = request.get_json()
    with imu_lock:
        latest_imu = data
    return jsonify({"status": "ok"}), 200

@app.route('/update_emg', methods=['POST'])
def update_emg():
    global latest_emg
    data = request.get_json()
    with emg_lock:
        latest_emg = data
    return jsonify({"status": "ok"}), 200

# --- ENDPOINTS LECTURA ---

@app.route('/get_imu', methods=['GET'])
def get_imu():
    with imu_lock:
        return jsonify(latest_imu)

@app.route('/get_emg', methods=['GET'])
def get_emg():
    with emg_lock:
        return jsonify(latest_emg)

@app.route('/get_all_data', methods=['GET'])
def get_all_data():
    with imu_lock:
        i_snap = latest_imu
    with emg_lock:
        e_snap = latest_emg
    return jsonify({
        "imu": i_snap,
        "emg": e_snap
    })

if __name__ == '__main__':
    print("Servidor iniciado en puerto 80")
    app.run(host='0.0.0.0', port=80)