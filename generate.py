#!/usr/bin/python3

import sys

fileName = ""
for param in sys.argv:
    if param.find(".wav") != -1:
        fileName = param
        break

if len(fileName) == 0:
    print("Usage: generate.py <file>.wav")
    exit(1)


import numpy as np
import cv2
from crccheck.crc import Crc16CcittFalse
import wave, struct



def encodeFrame(frame):
    for line in range(0, (len(frame) - 7 * 16)):
        data = np.frombuffer(waveFile.readframes(3), dtype=np.int16)
        L0 = data[0].view(np.uint16)
        R0 = data[1].view(np.uint16)
        L1 = data[2].view(np.uint16)
        R1 = data[3].view(np.uint16)
        L2 = data[4].view(np.uint16)
        R2 = data[5].view(np.uint16)
        #R0 = L0
        #R1 = L1
        #R2 = L2
        if False:
        #if True:
            #14 bit
            P = L0 ^ L1 ^ L2 ^ R0 ^ R1 ^ R2
            Q = 0x3fff
            frame[line +   0][0] = L0
            frame[line +  16][1] = R0
            frame[line +  32][2] = L1
            frame[line +  48][3] = R1
            frame[line +  64][4] = L2
            frame[line +  80][5] = R2
            frame[line +  96][6] = P
            frame[line + 112][7] = Q

        else:
            # 16bit
            P = L0 ^ L1 ^ L2 ^ R0 ^ R1 ^ R2
            S = (L0 & 0b11) << 12 | (R0 & 0b11) << 10 | (L1 & 0b11) << 8 | (R1 & 0b11) << 6 | (L2 & 0b11) << 4 | (R2 & 0b11) << 2 | (P & 0b11)
            frame[line +   0][0] = L0 >> 2
            frame[line +  16][1] = R0 >> 2
            frame[line +  32][2] = L1 >> 2
            frame[line +  48][3] = R1 >> 2
            frame[line +  64][4] = L2 >> 2
            frame[line +  80][5] = R2 >> 2
            frame[line +  96][6] = P >> 2
            frame[line + 112][7] = S >> 2

    return(frame)

def frameCRC(frame):
    for i in range(0, len(frame)):
        break

    return frame

def unpackbits(x, num_bits):
    xshape = list(x.shape)
    x = x.reshape([-1, 1])
    to_and = 2**np.arange(num_bits).reshape([1, num_bits])
    return np.flip((x & to_and).astype(bool).astype(int).reshape(xshape + [num_bits]), axis=0)

from PIL import Image, ImageDraw

def saveImage(frame1, frame2, fileName):
    img = Image.new('RGB', (128*5, len(frame1)*2))

    for i in range(0, len(frame)):
        for j in range(0, 8):
            ggg = unpackbits(frame1[i][j], 14)
            for k in range(0, 14):
                c = ggg[k] * 255
                img.putpixel((j * 14 * 5 + k * 5,     i * 2), (c, c, c))
                img.putpixel((j * 14 * 5 + k * 5 + 1, i * 2), (c, c, c))
                img.putpixel((j * 14 * 5 + k * 5 + 2, i * 2), (c, c, c))
                img.putpixel((j * 14 * 5 + k * 5 + 3, i * 2), (c, c, c))
                img.putpixel((j * 14 * 5 + k * 5 + 4, i * 2), (c, c, c))

            ggg = unpackbits(frame2[i][j], 14)
            for k in range(0, 14):
                c = ggg[k] * 255
                img.putpixel((j * 14 * 5 + k * 5,     i * 2 + 1), (c, c, c))
                img.putpixel((j * 14 * 5 + k * 5 + 1, i * 2 + 1), (c, c, c))
                img.putpixel((j * 14 * 5 + k * 5 + 2, i * 2 + 1), (c, c, c))
                img.putpixel((j * 14 * 5 + k * 5 + 3, i * 2 + 1), (c, c, c))
                img.putpixel((j * 14 * 5 + k * 5 + 4, i * 2 + 1), (c, c, c))

    #img.show()
    img.save(fileName + ".png")


waveFile = wave.open(fileName, mode="rb")
#data = np.frombuffer(waveFile.readframes(1), dtype=np.int16)
frame = np.full((128 + 7 * 16, 9), 0)
frame1 = encodeFrame(frame)
frame2 = encodeFrame(np.concatenate((frame1[(len(frame1) - 7*16):], np.full((128, 9), 0))))
frame3 = encodeFrame(np.concatenate((frame2[(len(frame1) - 7*16):], np.full((128, 9), 0))))
frame4 = encodeFrame(np.concatenate((frame3[(len(frame1) - 7*16):], np.full((128, 9), 0))))
frame5 = encodeFrame(np.concatenate((frame4[(len(frame1) - 7*16):], np.full((128, 9), 0))))

frame1 = np.concatenate((frame1[:len(frame2) - 7*16], frame2[:7*16]))
frame2 = np.concatenate((frame2[:len(frame2) - 7*16], frame3[:7*16]))
frame3 = np.concatenate((frame3[:len(frame2) - 7*16], frame4[:7*16]))
frame4 = np.concatenate((frame4[:len(frame2) - 7*16], frame5[:7*16]))

#saveImage(frame1, frame2, "sin_001")
#saveImage(frame3, frame4, "sin_002")


exit()

video = cv2.VideoCapture(fileName)

# TODO
start  = 24
end = 677
pixelSize = 5#.1015625

#fileName = fileName[:fileName.find(".")]

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
