#include <Python.h>
 #include <iostream>


static inline bool py_error_(const char *func, int line)
{
	if (PyErr_Occurred()) {
		printf("Python failure in %s:%d:", func, line);
		PyErr_Print();
		return true;
	}
	return false;
}

#define py_error() py_error_(__FUNCTION__, __LINE__) 
 
int main(int argc, char *argv[])
{
    // 初始化Python解释器
    Py_Initialize();
 
    // 检查初始化是否成功
    if (!Py_IsInitialized()) {
        return -1;
    }
 
    // 执行Python代码
  //  PyRun_SimpleString(
   //     "from time import time,ctime\n"
   //     "print('Today is', ctime(time()))\n");
        
            // 2、初始化python系统文件路径，保证可以访问到 .py文件
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('./')");

	// 3、调用python文件名，不用写后缀
	PyObject* pModule = PyImport_ImportModule("helloworld");
	if( pModule == NULL ){
		std::cout <<"module not found" << std::endl;
		return 1;
	}
	// 4、调用函数
	PyObject* pFunc = PyObject_GetAttrString(pModule, "printHello");
	if( !pFunc || !PyCallable_Check(pFunc)){
		std::cout <<"not found function add_num" << std::endl;
		return 0;
	}

    PyObject* pArgs1 = PyTuple_New(1);
 
    // 0：第一个参数，传入 int 类型的值 2
    


    PyTuple_SetItem(pArgs1, 0,Py_BuildValue("s", "print in python")); 

    printf("parse object %p args %p\n",pFunc,pArgs1);
	PyObject_CallObject(pFunc, Py_BuildValue("(s)", "print in python"));//调用函数    
    py_error();   
        
   printf("%d.%d\n", PY_MAJOR_VERSION,
           PY_MINOR_VERSION);

    pFunc = PyObject_GetAttrString(pModule, "OnRcvLine");
    
    //5、给python传参数
    // 函数调用的参数传递均是以元组的形式打包的,2表示参数个数
    // 如果AdditionFc中只有一个参数时，写1就可以了
    PyObject* pArgs = PyTuple_New(1);
 
    // 0：第一个参数，传入 int 类型的值 2
    
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", "  hello   ")); 

    // 6、使用C++的python接口调用该函数
    PyObject*  pReturn = PyObject_CallObject(pFunc, pArgs);
    
    char *strResult;
    PyArg_Parse(pReturn, "s", &strResult);
    std::cout << "return result is " << strResult << std::endl;
    
    
    
    // 7、接收python计算好的返回值
    int nResult;
    // i表示转换成int型变量。
    // 在这里，最需要注意的是：PyArg_Parse的最后一个参数，必须加上“&”符号
    PyArg_Parse(pReturn, "i", &nResult);


    //4、调用函数
    pFunc = PyObject_GetAttrString(pModule, "AdditionFc");
    
    //5、给python传参数
    // 函数调用的参数传递均是以元组的形式打包的,2表示参数个数
    // 如果AdditionFc中只有一个参数时，写1就可以了
     pArgs = PyTuple_New(2);
 
    // 0：第一个参数，传入 int 类型的值 2
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("i", 2)); 
    // 1：第二个参数，传入 int 类型的值 4
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("i", 4)); 
    
    PyObject *args = Py_BuildValue("(iiiiss)", 2,4,2,3,"this is tag","this is value string");
    // 6、使用C++的python接口调用该函数
    pReturn = PyEval_CallObject(pFunc, args);
    
    // 7、接收python计算好的返回值
    //int nResult;
    // i表示转换成int型变量。
    // 在这里，最需要注意的是：PyArg_Parse的最后一个参数，必须加上“&”符号
    PyArg_Parse(pReturn, "i", &nResult);
    std::cout << "return result is " << nResult << std::endl;
    

	
    // 终止Python解释器
    Py_Finalize();
    return 0;
}
