import qbus
import time

#----------------------------------------------------------------------------------------

def method1 (qin):
  print(qin)

#----------------------------------------------------------------------------------------

def module_init (qbus):
  # register QBUS methods
  qbus.register ("method1", method1)

#----------------------------------------------------------------------------------------

def module_done (qbus, obj):
  print(obj)

#---------------------- main entrypoint -------------------------------------------------

if __name__ == "__main__":

    qbus = qbus.QBus("Test", {})

    qbus.run_d()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("python: termination detected")

#----------------------------------------------------------------------------------------
