import sys
import argparse
import zmq
import random
import time

def run_server(address, backend_port):
    try:
        context = zmq.Context()
        socket = context.socket(zmq.PUB)
        socket.connect("tcp://{}:{}".format(address,backend_port))
        publisher_id = random.randrange(0,9999)
        for update in range(10):
            topic = random.randrange(1,10)
            messagedata = "server#%s" % publisher_id
            print "%s %s" % (topic, messagedata)
            socket.send("%d %s" % (topic, messagedata))
            time.sleep(1)
    except Exception, e:
        print e
        print "bringing down zmq test server"
    finally:
        pass
        socket.close()

def run_client(address, frontend_port):
    try:
        context = zmq.Context()
        socket = context.socket(zmq.SUB)
        print "Collecting updates from server..."
        socket.connect ("tcp://{}:{}".format(address, frontend_port) )
        topicfilter = "9"
        socket.setsockopt(zmq.SUBSCRIBE, topicfilter)
        for update_nbr in range(10):
            string = socket.recv()
            topic, messagedata = string.split()
            print "> ", topic, messagedata
    except Exception, e:
        print e
        print "bringing down zmq test client"
    finally:
        pass
        socket.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="test Python-0MQ forwarder for redis updates")
    parser.add_argument("-t","--trasmitter",
                        action="store",
                        nargs='?',
                        type=str,
                        help="0MQ port number where messages are sent")
    parser.add_argument("-r","--receiver", 
                        action="store",
                        nargs='?',
                        type=str,
                        help="0MQ port number to listen for messages")
    parser.add_argument("-s","--server", 
                        action="store",
                        nargs='?',
                        default='127.0.0.1',
                        type=str,
                        help="device server address")

    args = parser.parse_args()
    print args

    if args.trasmitter:
        run_server(args.server, args.trasmitter)

    if args.receiver:
        run_client(args.server, args.receiver)

        
