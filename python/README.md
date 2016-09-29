Forwarder device for 0mq communications. The device sits on redis
server and collects messages from any client that modifies the
configuration and forwards the messages to any subscribed client.

Usage
=====

python configuration_forwarder.py -f <forwader port> -b <backend port>

Assume clients transmit messages on <forwader port> and listen on <backend port>

Demo
====

On the server, run

```python configuration_forwarder.py -f <frontend port> -b <backend port>```

to start the forwarder device.
On the client machine,

```python configuration_forwarder_test.py -t <frontend port> -s <server address>```

to transmit messages, and

```python configuration_forwarder_test.py -r <backend_port> -s  <server address> ```

to listen for messages. Notes: a filter on the topic "9" is set, the default server is localhost (127.0.0.1).

