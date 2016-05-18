import fastpath
import signal

shutdown = False

def signalHandler(event, event_type):
    print("Handling signal")
    shutdown = True;

def msgHandler(subscriber, message):
    print("Received: %s" % (message, ));

fastpath.initialise()

t = fastpath.TCPTransport('tcp://localhost:6363', 'TEST')

m = fastpath.MutableMessage()
m.setSubject("SOME.TEST.MESSAGE")
m.addStringField("Name", "Tom")

print("My messages is: %s" % (m, ));
s = t.sendMessage(m);
print("Sent message: %s" %(s, ));

subscriber = fastpath.Subscriber(t, ">", msgHandler)
q = fastpath.BlockingQueue()

q.registerSignalEvent(signal.SIGINT, signalHandler)
e2 = q.addSubscriber(subscriber)

while shutdown is False:
    status = q.dispatch()
    if status is not fastpath.OK:
        shutdown = True

fastpath.destroy()
