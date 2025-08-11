# -*- coding: utf-8 -*-

import sys
import olsString

def parse_str(timeStamp, pid,tid,log_lv,tag_str , log_str):
    #print("log string in python, ", timeStamp,pid,"tag is ",tag_str, " data is",log_str)

    if "DSVTSPConnectSVC" in tag_str:

        logTxt = olsString.OlsString(log_str)
    
        if "Get vin" in log_str:
            logTxt.reSplit('=')
            return {"login vin" : logTxt.fetchStr(1)}
        elif "Get BatteryCode" in log_str:
            logTxt.reSplit('=')
            return {"login battery code" : logTxt.fetchStr(1)}
        elif "Request login to" in log_str:
            logTxt.reSplit('\[|\]')

            mode = ""
            if logTxt.fetchStr(6) == "1" :
                mode = "Country Platform"
            else :
                mode = "Enterprise platform"

            return {"login address" : logTxt.fetchStr(2),
                    "login port" : logTxt.fetchStr(4),
                    "login mode" : mode}
    return []

def get_caps():
    return "DSVTSPConnectSVC"
