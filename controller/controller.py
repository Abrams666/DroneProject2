#import
from cvzone.SerialModule import SerialObject
from PyQt6 import QtWidgets
import sys

#const

#config
arduino = SerialObject(portNo='COM6')
app = QtWidgets.QApplication(sys.argv)

#val
keycode = []

#func
def key(self):
    keycode = [int(self.key())]
    print(f"Key pressed: {keycode}")
    arduino.sendData(keycode)

#main
Form = QtWidgets.QWidget()
Form.setWindowTitle('Flight Controller')
Form.resize(1200, 800)
Form.keyPressEvent = key

Form.show()
sys.exit(app.exec())