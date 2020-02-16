#!/usr/bin/python3

import sys

fileName = ""
for param in sys.argv:
    if param.find(".avi") != -1 or param.find(".mp4") != -1:
        fileName = param
        break

if len(fileName) == 0:
    print("Usage: video.py <file>.avi")
    exit(1)


import numpy as np
import cv2
from crccheck.crc import Crc16CcittFalse
import wave, struct


def decodeFrame(frame):
    L = []
    R = []
    for line in range(0, (len(frame) - 7 * 16)):
        if True:
            #14 bit
            L0 = (frame[line + 0][0] << 2)
            R0 = (frame[line + 1][1] << 2)
            L1 = (frame[line + 2][2] << 2)
            R1 = (frame[line + 3][3] << 2)
            L2 = (frame[line + 4][4] << 2)
            R2 = (frame[line + 5][5] << 2)
            P  = (frame[line + 6][6] << 2)
            Q  = (frame[line + 7][7] << 2)

        else:
            # 16bit
            S  = frame[line + 7][7]
            L0 = (frame[line + 0][0] << 2) | ((S >> 12) & 0b11)
            R0 = (frame[line + 1][1] << 2) | ((S >> 10) & 0b11)
            L1 = (frame[line + 2][2] << 2) | ((S >>  8) & 0b11)
            R1 = (frame[line + 3][3] << 2) | ((S >>  6) & 0b11)
            L2 = (frame[line + 4][4] << 2) | ((S >>  4) & 0b11)
            R2 = (frame[line + 5][5] << 2) | ((S >>  2) & 0b11)
            P  = (frame[line + 6][6] << 2) | ((S >>  0) & 0b11)

        #parityCheck = L0 ^ L1 ^ L2 ^ R0 ^ R1 ^ R2

        waveFile.writeframesraw(struct.pack('<h', np.int16(L0)))
        waveFile.writeframesraw(struct.pack('<h', np.int16(L1)))
        waveFile.writeframesraw(struct.pack('<h', np.int16(L2)))
        #L.append(L0)
        #L.append(L1)
        #L.append(L2)
        #R.append(R0)
        #R.append(R1)
        #R.append(R2)

    return(L)


video = cv2.VideoCapture(fileName)

# TODO
start  = 24
end = 677
pixelSize = 5.1015625

#fileName = fileName[:fileName.find(".")]

waveFile = wave.open(fileName + ".wav", mode="wb")
waveFile.setnchannels(1)
waveFile.setsampwidth(2)
waveFile.setframerate(44100)

# 7 * 16 - высота лесенки
oldPCMFrame = []

for i in range(0, 7*16):
    oldPCMFrame.append([0,0,0,0,0,0,0,0])

while(video.isOpened()):
    ret, frame = video.read()

    if not ret:
        break

    ret, threshFrame = cv2.threshold(frame, 79, 255, cv2.THRESH_BINARY)

    def preparePCMFrame(frame, offset):
        pcm = []
        for j in range(offset, int(video.get(cv2.CAP_PROP_FRAME_HEIGHT)), 2):
            line = frame[j,:][:,1]
            linePCM = []
            for i in range(0, 128):
                if line[int(i * pixelSize + start + 2)]:
                    linePCM.append(1)
                else:
                    linePCM.append(0)

            dataBits = np.split(np.array(linePCM[:112]), 8)
            crcControlData = np.packbits(dataBits)
            data = []
            for x in dataBits:
                data.append(np.packbits(x))

            crc = np.packbits(linePCM[112:])
            crc = crc[0] << 8 | crc[1]
            #crc = linePCM[14:].view(np.uint16)
            crcControlValue = Crc16CcittFalse.calc(bytearray(crcControlData))
            if crc != crcControlValue:
                print("Error CRC: %d, %d, %d"%(j, crc, crcControlValue))

            #todo .view(np.uint16)
            linePCM = []
            linePCM.append(data[0][0] << 8 | data[0][1])
            linePCM.append(data[1][0] << 8 | data[1][1])
            linePCM.append(data[2][0] << 8 | data[2][1])
            linePCM.append(data[3][0] << 8 | data[3][1])
            linePCM.append(data[4][0] << 8 | data[4][1])
            linePCM.append(data[5][0] << 8 | data[5][1])
            linePCM.append(data[6][0] << 8 | data[6][1])
            linePCM.append(data[7][0] << 8 | data[7][1])
            pcm.append(linePCM)

        return(pcm)

    # Нужно разбить изображение на два поля
    PCMFrame = oldPCMFrame + preparePCMFrame(threshFrame, 0)
    waveFile.writeframesraw(np.array(decodeFrame(PCMFrame)).tobytes())
    oldPCMFrame = PCMFrame[(len(PCMFrame) - 7*16):]

    PCMFrame = oldPCMFrame + preparePCMFrame(threshFrame, 1)
    waveFile.writeframesraw(np.array(decodeFrame(PCMFrame)).tobytes())
    oldPCMFrame = PCMFrame[(len(PCMFrame) - 7*16):]

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    cv2.imshow('frame', gray)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

video.release()
cv2.destroyAllWindows()
