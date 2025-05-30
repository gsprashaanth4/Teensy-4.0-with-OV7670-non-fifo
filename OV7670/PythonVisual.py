import serial
import numpy as np
import cv2

# Set up serial port, adjust if needed
ComPort = 'COM22'
BaudRate = 4000000
ser = serial.Serial(ComPort, BaudRate, timeout=10)

width, height = 60, 60
frame_size = width * height  # 1 byte per pixel

print("Waiting for image data...")

while True:
    data = ser.read(frame_size)
    if len(data) != frame_size:
        print("Incomplete frame received")
        continue

    # Convert to grayscale image
    frame = np.frombuffer(data, dtype=np.uint8).reshape((height, width))

    # Resize to make pixels larger
    # scale = 30  # 20× scaling makes it 400×400 on screen
    # enlarged = cv2.resize(frame, (width * scale, height * scale), interpolation=cv2.INTER_NEAREST)

    # Display
    # cv2.imshow("OV7670 20x20 Zoomed", enlarged) # uncomment for blocked-Magnified image
    cv2.imshow("OV7670 20x20 Zoomed", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

ser.close()
cv2.destroyAllWindows()
