# The Wacom EMR digitizer does not set INPUT_PROP_DIRECT, so InputReader
# classifies it as a POINTER device: uniform scaling without display
# rotation handling, which breaks pen input in landscape.
# Treat it as a touchscreen so it runs in DIRECT mode (physical-frame
# mapping + orientation awareness).
touch.deviceType = touchScreen
touch.orientationAware = 1
