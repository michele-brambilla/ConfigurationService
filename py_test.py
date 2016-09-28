import zmq
context = zmq.Context()
zmq_socket = context.socket(zmq.PUB)
zmq_socket.connect("tcp://127.0.0.1:5554")
zmq_socket.send("Hello")

while 1 :
    msg = raw_input('')
    zmq_socket.send(msg)
    
