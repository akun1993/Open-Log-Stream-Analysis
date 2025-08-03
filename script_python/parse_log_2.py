import sys


def parse_str(timeStamp, pid,tid,log_lv,tag_str , log_str):
    #print("log string in python, ", timeStamp,pid,"tag is ",tag_str, " data is",log_str)
    if "DSVTSPConnectSVC" in tag_str:
        if "Get vin" in log_str:
            return [log_str]
        elif "Get BatteryCode" in log_str:
            return [log_str]
        elif "Request login to" in log_str:
            return [log_str]
    return []

def get_caps():
    return "DSVTSPConnectSVC"
