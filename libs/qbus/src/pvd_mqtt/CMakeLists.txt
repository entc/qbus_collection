INCLUDE_DIRECTORIES(".." "paho.mqtt.c/src" "../../../cape/src" "../pvd_common")

# external libs
SUBDIRS(paho.mqtt.c)

SET (PVD_MQTT_SOURCES
  qbus_mqtt.c
)

SET (PVD_MQTT_HEADERS
  qbus_mqtt.h
)

ADD_LIBRARY (qbus_pvd_mqtt SHARED ${PVD_MQTT_SOURCES} ${PVD_MQTT_HEADERS})
TARGET_LINK_LIBRARIES (qbus_pvd_mqtt cape qbus_pvd_common eclipse-paho-mqtt-c::paho-mqtt3c)
SET_TARGET_PROPERTIES (qbus_pvd_mqtt PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PVD_QBUS_DIR})

INSTALL (TARGETS qbus_pvd_mqtt DESTINATION bin/qbus)
