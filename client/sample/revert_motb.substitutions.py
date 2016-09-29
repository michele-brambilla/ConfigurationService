import argparse
import json
import os

import numpy as np

from pprint import pprint

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Redis client test")
    parser.add_argument("-s","--source",
                        action="store",
                        nargs='?',
                        type=str,
                        help="revert motb.substitutions to redis db")

    args = parser.parse_args()
    print args.source

    s = ""
    d = dict()
    with open(args.source) as data:
        while s.find("pattern") < 0:
            s = data.readline()
        s = data.readline().rstrip()
        keys = s.rstrip().rstrip("}").lstrip("{").split()
        while True :
            s = data.readline()
            if s.find("{"):
                break
            l = s.rstrip().rstrip("}").lstrip("{").split(",")
            d[l[1]] = l[2:]
    prefix = l[0]

    print keys
