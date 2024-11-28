#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WolframLibrary.h"
#include "WolframIOLibraryFunctions.h"
#include "WolframNumericArrayLibrary.h"

typedef struct test_data {
    int a;
    int b;
} test_data;

test_data current_data = {.a = 0, .b = 0};

DLLEXPORT void test_data_new(WolframLibraryData libData, mbool mode, mint id) {
    if (mode == 0) {
        current_data = (test_data){.a = 0, .b = 0};
    } else {
        current_data = (test_data){.a = 0, .b = 0};
    }
}

DLLEXPORT mint WolframLibrary_getVersion() {
    return WolframLibraryVersion;
}

DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData) {
    return (*libData->registerLibraryExpressionManager)("test", test_data_new);
}

DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData) {
    (*libData->unregisterLibraryExpressionManager)("test");
}

DLLEXPORT void test_data_set(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) {
    test_data data = current_data;
    data.a = MArgument_getInteger(Args[0]);
    data.b = MArgument_getInteger(Args[1]);
    current_data = data;
    MArgument_setInteger(Res, 0);
    return LIBRARY_NO_ERROR;
}

DLLEXPORT void test_data_get(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) {
    test_data data = current_data;
    if (Argc != 1) return LIBRARY_FUNCTION_ERROR;
    if (MArgument_getInteger(Args[0]) == 0) {
        MArgument_setInteger(Res, data.a);
    } else {
        MArgument_setInteger(Res, data.b);
    }
    return LIBRARY_NO_ERROR;
}