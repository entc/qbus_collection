import qbus

#----------------------------------------------------------------------------------------

def method1 (qin):
    print(qin)

#----------------------------------------------------------------------------------------

def module_cb_f (qbus_f):

    print('clb')

#----------------------------------------------------------------------------------------

def module_timer_f (qbus_f):

    qbus_f.send ('SENDB', 'fct1', None, None, module_cb_f)

#----------------------------------------------------------------------------------------

def module_init_f (qbus_f):

    qbus_f.timer (1000, module_timer_f)

#----------------------------------------------------------------------------------------

def module_done_f (qbus_f, obj):
    print(obj)

#---------------------- main entrypoint -------------------------------------------------

if __name__ == "__main__":

    qbus_b = qbus.QBus("SENDB", {})

    # run one instance in background
    qbus_b.run_d()

    # run one instance in foreground
    qbus.instance("SENDF", module_init_f, module_done_f)

#----------------------------------------------------------------------------------------
