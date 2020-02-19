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

L0_POS = 0
R0_POS = 16
L1_POS = 32
R1_POS = 48
L2_POS = 64
R2_POS = 80
P_POS  = 96
Q_POS  = 112
S_POS  = 112

def decodeFrame(frame):

    L = []
    R = []
    for line in range(0, (len(frame) - 7 * 16)):
        if False:
        #if True:
            #14 bit
            L0 = frame[line + L0_POS][0]
            R0 = frame[line + R0_POS][1]
            L1 = frame[line + L1_POS][2]
            R1 = frame[line + R1_POS][3]
            L2 = frame[line + L2_POS][4]
            R2 = frame[line + R2_POS][5]
            P  = frame[line +  P_POS][6]
            #L0 = (frame[line + L0_POS][0] << 2)
            #R0 = (frame[line + R0_POS][1] << 2)
            #L1 = (frame[line + L1_POS][2] << 2)
            #R1 = (frame[line + R1_POS][3] << 2)
            #L2 = (frame[line + L2_POS][4] << 2)
            #R2 = (frame[line + R2_POS][5] << 2)
            #P  = (frame[line +  P_POS][6] << 2)
            #Q  = (frame[line +  Q_POS][7] << 2)

        else:
            # 16bit
            S  = frame[line + S_POS][7]
            #L0 = ((S >> 12) & 0b11) | frame[line + L0_POS][0]
            #R0 = ((S >> 10) & 0b11) | frame[line + R0_POS][1]
            #L1 = ((S >> 10) & 0b11) | frame[line + L1_POS][2]
            #R1 = ((S >> 10) & 0b11) | frame[line + R1_POS][3]
            #L2 = ((S >> 10) & 0b11) | frame[line + L2_POS][4]
            #R2 = ((S >> 10) & 0b11) | frame[line + R2_POS][5]
            #P  = ((S >> 10) & 0b11) | frame[line +  P_POS][6]
            L0 = (frame[line + L0_POS][0] << 2) & 0xffff | ((frame[line + L0_POS][7] >> 12) & 0b11)
            R0 = (frame[line + R0_POS][1] << 2) & 0xffff | ((frame[line + R0_POS][7] >> 10) & 0b11)
            L1 = (frame[line + L1_POS][2] << 2) & 0xffff | ((frame[line + L1_POS][7] >>  8) & 0b11)
            R1 = (frame[line + R1_POS][3] << 2) & 0xffff | ((frame[line + R1_POS][7] >>  6) & 0b11)
            L2 = (frame[line + L2_POS][4] << 2) & 0xffff | ((frame[line + L2_POS][7] >>  4) & 0b11)
            R2 = (frame[line + R2_POS][5] << 2) & 0xffff | ((frame[line + R2_POS][7] >>  2) & 0b11)
            P  = (frame[line +  P_POS][6] << 2) & 0xffff | ((frame[line +  P_POS][7] >>  0) & 0b11)

        parityCheck = 0
        parityCheck = L0 ^ L1 ^ L2 ^ R0 ^ R1 ^ R2

        if (parityCheck != P) and frame[line + P_POS][8]:
            print("parity error")
            print(frame[line])
            print("L0 {0:06d} {1:>016b}".format(L0, frame[line +  0][0]))
            print("L1 {0:06d} {1:>016b}".format(L1, frame[line + 16][1]))
            print("L2 {0:06d} {1:>016b}".format(L2, frame[line + 32][2]))
            print("R0 {0:06d} {1:>016b}".format(R0, frame[line + 48][3]))
            print("R1 {0:06d} {1:>016b}".format(R1, frame[line + 64][4]))
            print("R2 {0:06d} {1:>016b}".format(R2, frame[line + 80][5]))
            print("P  {0:06d} {0:>016b}".format(P))
            print("PC {0:06d} {0:>016b}".format(parityCheck))
            parityCheck = np.bitwise_xor(L0, L1)
            print("PC {0:06d} {0:>016b}".format(parityCheck))
            parityCheck = np.bitwise_xor(L2, parityCheck)
            print("PC {0:06d} {0:>016b}".format(parityCheck))
            parityCheck = np.bitwise_xor(R0, parityCheck)
            print("PC {0:06d} {0:>016b}".format(parityCheck))
            parityCheck = np.bitwise_xor(R1, parityCheck)
            print("PC {0:06d} {0:>016b}".format(parityCheck))
            parityCheck = np.bitwise_xor(R2, parityCheck)
            print("PC {0:06d} {0:>016b}".format(parityCheck))

            if frame[line + L0_POS][8] == 0:
                L0 = P ^ L1 ^ L2 ^ R0 ^ R1 ^ R2
            elif frame[line + L1_POS][8] == 0:
                L1 = L0 ^ P ^ L2 ^ R0 ^ R1 ^ R2
            elif frame[line + L2_POS][8] == 0:
                L2 = L0 ^ L1 ^ P ^ R0 ^ R1 ^ R2
            elif frame[line + R0_POS][8] == 0:
                R0 = L0 ^ L1 ^ L2 ^ P ^ R1 ^ R2
            elif frame[line + R1_POS][8] == 0:
                R1 = L0 ^ L1 ^ L2 ^ R0 ^ P ^ R2
            elif frame[line + R2_POS][8] == 0:
                R2 = L0 ^ L1 ^ L2 ^ R0 ^ R1 ^ P

            parityCheck = L0 ^ L1 ^ L2 ^ R0 ^ R1 ^ R2
            if (parityCheck != P):
                print("parity error2")
                print("L0 {0:06d} {1:>016b}".format(L0, frame[line +  0][0]))
                print("L1 {0:06d} {1:>016b}".format(L1, frame[line + 16][1]))
                print("L2 {0:06d} {1:>016b}".format(L2, frame[line + 32][2]))
                print("R0 {0:06d} {1:>016b}".format(R0, frame[line + 48][3]))
                print("R1 {0:06d} {1:>016b}".format(R1, frame[line + 64][4]))
                print("R2 {0:06d} {1:>016b}".format(R2, frame[line + 80][5]))

                print("P  {0:06d} {0:>016b}".format(P))
                print("PC {0:06d} {0:>016b}".format(parityCheck))

                #exit()

        #print("L0 {0:>016b}".format(L0))
        #print("L1 {0:>016b}".format(L1))
        #print("L2 {0:>016b}".format(L2))
        #print("R0 {0:>016b}".format(R0))
        #print("R1 {0:>016b}".format(R1))
        #print("R2 {0:>016b}".format(R2))
        #exit()


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
start  = 30
end = 677
pixelSize = 5#.1015625

#fileName = fileName[:fileName.find(".")]

waveFile = wave.open(fileName + ".wav", mode="wb")
waveFile.setnchannels(1)
waveFile.setsampwidth(2)
waveFile.setframerate(44100)

# 7 * 16 - высота лесенки
#oldPCMFrame = np.full((7 * 16, 9), 0)

oldPCMFrame = []


while(video.isOpened()):
    ret, frame = video.read()

    if not ret:
        break

    ret, threshFrame = cv2.threshold(frame, 79, 255, cv2.THRESH_BINARY)

    def preparePCMFrame(frame, offset):
        pcm = []
        for j in range(offset + 1, int(video.get(cv2.CAP_PROP_FRAME_HEIGHT)), 2):
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
                if crc == 0:
                    continue

                crc = 0
                print("Error CRC: %d, %d, %d"%(j, crc, crcControlValue))

            #todo .view(np.uint16)
            linePCM = []
            linePCM.append(np.uint16(data[0][0]) << 8 | data[0][1] & 0xffff)
            linePCM.append(np.uint16(data[1][0]) << 8 | data[1][1] & 0xffff)
            linePCM.append(np.uint16(data[2][0]) << 8 | data[2][1] & 0xffff)
            linePCM.append(np.uint16(data[3][0]) << 8 | data[3][1] & 0xffff)
            linePCM.append(np.uint16(data[4][0]) << 8 | data[4][1] & 0xffff)
            linePCM.append(np.uint16(data[5][0]) << 8 | data[5][1] & 0xffff)
            linePCM.append(np.uint16(data[6][0]) << 8 | data[6][1] & 0xffff)
            linePCM.append(np.uint16(data[7][0]) << 8 | data[7][1] & 0xffff)
            linePCM.append(crc)
            pcm.append(linePCM)

        #for line in range(0, len(pcm)):
        #    print(pcm[line]);
        return(pcm)

    # Нужно разбить изображение на два поля
    PCMFrame = oldPCMFrame + preparePCMFrame(threshFrame, 0)
    waveFile.writeframesraw(np.array(decodeFrame(PCMFrame)).tobytes())
    oldPCMFrame = PCMFrame[(len(PCMFrame) - 112):]

    PCMFrame = oldPCMFrame + preparePCMFrame(threshFrame, 1)
    waveFile.writeframesraw(np.array(decodeFrame(PCMFrame)).tobytes())
    oldPCMFrame = PCMFrame[(len(PCMFrame) - 112):]

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    cv2.imshow('frame', gray)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

video.release()
cv2.destroyAllWindows()
