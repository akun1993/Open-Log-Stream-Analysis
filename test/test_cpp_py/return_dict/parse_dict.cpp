#include <Python.h>
#include <iostream>
 
int main() {
    // 初始化Python
    Py_Initialize();

    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");

    // 加载Python模块
    PyObject* pModule = PyImport_ImportModule("return_dict");
    if (!pModule) {
        std::cerr << "Error loading Python module!" << std::endl;
        return 1;
    }
 
    // 获取模块中的函数
    PyObject* pFunc = PyObject_GetAttrString(pModule, "return_dict");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        std::cerr << "Cannot find function!" << std::endl;
        Py_DECREF(pModule);
        return 1;
    }
 
    // 调用Python函数
    PyObject* pDict = PyObject_CallObject(pFunc, nullptr);
    if (!pDict) {
        std::cerr << "Call failed!" << std::endl;
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return 1;
    }


    // Both are Python List objects
    PyObject *pKeys = PyDict_Keys(pDict);
    PyObject *pValues = PyDict_Values(pDict);

    for (Py_ssize_t i = 0; i < PyDict_Size(pDict); ++i) {
        // PyString_AsString returns a char*
        printf("key is %s val is %s \n",PyUnicode_AsUTF8( PyList_GetItem(pKeys,   i)),PyUnicode_AsUTF8( PyList_GetItem(pValues, i)));
    }
 
 
    // 清理
    Py_DECREF(pFunc);
    Py_DECREF(pModule);
    Py_DECREF(pDict);
    Py_DECREF(pKeys);
    Py_DECREF(pValues);
    Py_Finalize();
 
    return 0;
}
