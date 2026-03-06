#import
from PyQt6 import QtWidgets, QtCore
import sys
import serial
import json

#setup
arduino = serial.Serial(port="COM5", baudrate=115200, timeout=0.1)
app = QtWidgets.QApplication(sys.argv)

#data template
TxTemplate = {
    "pitch": 0,
    "roll": 0,
    "yaw": 0,
    "basicThrust": 1000,
    "stop": 0
}

#vals
quantity = [10,10,1000]
TxData = TxTemplate.copy()
keycode = 0

#function

#class
class SerialWorker(QtCore.QThread):
    data_received = QtCore.pyqtSignal(dict)

    def __init__(self):
        super().__init__()
        self.running = True

    def run(self):
        while self.running:
            try:
                print((json.dumps(TxData) + "\n").encode())

                arduino.write((json.dumps(TxData) + "\n").encode())

                line = arduino.readline().decode().strip()
                if line:
                    data = json.loads(line)
                    self.data_received.emit(data)
            except:
                pass

    def stop(self):
        self.running = False
        self.wait()

class MainWindow(QtWidgets.QWidget):

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Flight Controller")
        self.resize(800,600)

        self.worker = SerialWorker()
        self.worker.data_received.connect(self.handle_data)
        self.worker.start()

    def handle_data(self, data):
        print("Received:", data)

    def keyPressEvent(self, event):
        keycode = int(event.key())
        #print("Key:", event.key())

        global TxData
        
        if(keycode == 88): #X
            TxData["stop"] = 1
        elif(keycode == 87): #W
            TxData["pitch"] = -quantity[0]
        elif(keycode == 83): #S
            TxData["pitch"] = quantity[0]
        elif(keycode == 65): #A
            TxData["roll"] = -quantity[1]
        elif(keycode == 68): #D
            TxData["roll"] = quantity[1]
        elif(keycode == 81): #Q
            TxData["yaw"] = -1
        elif(keycode == 69): #E
            TxData["yaw"] = 1
        elif(keycode == 82): #R
            quantity[2] += 20
        elif(keycode == 70): #F
            quantity[2] -= 20

        TxData["basicThrust"] = quantity[2]
        TxTemplate["basicThrust"] = quantity[2]

    def keyReleaseEvent(self, event):
        global TxData

        TxData = TxTemplate.copy()

#main
window = MainWindow()
window.show()
sys.exit(app.exec())