import sys



def printHello(my_str):
    print("Hello World!")
    print(my_str)
    #sys.stderr("python system print error")

def AdditionFc(timeStamp, pid,tid,log_lv,tag_str , log_str):
    print("Now is in python module tag ",tag_str, "  val str ",log_str)
    print("{} + {} + {} + {} = {}".format(timeStamp, pid,tid,log_lv, timeStamp+pid + tid + log_lv))
    return timeStamp+pid + tid + log_lv


def OnRcvLine(line):
    print("Now is in python module")
    print("{} + {} = {}",line)
    return line.strip()  


