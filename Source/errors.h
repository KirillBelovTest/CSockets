#pragma once

#include "WolframLibrary.h"

// acceptErrorMessage: maps accept() errors to Wolfram messages
DLLEXPORT void acceptErrorMessage(WolframLibraryData libData, int err);

// recvErrorMessage: maps recv() errors to Wolfram messages
DLLEXPORT void recvErrorMessage(WolframLibraryData libData, int err);

// selectErrorMessage: maps select() errors to Wolfram messages
DLLEXPORT void selectErrorMessage(WolframLibraryData libData, int err);

// sendErrorMessage: maps send() errors to Wolfram messages
DLLEXPORT void sendErrorMessage(WolframLibraryData libData, int err);
