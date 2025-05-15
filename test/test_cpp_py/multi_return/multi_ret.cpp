#include <Python.h>
#include <iostream>
 
int main() {
    // 初始化Python
    Py_Initialize();

    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");

    // 加载Python模块
    PyObject* pModule = PyImport_ImportModule("return_values");
    if (!pModule) {
        std::cerr << "Error loading Python module!" << std::endl;
        return 1;
    }
 
    // 获取模块中的函数
    PyObject* pFunc = PyObject_GetAttrString(pModule, "multiple_returns");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        std::cerr << "Cannot find function!" << std::endl;
        Py_DECREF(pModule);
        return 1;
    }
 
    // 调用Python函数
    PyObject* pValue = PyObject_CallObject(pFunc, nullptr);
    if (!pValue) {
        std::cerr << "Call failed!" << std::endl;
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return 1;
    }
 
    // 确保返回的是元组
    if (!PyTuple_Check(pValue)) {
        std::cerr << "The function did not return a tuple!" << std::endl;
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        Py_DECREF(pValue);
        return 1;
    }
 
    // 获取元组中的值
    for (Py_ssize_t i = 0; i < PyTuple_Size(pValue); i++) {
        PyObject* item = PyTuple_GetItem(pValue, i);
        if (PyLong_Check(item)) {
            printf("Element %lld\n", PyLong_AsLongLong(item));
        } else {
            std::cerr << "Unexpected type in tuple!" << std::endl;
        }
    }
 
    // 清理
    Py_DECREF(pFunc);
    Py_DECREF(pModule);
    Py_DECREF(pValue);
    Py_Finalize();
 
    return 0;
}
