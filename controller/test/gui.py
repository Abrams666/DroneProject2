import sys
import serial
import threading
from PyQt6.QtWidgets import QApplication, QWidget, QLabel
from PyQt6.QtGui import QPainter, QColor, QPen
from PyQt6.QtCore import Qt, QTimer

class FlightDisplay(QWidget):
    def __init__(self):
        super().__init__()

        self.roll = 0
        self.pitch = 0

        self.setWindowTitle("Flight Monitor")
        self.setGeometry(200, 200, 600, 400)

        # 數值顯示
        self.label = QLabel(self)
        self.label.move(10, 10)

        # 啟動 Serial Thread
        self.serial_thread = threading.Thread(target=self.read_serial)
        self.serial_thread.daemon = True
        self.serial_thread.start()

        # 畫面刷新Timer
        self.timer = QTimer()
        self.timer.timeout.connect(self.update)
        self.timer.start(30)

    def read_serial(self):
        ser = serial.Serial("COM3", 115200)  # 改成你的COM
        while True:
            line = ser.readline().decode().strip()
            try:
                r, p, y = line.split(",")
                self.roll = float(r)
                self.pitch = float(p)
            except:
                pass

    def paintEvent(self, event):
        painter = QPainter(self)

        # 背景天空
        painter.setBrush(QColor(80, 160, 255))
        painter.drawRect(0, 0, self.width(), self.height()//2)

        # 地面
        painter.setBrush(QColor(150, 80, 0))
        painter.drawRect(0, self.height()//2, self.width(), self.height()//2)

        # 畫水平線
        painter.setPen(QPen(Qt.GlobalColor.white, 3))
        center_y = self.height()//2 + int(self.pitch * 2)
        painter.drawLine(0, center_y, self.width(), center_y)

        self.label.setText(f"Roll: {self.roll:.2f}  Pitch: {self.pitch:.2f}")

    # 鍵盤監聽
    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_W:
            print("Forward")
        elif event.key() == Qt.Key.Key_S:
            print("Backward")

app = QApplication(sys.argv)
window = FlightDisplay()
window.show()
sys.exit(app.exec())