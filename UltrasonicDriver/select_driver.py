Import("env")

name = env["PIOENV"]

if "idf" in name:
    env.Append(CPPDEFINES=["ULTRASONIC_USE_ESP_IDF"])
elif "arduino" in name:
    env.Append(CPPDEFINES=["ULTRASONIC_USE_ARDUINO"])