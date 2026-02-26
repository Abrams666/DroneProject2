import pyvisual as pv
from ui.ui import create_ui
from cvzone.SerialModule import SerialObject

arduino = SerialObject()
mySwitch = [False]

# ===================================================
# ================ 1. LOGIC CODE ====================
# ===================================================

def hello(btn):
    mySwitch[0] = not mySwitch[0]
    if mySwitch[0]:
        arduino.sendData([1])
        #text.text = "on"
    else:
        arduino.sendData([0])
        #text.text = "off"
# ===================================================
# ============== 2. EVENT BINDINGS ==================
# ===================================================


def attach_events(ui):
    ui["page_0"]["test"].on_click = hello

    pass

# ===================================================
# ============== 3. MAIN FUNCTION ==================
# ===================================================


def main():
    app = pv.PvApp()
    ui = create_ui()
    attach_events(ui)
    ui["window"].show()
    app.run()

    # print("Starting to read data from Arduino...")
    # while True:
    #     data = arduino.getData()
    #     print(data)


if __name__ == '__main__':
    main()

