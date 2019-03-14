import pyuv
import signal
import socket

reader, writer = socket.socketpair()
reader.setblocking(False)
writer.setblocking(False)

loop = pyuv.Loop.default_loop()
prepare = pyuv.Prepare(loop)
signal_checker = pyuv.util.SignalChecker(loop, reader.fileno())

def prepare_cb(handle):
    print('Inside prepare_cb()')

def excepthook(typ, val, tb):
    print('Inside excepthook()')
    if typ is KeyboardInterrupt:
        print('The type was KeyboardInterrupt')
        prepare.stop()
        signal_checker.stop()

def main():
    loop.excepthook = excepthook

    prepare.start(prepare_cb)

    signal.set_wakeup_fd(writer.fileno())
    signal_checker.start()

    loop.run()

if __name__ == '__main__':
    main()
