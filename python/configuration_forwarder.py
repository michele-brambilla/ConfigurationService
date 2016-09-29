import sys
import argparse
import zmq

def main(frontend_port,backend_port):

    try:
        context = zmq.Context(1)
        # Socket facing clients
        frontend = context.socket(zmq.SUB)
        frontend.bind("tcp://*:"+frontend_port)
        
        frontend.setsockopt(zmq.SUBSCRIBE, "")
        
        # Socket facing services
        backend = context.socket(zmq.PUB)
        backend.bind("tcp://*:"+backend_port)

        zmq.device(zmq.FORWARDER, frontend, backend)
    except Exception, e:
        print e
        print "bringing down zmq device"
    finally:
        pass
        frontend.close()
        backend.close()
        context.term()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Python-0MQ forwarder for redis updates")
    parser.add_argument("-f","--frontend",
                        action="store",
                        nargs='?',
                        type=str,
                        default="5554",
                        help="0MQ frontend port number")
    parser.add_argument("-b","--backend", 
                        action="store",
                        nargs='?',
                        default="5555",
                        type=str,
                        help="0MQ backend port number")

    args = parser.parse_args()
    main(args.frontend,args.backend)
