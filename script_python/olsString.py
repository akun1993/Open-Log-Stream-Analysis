# -*- coding: utf-8 -*-
import re

class OlsString(object):
    def __init__(self,str_val):
        self.__str_val = str_val
        self.__split_list = []
        self.__split_list_len = 0 
    def split(self,delimiter = ' '):
        self.__split_list = [expression for expression in self.__str_val.split(delimiter) if expression] 
        self.__split_list_len = len(self.__split_list)
    

    def find(self,str_val):
        return self.__str_val.find(str_val)


    def startWith(self,str_val):
        return self.__str_val.startswith(str_val)

    '''
    Use re split str , delimiter can be 
    '''    
    def reSplit(self, delimiter ):
        self.__split_list = [expression for expression in re.split(delimiter,self.__str_val) if expression] 
        self.__split_list_len = len(self.__split_list)
        

    '''
    fetch str val by idx  , fetch len after idx
    '''
    def fetchStr(self ,idx , len = 1):
        if(idx >= self.__split_list_len ):
            return ""
        return self.__split_list[idx]
    
    '''
    fetch numner int val by idx 
    '''
    def fetchInt(self,idx ):
        if(idx >= self.__split_list_len ):
            return ""
        return int(self.__split_list[idx])
    
    '''
    fetch numner float val by idx 
    '''
    def fetchFloat(self,idx ):
        if(idx >= self.__split_list_len ):
            return ""
        return float(self.__split_list[idx])    


