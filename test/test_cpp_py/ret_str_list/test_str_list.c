#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
 
int main() {
    // 初始化 Python 解释器
    Py_Initialize();
 
    // 设置 Python 代码
    const char *python_code = "import sys\n"
                              "def get_strings():\n"
                              "    return ['string1', 'string2', 'string3']\n"
                              "sys.stdout.flush()";
    PyRun_SimpleString(python_code);
 
    // 调用 Python 函数并获取结果
    PyObject *pModuleName = PyUnicode_DecodeFSDefault("__main__");
    PyObject *pModule = PyImport_Import(pModuleName);
    PyObject *pFunc = PyObject_GetAttrString(pModule, "get_strings");
    PyObject *pValue = PyObject_CallObject(pFunc, NULL);
 
    // 检查是否有错误发生
    if (PyErr_Occurred()) {
        PyErr_Print();
        return 1;
    }
 
    // 步骤 3: 获取结果并转换
    // 将 Python 列表转换为 C 的字符串数组
    Py_ssize_t size;
    PyObject *item;
    const char *cstr;
    char **cstrArray;
     size =  PyList_Size(pValue);
    cstrArray = (char **)malloc(size * sizeof(char *));
    for (Py_ssize_t i = 0; i < size; ++i) {
        item = PyList_GetItem(pValue, i);
        cstr = PyUnicode_AsUTF8(item);
        cstrArray[i] = strdup(cstr); // 注意内存管理，确保释放这些字符串和数组
    }
 
    // 使用字符串数组...
    for (Py_ssize_t i = 0; i < size; ++i) {
        printf("%s\n", cstrArray[i]);
        free(cstrArray[i]); // 释放每个字符串的内存
    }
    free(cstrArray); // 释放字符串数组的内存指针
 
    // 清理工作
    Py_DECREF(pModuleName);
    Py_DECREF(pModule);
    Py_DECREF(pFunc);
    Py_DECREF(pValue);
    Py_Finalize(); // 关闭 Python 解释器
    return 0;
}