import sys

def printHello(my_str):
    print("Hello World!")
    print(my_str)
    #sys.stderr("python system print error")

def AdditionFc(a, b):
    print("Now is in python module")
    print("{} + {} = {}".format(a, b, a+b))
    return a + b    


def OnRcvLine(line):
    print("Now is in python module")
    print("{} + {} = {}",line)
    return line.strip()  


