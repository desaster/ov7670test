#
# Dependencies:
#
# python 2.7+
# twisted
#  zope.interface
# pyserial
# pygame
# pywin32

from twisted.internet import reactor
from twisted.internet import defer
from twisted.internet import protocol
from twisted.internet.defer import inlineCallbacks
from twisted.internet.task import LoopingCall
from twisted.internet.serialport import SerialPort
import pygame
import sys
import pickle

def parsergb565(byte1, byte2):
    byte12 = byte1 << 8 | byte2

    red = byte12 >> 8+3
    green = (byte12 >> 5) & 0x3f
    blue = byte12 & 0x1f
    
    red *= 8
    green *= 4
    blue *= 8

    return (red, green, blue)

class SpecialSerialProtocol(protocol.Protocol):

    recvd = ''
    dataDeferred = None
    clearSchedule = None
    dataSize = 0

    def dataReceived(self, data):
        for c in data:
            if self.dataDeferred and self.dataSize:
                self.recvd += c
                self.dataSize -= 1
                if self.dataSize == 0:
                    self.endDataHandler()
            elif self.dataDeferred and not self.dataSize:
                if c == '\n':
                    self.endDataHandler()
                elif c == '\r':
                    continue
                else:
                    self.recvd += c
            else:
                print 'Unhandled data: [%s]' % (c,)

    def endDataHandler(self):
        cb = self.dataDeferred.callback
        data = self.recvd
        self.dataDeferred = None
        self.dataSize = 0
        self.recvd = ''
        if not self.clearSchedule.called:
            self.clearSchedule.cancel()
        self.clearSchedule = None
        cb(data)

    def converse(self, msg, size = None, timeout = 1):
        if self.dataDeferred is not None:
            raise Exception, 'write callback already in progress!'
        d = defer.Deferred()
        self.dataDeferred = d
        if size:
            self.dataSize = size
        if timeout:
            self.clearSchedule = \
                reactor.callLater(timeout, self.endDataHandler)
        self.transport.write(msg)
        return d


class OV7670Test(SpecialSerialProtocol):

    @inlineCallbacks
    def getlines(self):
        ok = yield self.converse('getimage\r')
        newbuf = self.transport.app.imgbuf[:]
        for i in range(0, 120):
            data = ''
            while len(data) != 320:
                data = yield self.converse('getline %d\r' % (i,), 320)
            newbuf[i] = data
        self.transport.app.imgbuf = newbuf[:]

    def connectionMade(self):
        self.refresh = LoopingCall(self.getlines)
        self.refresh.start(0.001)

class Application(object):
    def __init__(self):
        self.screen = pygame.display.set_mode((320, 240))
        self.imgbuf = ['\0' * 320] * 120
        self.tick = LoopingCall(self.game_tick)
        self.tick.start(1.0 / 15) # desired FPS
        # Set up anything else twisted here, like listening sockets
        self.ov7670 = OV7670Test()
        self.serial = SerialPort(self.ov7670, 5, reactor, baudrate=921600)
        self.serial.app = self

    def quit(self):
        print 'Quitting! (or more likely crashing)'
        self.tick.stop()
        reactor.stop()

    def game_tick(self):
        self.redraw()
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.quit()
            elif (event.type == pygame.KEYDOWN):
                if (event.key == pygame.K_q):
                    self.quit()
                if (event.key == pygame.K_p):
                    self.ov7670.refresh.stop()
                if (event.key == pygame.K_r):
                    self.ov7670.refresh.start(0.001)
                if (event.key == pygame.K_SPACE):
                    self.imgbuf = ['\0' * 320] * 120
                    self.ov7670.getlines()

    def redraw(self):
        for y in range(0, 120):
            i = 0
            for x in range(0, 160):
                color = parsergb565(
                    ord(self.imgbuf[y][i]),
                    ord(self.imgbuf[y][i + 1]))
                i += 2

                self.screen.set_at((2 * x, 2 * y), color)
                self.screen.set_at((2 * x + 1, 2 * y), color)
                self.screen.set_at((2 * x, 2 * y + 1), color)
                self.screen.set_at((2 * x + 1, 2 * y + 1), color)
        pygame.display.flip()

if __name__ == '__main__':
    Application()
    reactor.run()

# vim: set sw=4 et:
