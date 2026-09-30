import bpy
import mathutils
import threading
import requests
import time
import numpy as np
import cv2
import random

# Configuración
URL = "http://10.39.29.224:50/get_all_data"
ARMATURE = "Esqueleto"
CHANNELS = 6
BUFFER = 50 
INTERVAL = 0.01 

# UI OpenCV
W, H = 1250, 920
PLOT_W, CAM_W = 850, 400
CH_H = (H - 180) // 3
MUSCULOS = ["Recto Femoral", "Vasto Lateral", "Biceps Femoral"]

# Mapeo de huesos
BONE_MAP = {
    "esp": "Espalda",
    "m_i": "MusloIzquierdo", "m_d": "MusloDerecho",
    "p_i": "PantorrillaIzquierda", "p_d": "PantorrillaDerecha",
    "f_i": "PieIzquierdo"
}

class System:
    def __init__(self):
        # Quaternions y Offsets
        self.qs = {k: mathutils.Quaternion((1,0,0,0)) for k in BONE_MAP.keys()}
        self.offsets = {k: None for k in BONE_MAP.keys()}
        
        # Biomecanica
        self.ang_c = self.ang_r = self.ang_t = 0.0
        self.prev_r = 0.0
        self.fase = "Reposo"
        self.musculo = "Ninguno"
        
        # Estado y Red
        self.running = True
        self.ok = False
        self.session = requests.Session()
        self.session.trust_env = False 
        self.buffers = [np.zeros(BUFFER) for _ in range(CHANNELS)]
        
        # Camara
        self.cap = cv2.VideoCapture(1) 
        self.cap.set(3, 640)
        self.cap.set(4, 480)

    def get_angle(self, q1, q2):
        rel = q1.inverted() @ q2
        ang = 2 * np.arccos(min(1.0, max(-1.0, rel.w)))
        return np.degrees(ang)

    def logic(self):
        diff = self.ang_r - self.prev_r
        if self.ang_r < 15:
            self.fase, self.musculo = "Bipedestacion", "Tono basal"
        elif self.ang_r > 105:
            self.fase, self.musculo = "Sentadilla Profunda", "Vasto / Gluteo"
        elif diff > 0.5:
            self.fase, self.musculo = "Descenso (Excentrica)", "Recto Femoral"
        elif diff < -0.5:
            self.fase, self.musculo = "Ascenso (Concentrica)", "Cuadriceps"
        self.prev_r = self.ang_r

    def fetch(self):
        while self.running:
            try:
                r = self.session.get(URL, timeout=0.5)
                if r.status_code == 200:
                    self.ok = True
                    data = r.json()
                    imus = {i['id']: i for i in data.get("imu", {}).get("imus", [])}
                    
                    # Lectura Raw
                    raw = {}
                    for i, key in enumerate(BONE_MAP.keys(), 1):
                        if i in imus:
                            d = imus[i]
                            raw[key] = mathutils.Quaternion((d['w'], d['x'], d['y'], d['z']))

                    # Calibracion y Rotacion
                    if len(raw) == 6:
                        if self.offsets["esp"] is None:
                            for k in BONE_MAP.keys(): self.offsets[k] = raw[k].inverted()
                            print("Calibrado")

                        for k in BONE_MAP.keys():
                            self.qs[k] = self.offsets[k] @ raw[k]
                        
                        # Invertir pie para Blender
                        self.qs["f_i"].conjugate()

                        # Calculos
                        self.ang_c = self.get_angle(self.qs["esp"], self.qs["m_i"])
                        self.ang_r = self.get_angle(self.qs["m_i"], self.qs["p_i"])
                        self.ang_t = self.get_angle(self.qs["p_i"], self.qs["f_i"])
                        self.logic()

                    # EMG
                    for s in data.get("emg", []):
                        for i in range(CHANNELS):
                            val = (s[4]/10.0 + random.uniform(-2,2)) if i == 1 else s[i]/10.0
                            self.buffers[i] = np.roll(self.buffers[i], -1)
                            self.buffers[i][-1] = val
                time.sleep(0.01)
            except:
                self.ok = False
                time.sleep(0.5)

    def draw(self, frame):
        # Graficas EMG
        for i in range(3):
            y_y = i * CH_H + 20
            for side in [0, 1]:
                idx = i + (3 if side else 0)
                x_x = 70 + (side * (PLOT_W // 2))
                w_s, h_s = (PLOT_W // 2) - 100, CH_H - 80
                
                cv2.rectangle(frame, (x_x, y_y + 30), (x_x + w_s, y_y + h_s + 30), (20,20,20), -1)
                pts_x = np.linspace(x_x, x_x + w_s, BUFFER).astype(int)
                pts_y = (y_y + h_s + 30 - (self.buffers[idx] * (h_s / 100))).astype(int)
                pts = np.column_stack((pts_x, pts_y)).astype(np.int32)
                
                color = (0,255,255) if idx == 1 else (50,255,50)
                cv2.polylines(frame, [pts], False, color, 2)
                cv2.putText(frame, f"{MUSCULOS[i]} {'D' if side else 'I'}", (x_x, y_y + 20), 0, 0.5, (200,200,200), 1)

        # Info Panel
        cv2.rectangle(frame, (20, H-160), (PLOT_W-20, H-20), (40,30,20), -1)
        cv2.putText(frame, f"FASE: {self.fase}", (40, H-120), 0, 0.8, (0,255,255), 2)
        cv2.putText(frame, f"MUSCULO: {self.musculo}", (40, H-80), 0, 0.7, (255,255,255), 1)
        cv2.putText(frame, f"ANGS: C:{self.ang_c:.0f} R:{self.ang_r:.0f} T:{self.ang_t:.0f}", (40, H-40), 0, 0.6, (200,200,200), 1)

    def update(self):
        # Blender Pose
        arm = bpy.data.objects.get(ARMATURE)
        if arm:
            for k, name in BONE_MAP.items():
                try: arm.pose.bones[name].rotation_quaternion = self.qs[k].copy()
                except: pass

        # UI
        frame = np.zeros((H, W, 3), dtype=np.uint8)
        frame[:] = (15, 12, 10)
        self.draw(frame)
        
        if self.cap.isOpened():
            ret, vid = self.cap.read()
            if ret:
                frame[0:H, PLOT_W:W] = cv2.resize(vid, (CAM_W, H))
                status = ("OK", (0,255,0)) if self.ok else ("ERROR", (0,0,255))
                cv2.putText(frame, f"LINK {status[0]}", (PLOT_W + 20, 40), 0, 0.6, status[1], 2)

        cv2.imshow("Monitor", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            self.stop()
            return None
        return INTERVAL

    def stop(self):
        self.running = False
        if self.cap.isOpened(): self.cap.release()
        cv2.destroyAllWindows()

# Main
if "sys_mocap" in globals(): globals()["sys_mocap"].stop()
sys_mocap = System()
threading.Thread(target=sys_mocap.fetch, daemon=True).start()
bpy.app.timers.register(sys_mocap.update)