# abstract operation-system layer
INCLUDE_DIRECTORIES("../paho.mqtt.c/src")

#----------------------------------------------------------------------------------------

add_executable          (ut_pvd_mqtt_signals ut_pvd_mqtt_signals.c)
target_link_libraries   (ut_pvd_mqtt_signals eclipse-paho-mqtt-c::paho-mqtt3c)

#----------------------------------------------------------------------------------------
