# -*- coding: utf-8 -*-

import sys
import olsString

def parse_str(timeStamp, pid,tid,log_lv,tag_str , log_str):
    #print("log string in python, ", timeStamp,pid,"tag is ",tag_str, " data is",log_str)
    if "DSVDiagnosticAPP" in tag_str:

        logTxt = olsString.OlsString(log_str)
    
        if "receive MCU msg" in log_str:
            logTxt.reSplit('=')
            return {"MCU msg" : log_str}
        elif "Cmd_para" in log_str:
            logTxt.reSplit('=')
            return {"Cmd_para" : log_str}
    elif "DSVFSALib" in tag_str:
        logTxt = olsString.OlsString(log_str)
    
        if "connect peerIP" in log_str:
            logTxt.reSplit(' ')
            return {"peer info" : logTxt.fetchStr(1)}
        elif "send to client success" in log_str:
            logTxt.reSplit('=')
            return {"send success" : "send length 23"}
    elif "DSVMcuComSvc" in tag_str:
        logTxt = olsString.OlsString(log_str)
        if "U w" in log_str:
            logTxt.reSplit(' ')
            return {"Uart send" : log_str}
        elif "U r" in log_str:
            logTxt.reSplit('=')
            return {"Uart recv" : log_str}
    elif "DSVPPEProxy" in tag_str:
        if "U w" in log_str:
            logTxt.reSplit(' ')
            return {"Uart send" : log_str}
        elif "U r" in log_str:
            logTxt.reSplit('=')
            return {"Uart recv" : log_str}
    elif "DSVMiscSVC" in tag_str:
        logTxt = olsString.OlsString(log_str)
        if "RTCTime mInfoValidFlag" in log_str:
            logTxt.reSplit(' ')
            return {"RTCTime " : logTxt.fetchStr(4)}
    elif "DSVLocationAPP" in tag_str:
        logTxt = olsString.OlsString(log_str)
        if "onGnssLocationCb_after shift" in log_str:
            logTxt.reSplit(' ')
            return {"onGnssLocationCb_after" : "403F48CBF258BF26"}
    elif "DSVTSPConnectSVC" in tag_str:

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

