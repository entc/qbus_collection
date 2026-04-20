import qbus
import time
import signal

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

#----------------------------------------------------------------------------------------

def module_run (qbus):

    qbus.run_d()

    try:
        while True:
            print("tick")
            time.sleep(1)

    except KeyboardInterrupt:
        print("python: termination detected")

#---------------------- main entrypoint -------------------------------------------------

if __name__ == "__main__":

    qbus = qbus.QBus("Test", {})

    qbus.set_cb (module_init, module_done)

    if signal.getsignal(signal.SIGINT) == signal.SIG_IGN:

        raise Exception("SIGINT is disabled")

    else:

        module_run (qbus)

#----------------------------------------------------------------------------------------
